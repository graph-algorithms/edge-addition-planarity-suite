"""Automate the edge-deletion analysis on the graph(s) in a .g6 input file

Classes:
    EdgeDeletionAnalysisError
    EdgeDeletionAnalyzer
"""

#!/usr/bin/env python

__all__ = ["EdgeDeletionAnalyzer", "EdgeDeletionAnalysisError"]

import argparse
from copy import deepcopy
import logging
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Optional

from graph import Graph, GraphError
from planaritytesting_utils import g6_header, g6_suffix, LEDA_header


class EdgeDeletionAnalysisError(BaseException):
    """Signals issues encountered during edge deletion analysis

    For example:
        - Nonzero exit code from running various planarity commands
    """

    def __init__(self, message):
        super().__init__(message)
        self.message = message


class EdgeDeletionAnalyzer:
    """Performs edge-deletion analysis on graph(s) in a .g6 input file"""

    __MAX_NUM_ERRORS = 1000

    def __init__(
        self,
        planarity_path: Path,
        infile_path: Path,
        output_dir: Optional[Path] = None,
        log_path: Optional[Path] = None,
    ) -> None:
        """Validate input and set up for edge-deletion analysis

        If output_dir was not None and doesn't correspond to a file,
        self.output_dir will be set to:
            {output_dir}/{infile_path.stem}
        If output_dir was None, self.output_dir will be set to
            TestSupport/results/edge_deletion_analysis/{infile_path.stem}
        Where the infile_path.stem removes the final suffix, e.g. for file
        n{order}.m{num_edges}.g6, we create the directory
            TestSupport/results/edge_deletion_analysis/n{order}.m{num_edges}

        If the output_dir exists, all of its contents are wiped and the
        directory remade. Then, the input file is copied into this directory.

        Args:
            planarity_path: Path to planarity executable
            infile_path: Path to graph input file
            output_dir: Directory under which a new subdirectory will be
                created to which results will be written.

        Raises:
            argparse.ArgumentTypeError: If the planarity_path doesn't
                correspond to an executable, if the infile_path doesn't
                correspond to a file, or if the output_dir corresponds to a
                file (rather than an existing directory, or to a directory that
                does not yet exist)
        """
        if (
            not planarity_path
            or not isinstance(planarity_path, Path)
            or not shutil.which(str(planarity_path.resolve()))
        ):
            raise argparse.ArgumentTypeError(
                f"Path for planarity executable '{planarity_path}' does not "
                "correspond to an executable."
            )

        if not infile_path.is_file():
            raise argparse.ArgumentTypeError(
                f"Path '{infile_path}' doesn't correspond to a file."
            )

        if not output_dir:
            script_entrypoint = Path(sys.argv[0]).resolve().parent.parent
            output_dir = Path.joinpath(
                script_entrypoint, "results", "edge_deletion_analysis"
            )

        if output_dir.is_file():
            raise argparse.ArgumentTypeError(
                f"Path '{output_dir}' corresponds to a file, and can't be "
                "used as the output directory."
            )

        output_dir = Path.joinpath(output_dir, f"{infile_path.stem}")

        if Path.is_dir(output_dir):
            shutil.rmtree(output_dir)

        Path.mkdir(output_dir, parents=True, exist_ok=True)

        infile_copy_path = Path.joinpath(
            output_dir, infile_path.name
        ).resolve()

        shutil.copyfile(infile_path, infile_copy_path)

        self.planarity_path = planarity_path.resolve()
        self.orig_infile_path = infile_copy_path
        self.output_dir = output_dir

        self._setup_logger(log_path)

    @staticmethod
    def _determine_input_filetype(infile_path: Path) -> str:
        """Determine whether input file encoding
        Args:
            infile_path: Path to graph input file

        Returns:
            str: One of 'LEDA', 'AdjList', 'AdjMat', or 'G6'

        Raises:
            EdgeDeletionAnalysisError: If infile is empty or if unable to
                determine the input file encoding
        """
        with open(infile_path, "r", encoding="utf-8") as infile:
            first_line = infile.readline()
            if not first_line:
                raise EdgeDeletionAnalysisError(f"'{infile_path}' is empty.")
            if LEDA_header() in first_line:
                return "LEDA"
            if re.match(r"N=(\d+)", first_line):
                return "AdjList"
            if first_line[0].isdigit():
                return "AdjMat"
            if infile_path.suffix == g6_suffix() and (
                first_line.find(g6_header())
                or (ord(first_line[0]) >= 63 and ord(first_line[0]) <= 126)
            ):
                return "G6"

            raise EdgeDeletionAnalysisError(
                f"Unable to determine filetype of '{infile_path}'."
            )

    def _setup_logger(self, log_path: Optional[Path] = None) -> None:
        """Set up logger instance for EdgeDeletionAnalyzer

        Args:
            log_path: If none, defaults to:
                {self.output_dir}/edge_deletion_analysis-{graph_input_name}.log
        """
        graph_input_name = self.orig_infile_path.stem

        if not log_path:
            log_path = Path.joinpath(
                self.output_dir,
                f"edge_deletion_analysis-{graph_input_name}.log",
            )

        # logging.getLogger() returns the *same instance* of a logger
        # when given the same name. In order to prevent this, must either give
        # a unique name, or must prevent adding additional handlers to the
        # logger
        self.logger = logging.getLogger(__name__ + graph_input_name)
        self.logger.setLevel(logging.DEBUG)

        if not self.logger.handlers:
            # Create the Handler for logging data to a file
            logger_handler = logging.FileHandler(filename=log_path)
            logger_handler.setLevel(logging.DEBUG)

            # Create a Formatter for formatting the log messages
            logger_formatter = logging.Formatter(
                "[%(levelname)s] - %(module)s.%(funcName)s - %(message)s"
            )

            # Add the Formatter to the Handler
            logger_handler.setFormatter(logger_formatter)

            # Add the Handler to the Logger
            self.logger.addHandler(logger_handler)

            stderr_handler = logging.StreamHandler()
            stderr_handler.setLevel(logging.ERROR)
            stderr_handler.setFormatter(logger_formatter)
            self.logger.addHandler(stderr_handler)

    def analyze_graphs(self) -> None:
        """Transform input graph(s) and perform edge-deletion analysis on each

        Iterates over contents of .g6 input file; for each line fo the input
        file, write a new .g6 file containing only that graph. Then, run the
        transformation to adjacency list, and finally perform the edge-deletion
        analysis steps to determine if the graph contains a K_{3, 3} that was
        missed by the K_{3, 3} search graph algorithm extension.

        Raises:
            EdgeDeletionAnalysisError: If an error occured when trying to
                determine the input file type, if the input file is anything
                other than a .g6 file, if transformation failed for the input
                graph(s), or if any of the analysis steps failed for the input
                graph(s)
        """
        try:
            file_type = EdgeDeletionAnalyzer._determine_input_filetype(
                self.orig_infile_path
            )
        except EdgeDeletionAnalysisError as input_filetype_error:
            raise EdgeDeletionAnalysisError(
                "Failed to determine input filetype of "
                f"'{self.orig_infile_path}'."
            ) from input_filetype_error

        if file_type != "G6":
            raise EdgeDeletionAnalysisError(
                f"'{file_type}' is not supported; please supply a .g6 file."
            )

        num_errors = 0
        with open(self.orig_infile_path, "r", encoding="utf-8") as orig_infile:
            line_num = 0
            for line in orig_infile:
                line_num += 1
                if line_num == 1:
                    line.replace(g6_header(), "")

                new_g6_name = f"{self.orig_infile_path.name}." f"{line_num}"
                new_parent_dir = Path.joinpath(self.output_dir, new_g6_name)
                Path.mkdir(new_parent_dir, parents=True, exist_ok=True)
                new_g6_path = Path.joinpath(
                    new_parent_dir, f"{new_g6_name}.g6"
                )
                with open(new_g6_path, "w", encoding="utf-8") as new_g6_file:
                    new_g6_file.write(line)

                try:
                    adj_list_path = self.transform_input(
                        new_g6_path, new_parent_dir
                    )
                except EdgeDeletionAnalysisError as input_transform_error:
                    raise EdgeDeletionAnalysisError(
                        f"Unable to transform graph on line {line_num} in "
                        f"'{self.orig_infile_path}'."
                    ) from input_transform_error

                try:
                    contains_missed_k33 = self.analyze_transformed_graph(
                        adj_list_path, new_parent_dir
                    )
                except EdgeDeletionAnalysisError as analysis_error:
                    raise EdgeDeletionAnalysisError(
                        "Error encountered analyzing graph on line "
                        f"{line_num} in '{self.orig_infile_path}'."
                    ) from analysis_error

                if not contains_missed_k33:
                    shutil.rmtree(new_parent_dir, ignore_errors=True)
                else:
                    num_errors += 1
                    if num_errors > EdgeDeletionAnalyzer.__MAX_NUM_ERRORS:
                        self.logger.error(
                            "Encountered more errors than supported "
                            "(i.e. %d)",
                            EdgeDeletionAnalyzer.__MAX_NUM_ERRORS,
                        )
                        break

    def transform_input(self, infile_path: Path, output_dir: Path) -> Path:
        """Transforms input graph to adjacency list

        Runs
            planarity -t -ta {input_file} {infile_stem}.AdjList.out.txt
        to transform the graph to adjacency list

        Args:
            infile_path: Path to input file containing a single graph
            output_dir: Directory to which transformed graph is output

        Returns:
            Path to Adjacency List representation of input graph.

        Raises:
            EdgeDeletionAnalysisError: if calling planarity returned nonzero
                exit code
        """
        adj_list_path = Path.joinpath(
            output_dir, infile_path.stem + ".AdjList.out.txt"
        )

        planarity_transform_args = [
            f"{self.planarity_path}",
            "-t",
            "-ta",
            f"{infile_path}",
            f"{adj_list_path}",
        ]

        # TestGraphFunctionality() returns either OK or NOTOK, which means
        # commandLine() will return 0 or -1.
        try:
            subprocess.run(planarity_transform_args, check=True)
        except subprocess.CalledProcessError as e:
            error_message = (
                f"Unable to transform '{infile_path}' " "to Adjacency list."
            )
            self.logger.error(error_message)
            raise EdgeDeletionAnalysisError(error_message) from e

        return adj_list_path

    def analyze_transformed_graph(
        self, adj_list_path: Path, output_dir: Path
    ) -> bool:
        """Perform steps of edge-deletion analysis
        
        First os.chdir() to the output_dir, since SpecificGraph() calls
        ConstructInputFilename(), which enforces a limit on the length of the
        infileName.

        2. Runs 
            planarity -s -3 {infile_stem}.AdjList.out.txt \
                {infile_stem}.AdjList.s.3.out.txt
        and report whether a subgraph homeomorphic to K_{3, 3} was found
        (NONEMBEDDABLE) or not.
        3. If no subgraph homeomorphic to K_{3, 3} was found (i.e. previous
        test yielded OK), run
            planarity -s -p {infile_stem}.AdjList.out.txt \
                {infile_stem}.AdjList.s.p.out.txt \
                {infile_stem}.AdjList.s.p.obstruction.txt
        a. If the graph is reported as planar, then we can be sure no K_{3, 3}
        exists in the graph and execution continues without running the rest
        of the loop body.
        b. If the graph was reported as nonplanar, then examine the obstruction
        in {input_file}.AdjList.s.p.obstruction.txt:
            i. If the obstruction is homeomorphic to K_{3, 3}, then report that
            the original graph contains a subgraph homeomorphic to K_{3, 3}
            that was not found by the K_{3, 3} search
            ii. If the obstruction is homeomorphic to K_5, then perform the
            edge-deletion analysis to determine whether the graph doesn't
            contain a K_{3, 3} (with a high degree of confidence)

        Args:
            adj_list_path: Path to adjacency list representation of input graph
            output_dir: Path to output directory

        Return:
            bool indicating whether or not a K_{3, 3} was found that K_{3, 3}
                search missed
        Raises:
            EdgeDeletionAnalysisError: re-raised from any step of the analysis
        """
        contains_missed_k33 = False
        orig_dir = os.getcwd()
        os.chdir(output_dir)
        try:
            contains_k33 = self._run_k33_search(adj_list_path)
            if not contains_k33:
                planar_obstruction_name = self._run_planarity(
                    adj_list_path, output_dir
                )
                if planar_obstruction_name is not None:
                    obstruction_type = self.determine_obstruction_type(
                        planar_obstruction_name, output_dir
                    )
                    planar_obstruction_path = Path.joinpath(
                        output_dir, planar_obstruction_name
                    )
                    if obstruction_type == "K33":
                        self.logger.error(
                            "In '%s', planarity found a "
                            "K_{3, 3} that was not found by K_{3, 3} search; "
                            "see '%s'.",
                            adj_list_path,
                            planar_obstruction_path,
                        )
                        contains_missed_k33 = True
                    elif obstruction_type == "K5":
                        self.logger.info(
                            "'%s' contains a subgraph "
                            "homeomorphic to K_5; proceeding with "
                            "edge-deletion analysis."
                            "\n=======================\n",
                            adj_list_path,
                        )
                        contains_missed_k33 = self._edge_deletion_analysis(
                            adj_list_path, output_dir
                        )
                        self.logger.info("\n=======================\n")
                        if contains_missed_k33:
                            self.logger.error(
                                "In '%s', edge-deletion "
                                "analysis determined that there is a "
                                "K_{3, 3} that was not found by K_{3, 3} "
                                "search.",
                                adj_list_path,
                            )
                        else:
                            self.logger.info(
                                "'%s' likely doesn't contain a K_{3, 3}.",
                                adj_list_path,
                            )
        except EdgeDeletionAnalysisError as e:
            raise EdgeDeletionAnalysisError(
                f"Encountered error when processing '{adj_list_path}'."
            ) from e
        else:
            return contains_missed_k33
        finally:
            os.chdir(orig_dir)

    def _run_k33_search(self, graph_infile_path: Path) -> Optional[bool]:
        """Run K_{3, 3} search

        Args:
            graph_infile_path: Path to graph on which you wish to run K_{3, 3}
                search

        Returns:
            bool: indicates whether or not the input graph contained a K_{3, 3}

        Raises:
            EdgeDeletionAnalysisError: If calling planarity returned anything
                other than 0 (OK) or 1 (NONEMBEDDABLE), or if messages emitted
                to stdout don't align with their respective exit codes
        """
        k33_search_output_name = Path(graph_infile_path.stem + ".s.3.out.txt")

        k33_search_args = [
            f"{self.planarity_path}",
            "-s",
            "-3",
            f"{graph_infile_path.name}",
            f"{k33_search_output_name}",
        ]

        # check=False since exit code will be nonzero if NONEMBEDDABLE, which
        # is not an error state.
        result = subprocess.run(
            k33_search_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

        # SpecificGraph() will return OK, NONEMBEDDABLE, or NOTOK. This means
        # commandLine() will return 0, 1, or -1 for those three cases; since
        # subprocess.run() with check=True would raise a CalledProcessError for
        # nonzero exit codes, this logic is required to handle when we *truly*
        # encounter an error, i.e. NOTOK yielding exit code -1
        if result.returncode not in (0, 1):
            try:
                result.check_returncode()
            except subprocess.CalledProcessError as e:
                error_message = (
                    "Encountered an error running K_{3, 3} "
                    f"search on '{graph_infile_path}':"
                    f"\n\tOutput to stdout:\n\t\t{result.stdout}"
                    f"\n\tOutput to stderr:\n\t\t{result.stderr}"
                )
                self.logger.error(error_message)
                raise EdgeDeletionAnalysisError(error_message) from e

        if result.returncode == 1:
            if b"has a subgraph homeomorphic to" not in result.stdout:
                error_message = (
                    "planarity SpecificGraph() exit code "
                    "doesn't align with stdout result for "
                    f"'{graph_infile_path}':"
                    f"\n\tOutput to stdout:\n\t\t{result.stdout}"
                    f"\n\tOutput to stderr:\n\t\t{result.stderr}"
                )
                self.logger.error(error_message)
                raise EdgeDeletionAnalysisError(error_message)
            self.logger.info(
                "'%s' contains a subgraph homeomorphic "
                "to K_{3, 3}; "
                "see '%s'",
                graph_infile_path,
                k33_search_output_name,
            )
            return True
        elif result.returncode == 0:
            if b"has no subgraph homeomorphic to" not in result.stdout:
                error_message = (
                    "planarity SpecificGraph() exit code "
                    "doesn't align with stdout result for "
                    f"'{graph_infile_path}':"
                    f"\n\tOutput to stdout:\n\t\t{result.stdout}"
                    f"\n\tOutput to stderr:\n\t\t{result.stderr}"
                )
                self.logger.error(error_message)
                raise EdgeDeletionAnalysisError(error_message)
            self.logger.info(
                "'%s' is reported as not containing a "
                "subgraph homeomorphic to K_{3, 3}.",
                graph_infile_path,
            )
            return False

    def _run_planarity(
        self, graph_infile_path: Path, output_dir: Path
    ) -> Optional[Path]:
        """Invoke planar graph embedder/Kuratowski subgraph isolator

        Args:
            graph_infile_path: Path to graph on which you wish to run planarity
            output_dir: Path to which you wish to output results of planarity

        Returns:
            planar_obstruction_name: Path object that only contains the stem
                and extension of the obstruction to planarity; expected to be a
                file within output_dir.

        Raises:
            EdgeDeletionAnalysisError: if calling planarity returned anything
                other than 0 (OK) or 1 (NONEMBEDDABLE), or if messages emitted
                to stdout don't align with their respective exit codes
        """
        core_planarity_output_name = Path(
            graph_infile_path.stem + ".s.p.out.txt"
        )
        planar_obstruction_name = Path(
            graph_infile_path.stem + ".s.p.obstruction.txt"
        )

        core_planarity_args = [
            f"{self.planarity_path}",
            "-s",
            "-p",
            f"{graph_infile_path.name}",
            f"{core_planarity_output_name}",
            f"{planar_obstruction_name}",
        ]

        # check=False since we want to handle nonzero exit code appropriately
        result = subprocess.run(
            core_planarity_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

        # SpecificGraph() will return OK, NONEMBEDDABLE, or NOTOK. This means
        # commandLine() will return 0, 1, or -1 for those three cases; since
        # subprocess.run() with check=True would raise a CalledProcessError for
        # nonzero exit codes, this logic is required to handle when we *truly*
        # encounter an error, i.e. NOTOK yielding exit code -1
        if result.returncode not in (0, 1):
            try:
                result.check_returncode()
            except subprocess.CalledProcessError as e:
                error_message = (
                    "Encountered an error running core "
                    f"planarity on '{graph_infile_path}':"
                    f"\n\tOutput to stdout:\n\t\t{result.stdout}"
                    f"\n\tOutput to stderr:\n\t\t{result.stderr}"
                )
                self.logger.error(error_message)
                raise EdgeDeletionAnalysisError(error_message) from e

        if result.returncode == 1:
            if b"is not planar" not in result.stdout:
                error_message = (
                    "planarity SpecificGraph() exit code "
                    "doesn't align with stdout result for "
                    f"'{graph_infile_path}':"
                    f"\n\tOutput to stdout:\n\t\t{result.stdout}"
                    f"\n\tOutput to stderr:\n\t\t{result.stderr}"
                )
                self.logger.error(error_message)
                raise EdgeDeletionAnalysisError(error_message)
            self.logger.info(
                "'%s' is nonplanar; see '%s'",
                graph_infile_path,
                planar_obstruction_name,
            )
        elif result.returncode == 0:
            if b"is planar" not in result.stdout:
                error_message = (
                    "planarity SpecificGraph() exit code "
                    "doesn't align with stdout result for "
                    f"'{graph_infile_path}':"
                    f"\n\tOutput to stdout:\n\t\t{result.stdout}"
                    f"\n\tOutput to stderr:\n\t\t{result.stderr}"
                )
                self.logger.error(error_message)
                raise EdgeDeletionAnalysisError(error_message)
            core_planarity_output_path = Path.joinpath(
                output_dir, core_planarity_output_name
            )
            self.logger.info(
                "'%s' is reported as planar, as shown in '%s'.",
                graph_infile_path,
                core_planarity_output_path,
            )
            planar_obstruction_name = None

        return planar_obstruction_name

    def determine_obstruction_type(
        self, planar_obstruction_name: Path, output_dir: Path
    ) -> str:
        """Determine obstruction type based on max degree

        A coarse test for the type of obstruction encountered by the planar
        graph embedder/Kuratowski subgraph isolator.

        Returns:
            Either 'K33' if max degree is 3 or 'K5' if max degree is 4

        Raises:
            EdgeDeletionAnalysisError: If error encountered when trying to read
                obstruction graph from file, or if the obstruction found
                doesn't have the expected characteristics of neither K_5 nor
                K_{3, 3}
        """
        try:
            obstruction_graph = EdgeDeletionAnalyzer._read_adj_list_graph_repr(
                planar_obstruction_name
            )
        except EdgeDeletionAnalysisError as e:
            raise EdgeDeletionAnalysisError(
                "Encountered error when trying to read obstruction subgraph "
                "from "
                f"'{Path.joinpath(output_dir, planar_obstruction_name)}'"
            ) from e

        obstruction_max_degree = obstruction_graph.get_max_degree()
        if obstruction_max_degree == 3:
            return "K33"
        elif obstruction_max_degree == 4:
            return "K5"
        else:
            raise EdgeDeletionAnalysisError(
                "The obstruction found doesn't have the expected "
                "characteristics of neither K_5 nor K_{3, 3}; see "
                f"'{Path.joinpath(output_dir, planar_obstruction_name)}'."
            )

    @staticmethod
    def _read_adj_list_graph_repr(graph_path: Path) -> Graph:
        """Static method to read graph as adjacency list from file

        Args:
            graph_path: Path to adjacency-list representation of graph we wish
                to use to initialize a Graph object

        Returns:
            Graph with order and graph_adj_list_repr corresponding to file
            contents.

        Raises:
            EdgeDeletionAnalysisError: If file header is invalid, or if
                GraphError was encountered when we tried to add an edge based
                on file contents.
        """
        with open(graph_path, "r", encoding="utf-8") as graph_file:
            header = graph_file.readline()
            match = re.match(r"N=(?P<order>\d+)", header)
            if not match:
                raise EdgeDeletionAnalysisError(
                    f"Infile '{graph_path}' doesn't contain "
                    "an adjacency list: invalid header."
                )

            order = int(match.group("order"))
            graph = Graph(order)
            line_num = 0
            for line in graph_file:
                line_num += 1
                line_elements = line.split(":")
                u = int(line_elements[0])
                neighbours = [
                    int(v) for v in line_elements[1].split() if v if v != "-1"
                ]
                for v in neighbours:
                    try:
                        graph.add_arc(u, v)
                    except GraphError as e:
                        raise EdgeDeletionAnalysisError(
                            f"Unable to add edge ({u}, {v}) as specified on"
                            f"line {line_num} of '{graph_path}'."
                        ) from e

        return graph

    def _edge_deletion_analysis(  # pylint: disable=too-many-locals
        self, adj_list_path: Path, output_dir: Path
    ) -> bool:
        """Search for K_{3, 3} by removing graph edges and re-running planarity

        If the original graph was determined to not contain a subgraph
        homeomorphic to K_{3, 3} by the K_{3, 3} search but was reported as
        nonplanar and the obstruction found was homeomorphic to K_5, we want to
        see if there is no subgraph homeomorphic to K_{3, 3} with a high degree
        of confidence (not a guarantee).

        This is done by iterating over the graph_adj_list_repr of the original
        graph:
            - for each edge, if the head vertex label is greater than the
            tail vertex label, create a deepcopy of the original graph and
            remove that edge before re-running planarity to see if it is
            planar. If so, then continue to next iteration. If it is nonplanar,
            either it finds an obstruction homeomorphic to K_{3, 3} and
            we emit a message and continue to the next iteration, or the
            obstruction is homeomorphic to K_5. If so, run K_{3, 3} search on
            the graph-minus-one-edge and see if we still report that there is
            no K_{3, 3}. It may be the case that the graph-minus-one-edge still
            has the same underlying structure that causes the issue with
            K_{3, 3} search on the original graph, so even this additional test
            is not a 100% guarantee of finding the K_{3, 3} that we should have
            found in the original graph.

        Args:
            adj_list_path: Path to adjacency list upon which to perform
                edge-deletion analysis
            output_dir: Directory to which you wish to output the results of
                edge-deletion analysis on the input graph

        Returns:
            bool indicating at least one K_{3, 3} was found as a result of the
            edge-deletion manipulation.

        Raises:
            EdgeDeletionAnalysisError is raised if:
                - the original adjacency list produced by the transform is
                1-based rather than the expected 0-based
                    - this could happen if rather than starting with a graph in
                    .g6 format, your input graph is an adjacency list produced
                    by planarity when NIL is set to 0 rather than -1, causing
                    gp_GetFirstVertex(theGraph) to return 1 and line
                    terminators to be set to 0 because
                    theGraph->internalFlags & FLAGS_ZEROBASEDIO is falsy
                - Deleting an edge from the copied graph fails
                - Running planarity on the original-graph-minus-one-edge fails
                - Determining the type of obstruction found fails
        """
        try:
            orig_graph = EdgeDeletionAnalyzer._read_adj_list_graph_repr(
                adj_list_path
            )
        except EdgeDeletionAnalysisError as e:
            raise EdgeDeletionAnalysisError(
                "Unable to read original graph adjacency list representation"
                f"from file '{adj_list_path}'."
            ) from e

        adj_list_stem = ".".join(adj_list_path.name.split(".")[:-3])
        adj_list_suffix = "".join(adj_list_path.suffixes[-3:])

        orig_graph_contains_k33 = False
        u = -1
        for (  # pylint: disable=too-many-nested-blocks
            adj_list
        ) in orig_graph.graph_adj_list_repr:
            u += 1
            adj_list_index = -1
            for v in adj_list:
                adj_list_index += 1
                if v > u:
                    orig_graph_minus_edge = deepcopy(orig_graph)
                    try:
                        orig_graph_minus_edge.delete_edge(u, v)
                    except EdgeDeletionAnalysisError as e:
                        raise EdgeDeletionAnalysisError(
                            f"Unable to delete edge {{{u}, {v}}} from copied "
                            "graph."
                        ) from e

                    orig_graph_minus_edge_path = Path.joinpath(
                        adj_list_path.parent,
                        f"{adj_list_stem}.rem{u}-{v}" f"{adj_list_suffix}",
                    )
                    with open(
                        orig_graph_minus_edge_path, "w", encoding="utf-8"
                    ) as outfile:
                        outfile.write(str(orig_graph_minus_edge))

                    try:
                        obstruction_name = self._run_planarity(
                            orig_graph_minus_edge_path, output_dir
                        )
                    except EdgeDeletionAnalysisError as e:
                        raise EdgeDeletionAnalysisError(
                            "Failed to run planarity on "
                            f"'{orig_graph_minus_edge_path}'."
                        ) from e

                    if obstruction_name:
                        try:
                            obstruction_type = self.determine_obstruction_type(
                                obstruction_name, output_dir
                            )
                        except EdgeDeletionAnalysisError as e:
                            raise EdgeDeletionAnalysisError(
                                "Failed to determine obstruction type of "
                                "graph produced by removing edge "
                                f"{{{u}, {v}}} from the original graph."
                            ) from e
                        planar_obstruction_path = Path.joinpath(
                            output_dir, obstruction_name
                        )
                        if obstruction_type == "K33":
                            self.logger.info(
                                "'%s' contains a "
                                "K_{3, 3} that was not found by K_{3, 3} "
                                "search; see '%s'.",
                                orig_graph_minus_edge_path,
                                planar_obstruction_path,
                            )
                            orig_graph_contains_k33 = True
                        elif obstruction_type == "K5":
                            self.logger.info(
                                "'%s' contains a "
                                "subgraph homeomorphic to K_5; see "
                                "'%s'.",
                                orig_graph_minus_edge_path,
                                planar_obstruction_path,
                            )
                            orig_graph_minus_edge_contains_k33 = (
                                self._run_k33_search(
                                    orig_graph_minus_edge_path
                                )
                            )
                            if orig_graph_minus_edge_contains_k33:
                                self.logger.info(
                                    "'%s' contains "
                                    "a K_{3, 3} that was not found by "
                                    "K_{3, 3} search; see "
                                    "'%s'.",
                                    orig_graph_minus_edge_path,
                                    planar_obstruction_path,
                                )
                                orig_graph_contains_k33 = True
                            else:
                                self.logger.info(
                                    "'%s' doesn't "
                                    "appear to contain a K_{3, 3}.",
                                    orig_graph_minus_edge_path,
                                )

        return orig_graph_contains_k33


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage="python %(prog)s [options]",
        description="Edge deletion analysis tool\n\n"
        "When given an input file, determine the type of input. If "
        "the file is a not a .g6 file, an error is raised. Otherwise, "
        "iterate over each line and create a separate .g6 file:\n"
        "\t{input_file.stem}.{line_num}.g6\n"
        "And perform the following steps on each graph:\n"
        "1. Runs\n"
        "\tplanarity -t -ta {input_file} {infile_stem}.AdjList.out"
        ".txt\n"
        "to transform the graph to adjacency list\n"
        "2. Runs\n"
        "\tplanarity -s -3 {infile_stem}.AdjList.out.txt"
        "{infile_stem}.AdjList.s.3.out.txt\n"
        "and report whether a subgraph homeomorphic to K_{3, 3} was "
        "found (NONEMBEDDABLE) or not.\n"
        "3. If no subgraph homeomorphic to K_{3, 3} was found (i.e. "
        "previous test yielded OK), run\n"
        "\tplanarity -s -p {infile_stem}.AdjList.out.txt "
        "{infile_stem}.AdjList.s.p.out.txt {infile_stem}.AdjList.s.p."
        "obstruction.txt\n"
        "\ta. If the graph is reported as planar, then we can be sure "
        "no K_{3, 3} exists in the graph and execution terminates.\n"
        "\tb. If the graph was reported as nonplanar, examine the "
        "obstruction in \n"
        "\t\t{input_file}.AdjList.s.p.obstruction.txt:\n"
        "\t\ti. If the obstruction is homeomorphic to K_{3, 3}, then "
        "report that the  original graph contains a subgraph "
        "homeomorphic to K_{3, 3} that was not found by the K_{3, 3} "
        "search\n"
        "\t\tii. If the obstruction is homeomorphic to K_5, then "
        "perform the edge-deletion analysis to determine whether the "
        "graph doesn't contain a K_{3, 3} (with high confidence).",
    )
    parser.add_argument(
        "-p",
        "--planaritypath",
        type=Path,
        required=True,
        help="Path to planarity executable",
        metavar="PATH_TO_PLANARITY_EXECUTABLE",
    )
    parser.add_argument(
        "-i",
        "--inputfile",
        type=Path,
        required=True,
        help="Path to graph(s) to analyze",
        metavar="PATH_TO_GRAPH(s)",
    )
    parser.add_argument(
        "-o",
        "--outputdir",
        type=Path,
        default=None,
        metavar="OUTPUT_DIR_PATH",
        help="Parent directory under which subdirectory named after "
        "{infile_stem} will be created for output results. If none "
        "provided, defaults to:\n"
        "\tTestSupport/results/edge_deletion_analysis/{infile_stem}",
    )

    args = parser.parse_args()

    eda = EdgeDeletionAnalyzer(
        planarity_path=args.planaritypath,
        infile_path=args.inputfile,
        output_dir=args.outputdir,
    )
    eda.analyze_graphs()

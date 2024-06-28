"""Run leaks on MacOS to test for memory issues, e.g. leaks or overruns
"""

#!/usr/bin/env python

__all__ = []

import argparse
from datetime import datetime
import json
import logging
import multiprocessing
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
from typing import Optional

from planaritytesting_utils import (
    PLANARITY_ALGORITHM_SPECIFIERS,
    determine_input_filetype,
    is_path_to_executable,
)


class PlanarityLeaksAnalysisError(Exception):
    """
    Custom exception for representing errors that arise when performing memory
    checks on a planarity executable.
    """

    def __init__(self, message):
        super().__init__(message)
        self.message = message


class PlanarityLeaksAnalyzer:
    """Driver for planarity memory leak analysis"""

    def __init__(
        self,
        perform_full_analysis: bool = False,
        planarity_path: Optional[Path] = None,
        infile_path: Optional[Path] = None,
        output_dir: Optional[Path] = None,
    ) -> None:
        """TODO: WRITE DOCSTRING LOLS
        Raises:
            argparse.ArgumentTypeError:
            OSError: If run on any platform other than MacOS
        """
        if platform.system() != "Darwin":
            raise OSError("This utility is only for use on MacOS.")

        if not shutil.which("leaks"):
            raise OSError(
                "leaks is not installed; please install leaks and ensure your "
                "path variable contains the directory to which it is "
                "installed."
            )

        planarity_project_root = (
            Path(sys.argv[0]).resolve().parent.parent.parent
        )
        if not planarity_path:
            planarity_path = Path.joinpath(
                planarity_project_root, "Release", "planarity"
            )

        if not is_path_to_executable(planarity_path):
            raise argparse.ArgumentTypeError(
                f"'Path for planarity executable {planarity_path}' doesn't "
                "correspond to an executable."
            )

        self.curr_timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

        if not output_dir:
            output_dir = Path.joinpath(
                planarity_project_root,
                "TestSupport",
                "results",
                "planarity_leaks_analysis",
            )

        if output_dir.is_file():
            raise argparse.ArgumentTypeError(
                f"Path '{output_dir}' corresponds to a file, and can't be "
                "used as the output directory."
            )

        output_dir = Path.joinpath(output_dir, f"{self.curr_timestamp}")

        if Path.is_dir(output_dir):
            shutil.rmtree(output_dir)

        Path.mkdir(output_dir, parents=True, exist_ok=True)

        if infile_path:
            try:
                file_type = determine_input_filetype(infile_path)
            except ValueError as input_filetype_error:
                shutil.rmtree(output_dir)
                raise argparse.ArgumentTypeError(
                    "Failed to determine input filetype of "
                    f"'{infile_path}'."
                ) from input_filetype_error

            if file_type != "G6":
                shutil.rmtree(output_dir)
                raise argparse.ArgumentTypeError(
                    f"Determined '{infile_path}' has filetype '{file_type}', "
                    "which is not supported; please supply a .g6 file."
                )

            infile_copy_path = Path.joinpath(
                output_dir, infile_path.name
            ).resolve()

            shutil.copyfile(infile_path, infile_copy_path)
        else:
            infile_copy_path = None

        self.planarity_path = planarity_path
        self.infile_path = infile_copy_path
        self.output_dir = output_dir

        self._setup_logger()
        self._leaks_environment_variables = (
            self._setup_leaks_environment_variables(perform_full_analysis)
        )

    def _setup_logger(self) -> None:
        """Set up logger instance for PlanarityLeaksAnalyzer"""

        log_path = Path.joinpath(
            self.output_dir,
            f"edge_deletion_analysis-{self.curr_timestamp}.log",
        )

        # logging.getLogger() returns the *same instance* of a logger
        # when given the same name. In order to prevent this, must either give
        # a unique name, or must prevent adding additional handlers to the
        # logger
        self.logger = logging.getLogger(__name__ + self.curr_timestamp)
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

    def _setup_leaks_environment_variables(
        self, perform_full_analysis: bool = False
    ) -> dict[str, str]:
        """Sets up environment variables to configure granularity of leaks runs

        Args:
            perform_full_analysis: bool to indicate whether additional
                variables should be set for the leaks run
        """
        leaks_environment_variables = os.environ.copy()
        leaks_environment_variables["MallocStackLogging"] = "1"
        if perform_full_analysis:
            leaks_environment_variables = dict(
                leaks_environment_variables,
                **{
                    "MallocStackLoggingNoCompact": "1",
                    "MallocScribble": "1",
                    "MallocPreScribble": "1",
                    "MallocGuardEdges": "1",
                    "MallocCheckHeapStart": "1",
                    "MallocCheckHeapEach": "100",
                },
            )
        self.logger.info(
            "Configured leaks environment variables: %s",
            json.dumps(leaks_environment_variables, indent=4),
        )
        return leaks_environment_variables

    def _run_leaks(
        self, command_args: list[str], outfile_basename: Path
    ) -> None:
        """Run leaks utility for given args

        Args:
            command_args: List of strings comprised of executable name and
                command line arguments
            outfile_name: name of file to which you wish to write leaks output
        """
        stdout_outfile_path = outfile_basename.with_suffix(
            outfile_basename.suffix + ".stdout.txt"
        )
        stderr_outfile_path = outfile_basename.with_suffix(
            outfile_basename.suffix + ".stderr.txt"
        )
        full_args = ["leaks", "-atExit", "--"] + command_args
        with open(
            stdout_outfile_path, "w", encoding="utf-8"
        ) as stdout_outfile, open(
            stderr_outfile_path, "w", encoding="utf-8"
        ) as stderr_outfile:
            # running leaks processes hang if I set stderr=subprocess.PIPE, so
            # instead of routing stderr to subprocess.STDOUT, might as well
            # capture stderr in a separate file.
            subprocess.run(
                full_args,
                stdout=stdout_outfile,
                stderr=stderr_outfile,
                env=self._leaks_environment_variables,
                check=False,
            )

    def test_random_graphs(
        self, num_graphs_to_generate: int, order: int
    ) -> None:
        """Check RandomGraphs() for memory issues
        'planarity -r [-q] C K N': Random graphs

        Args:
            num_graphs_to_generate: Number of graphs to randomly generate
            order: Number of vertices in each randomly generated graph
        """

        test_random_graphs_args = [
            (command, num_graphs_to_generate, order)
            for command in PLANARITY_ALGORITHM_SPECIFIERS()
        ]
        with multiprocessing.Pool(
            processes=multiprocessing.cpu_count()
        ) as pool:
            _ = pool.starmap_async(
                self._test_random_graphs, test_random_graphs_args
            )
            pool.close()
            pool.join()

    def _test_random_graphs(
        self, command: str, num_graphs_to_generate: int, order: int
    ) -> None:
        """Runs RandomGraphs() for a given algorithm command specifier

        Args:
            num_graphs_to_generate: Number of graphs to randomly generate
            order: Number of vertices in each randomly generated graph
        """
        random_graphs_args = [
            f"{self.planarity_path}",
            "-r",
            f"-{command}",
            f"{num_graphs_to_generate}",
            f"{order}",
        ]
        random_graphs_outfile_parent_dir = Path.joinpath(
            self.output_dir,
            "RandomGraphs",
        )
        Path.mkdir(
            random_graphs_outfile_parent_dir, parents=True, exist_ok=True
        )
        random_graphs_outfile_basename = Path.joinpath(
            random_graphs_outfile_parent_dir,
            f"RandomGraphs.{command}.{num_graphs_to_generate}.{order}",
        )
        self._run_leaks(random_graphs_args, random_graphs_outfile_basename)

    def test_max_planar_graph_generator(self) -> None:
        """Check RandomGraph for memory issues
        'planarity -rm [-q] N O [O2]': Maximal planar random graph
        """


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage="python %(prog)s [options]",
        description="Planarity leaks analysis\n"
        "Automates runs of leaks on MacOS to identify memory issues for the "
        "following:\n"
        "- Random graphs with one small graph,\n"
        "- Max planar graph generator,\n"
        "If a .g6 infile containing one or more graphs is provided, will "
        "run:\n"
        "- Specific Graph (will only run on first graph in input file)\n"
        "- Test All Graphs\n"
        "For all algorithm command specifiers (i.e. {P, D, O, 2, 3, 4}).\n"
        "Additionally, if a .g6 infile is provided, will test the Graph "
        "Transformation tool (will only transform the first graph in"
        "the file)\n\n"
        "If an output directory is specified, a subdirectory will be created "
        "to contain the results:\n"
        "\t{output_dir}/{test_timestamp}/\n"
        "If an output directory is not specified, defaults to:\n"
        "\tTestSupport/results/planarity_leaks_analysis/{test_timestamp}/\n",
    )
    parser.add_argument(
        "-f",
        "--fullanalysis",
        action="store_true",
        help="Determines whether full memory check will be performed. N.B. "
        "This is significantly slower than only checking for leaks.",
    )
    parser.add_argument(
        "-p",
        "--planaritypath",
        type=Path,
        default=None,
        help="Must be a path to a planarity executable that was compiled "
        "with -g; defaults to:\n"
        "\t{planarity_project_root}/Release/planarity",
        metavar="PATH_TO_PLANARITY_EXECUTABLE",
    )
    parser.add_argument(
        "-i",
        "--inputfile",
        type=Path,
        default=None,
        metavar="PATH_TO_G6_INFILE",
        help="Optional path to .g6 infile which we wish to use to profile "
        "memory issues with Specific Graph, Test All Graphs, and Transform "
        "Graph.",
    )
    parser.add_argument(
        "-o",
        "--outputdir",
        type=Path,
        default=None,
        metavar="DIR_FOR_RESULTS",
        help="If no output directory provided, defaults to\n"
        "\tTestSupport/results/planarity_leaks_analysis/",
    )

    args = parser.parse_args()

    planarity_leaks_analyzer = PlanarityLeaksAnalyzer(
        perform_full_analysis=args.fullanalysis,
        planarity_path=args.planaritypath,
        infile_path=args.inputfile,
        output_dir=args.outputdir,
    )

    planarity_leaks_analyzer.test_random_graphs(50, 6)

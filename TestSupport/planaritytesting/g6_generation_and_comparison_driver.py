#!/usr/bin/env python

__all__ = []

import sys
import os
import shutil
import argparse
from pathlib import Path
import subprocess

# FIXME: Remove; only used for temporary examination of planarity output
from pprint import PrettyPrinter

from graph_generation_orchestrator import distribute_geng_workload
from planarity_testAllGraphs_orchestrator import (
    distribute_planarity_testAllGraphs_workload
)
from g6_diff_finder import G6DiffFinder, G6DiffFinderException
from planarity_constants import (
    PLANARITY_ALGORITHM_SPECIFIERS,
    max_num_edges_for_order
)
from planarity_testAllGraphs_output_parsing import process_file_contents


class G6GenerationAndComparisonDriver:
    """
    """
    def __init__(
            self, geng_path: Path, planarity_path: Path,
            planarity_backup_path: Path, output_parent_dir: Path,
            orders: tuple
        ):
        try:
            geng_path, planarity_path, planarity_backup_path, \
            output_parent_dir, orders = self._validate_and_normalize_input(
                geng_path, planarity_path, planarity_backup_path,
                output_parent_dir, orders
            )
        except argparse.ArgumentError as e:
            raise argparse.ArgumentError(
                'Invalid parameters; unable to proceed with generation and '
                'comparison.'
            )
        
        self.geng_path = geng_path
        self.planarity_path = planarity_path
        self.planarity_backup_path = planarity_backup_path
        self.output_parent_dir = output_parent_dir
        self.orders = orders

    def _validate_and_normalize_input(
            self, geng_path: Path, planarity_path: Path,
            planarity_backup_path: Path, output_parent_dir: Path,
            orders: tuple
        )->tuple[Path, Path, Path, Path, tuple]:
        """Validates G6GenerationAndComparisonDriver initialization values

        Args:
            geng_path: Path to geng executable
            planarity_path: Path to planarity executable
            planarity_backup_path: Path to planarity-backup executable, or None
            output_parent_dir: Path to directory to which you wish to output
                .g6 files and comparison results
            orders: Tuple containing orders of graphs for which you wish to
                generate the .g6 files

        Raises:
            argparse.ArgumentTypeError: If any of the arguments were invalid
        """
        if (
                not geng_path or
                not isinstance(geng_path, Path) or
                not shutil.which(str(geng_path.resolve()))
            ):
            raise argparse.ArgumentTypeError(
                f"Path for geng executable '{geng_path}' does not correspond "
                "to an executable."
            )
        
        if (
                not planarity_path
                or not isinstance(planarity_path, Path)
                or not shutil.which(str(planarity_path.resolve()))
            ):
            raise argparse.ArgumentTypeError(
                f"Path for planarity executable '{planarity_path}' does not "
                "correspond to an executable."
            )
        
        if (
                planarity_backup_path 
                and not isinstance(planarity_backup_path, Path)
                and not shutil.which(str(planarity_backup_path.resolve()))
            ):
            raise argparse.ArgumentTypeError(
                "Path for planarity_backup executable "
                f"'{planarity_backup_path}' does not correspond to an "
                "executable."
            )

        if not orders:
            raise argparse.ArgumentTypeError("No graph orders given.")
        
        if any(
                order for order in orders
                if order < 2
                if order > 12
                if not isinstance(order, int)
            ):
            raise argparse.ArgumentTypeError(
                "Desired graph orders must be integers between 2 an 12 "
                "inclusive."
            )

        if not output_parent_dir:
            script_entrypoint = Path(sys.argv[0]).resolve().parent.parent
            output_parent_dir = Path.joinpath(
                script_entrypoint, 'results',
                'g6_generation_and_comparison_driver'
            )
            Path.mkdir(output_parent_dir, parents=True, exist_ok=True)
            for order in orders:
                candidate_output_dir = Path.joinpath(
                    output_parent_dir, f"{order}"
                )
                if Path.is_dir(candidate_output_dir):
                    shutil.rmtree(candidate_output_dir)

        if (
            not isinstance(output_parent_dir, Path) or
            not output_parent_dir.is_dir()
            ):
            raise argparse.ArgumentTypeError(
                "Output directory path is invalid."
            )
        
        return geng_path, planarity_path, planarity_backup_path, \
            output_parent_dir, orders

    def generate_g6_files(self):
        for order in self.orders:
            g6_output_dir_for_order = Path.joinpath(
                self.output_parent_dir, f"{order}"
            )
            self._generate_geng_g6_files_for_order(
                order, g6_output_dir_for_order
            )

            if self.planarity_backup_path:
                for num_edges in range(max_num_edges_for_order(order) + 1):
                    self._generate_makeg_g6_files_for_order_and_num_edges(
                        order, num_edges
                    )

    def _generate_geng_g6_files_for_order(
            self, order: int, geng_g6_output_dir_for_order: Path
        ):
        distribute_geng_workload(
            geng_path=self.geng_path, canonical_files=False, order=order,
            output_dir=geng_g6_output_dir_for_order
        )
        
        distribute_geng_workload(
            geng_path=geng_path, canonical_files=True, order=order,
            output_dir=geng_g6_output_dir_for_order
        )

    def _generate_makeg_g6_files_for_order_and_num_edges(
            self, order: int, num_edges: int, command: str = '3'
        ):
        g6_output_dir_for_order_and_edgecount = Path.joinpath(
            self.output_parent_dir, f"{order}"
        )
        Path.mkdir(
            g6_output_dir_for_order_and_edgecount, parents=True, exist_ok=True
        )
        
        planarity_backup_outfile_dir = Path.joinpath(
            self.output_parent_dir, f"{order}", 'results', f"{command}",
            f"{num_edges}"
        )
        Path.mkdir(planarity_backup_outfile_dir, parents=True, exist_ok=True)

        self._generate_makeg_g6_file_for_order_and_num_edges(
            g6_output_dir_for_order_and_edgecount,
            planarity_backup_outfile_dir,
            order, num_edges, False, command
        )
        self._generate_makeg_g6_file_for_order_and_num_edges(
            g6_output_dir_for_order_and_edgecount,
            planarity_backup_outfile_dir,
            order, num_edges, True, command
        )
    
    def _generate_makeg_g6_file_for_order_and_num_edges(
            self, g6_output_dir_for_order_and_edgecount: Path,
            planarity_backup_outfile_dir: Path,
            order: int, num_edges: int, canonical_files:bool,
            command: str = '3'
        ):
        canonical_ext = '.canonical' if canonical_files else ''
        planarity_backup_outfile_name = Path.joinpath(
            planarity_backup_outfile_dir,
            f"planarity-backup-output_n{order}.m{num_edges}." +
            f"makeg{canonical_ext}.{command}.out.txt"
        )
        with (
                open(planarity_backup_outfile_name, "w")
                as makeg_outfile
            ):
            planarity_backup_args = [
                    f"{planarity_backup_path}", '-gen',
                    f"{g6_output_dir_for_order_and_edgecount}", f"-{command}",
                    f"{order}", f"{num_edges}", f"{num_edges}"
            ]
            if canonical_files:
                planarity_backup_args.insert(4, '-l')
            subprocess.run(
                planarity_backup_args,
                stdout=makeg_outfile, stderr=subprocess.PIPE
            )

    def run_planarity(self):
        for order in self.orders:
            geng_g6_output_dir_for_order = Path.joinpath(
                self.output_parent_dir, f"{order}"
            )
            planarity_output_dir_for_order = Path.joinpath(
                self.output_parent_dir, 'results', f"{order}"
            )
            self._run_planarity_on_g6_files_for_order(
                order, geng_g6_output_dir_for_order,
                planarity_output_dir_for_order,
                makeg_g6=False
            )

            if self.planarity_backup_path:
                self._run_planarity_on_g6_files_for_order(
                    order, geng_g6_output_dir_for_order,
                    planarity_output_dir_for_order,
                    makeg_g6=True
                )

    def _run_planarity_on_g6_files_for_order(
            self, order: int, g6_output_dir_for_order: Path,
            planarity_output_dir_for_order: Path, makeg_g6:bool
        ):
        distribute_planarity_testAllGraphs_workload(
            planarity_path=self.planarity_path, canonical_files=False,
            makeg_g6=makeg_g6, order=order,
            input_dir=g6_output_dir_for_order,
            output_dir=planarity_output_dir_for_order
        )
        
        distribute_planarity_testAllGraphs_workload(
            planarity_path=self.planarity_path, canonical_files=True,
            makeg_g6=makeg_g6, order=order,
            input_dir=g6_output_dir_for_order,
            output_dir=planarity_output_dir_for_order
        )
    
    def reorganize_files(self):
        for order in self.orders:
            orig_geng_g6_output_dir_for_order = Path.joinpath(
                    self.output_parent_dir, f"{order}"
            )
            for num_edges in range(max_num_edges_for_order(order) + 1):
                self._move_g6_files(
                    order, num_edges, orig_geng_g6_output_dir_for_order
                )
                self._move_planarity_output_files(
                    order, num_edges, orig_geng_g6_output_dir_for_order
                )

        shutil.rmtree(Path.joinpath(self.output_parent_dir, 'results'))

    def _move_g6_files(
            self, order: int, num_edges:int,
            g6_output_dir_for_order: Path
        ):
        g6_output_dir_for_order_and_edgecount = Path.joinpath(
            g6_output_dir_for_order, "graphs", f"{num_edges}"
        )

        Path.mkdir(
            g6_output_dir_for_order_and_edgecount,
            parents=True, exist_ok=True
        )

        geng_g6_outfile_name = f"n{order}.m{num_edges}.g6"
        self._move_file(
            g6_output_dir_for_order, geng_g6_outfile_name,
            g6_output_dir_for_order_and_edgecount
        )

        geng_canonical_g6_outfile_name = \
            f"n{order}.m{num_edges}.canonical.g6"
        self._move_file(
            g6_output_dir_for_order, geng_canonical_g6_outfile_name,
            g6_output_dir_for_order_and_edgecount
        )

        makeg_g6_outfile_name = \
            f"n{order}.m{num_edges}.makeg.g6"
        self._move_file(
            g6_output_dir_for_order, makeg_g6_outfile_name,
            g6_output_dir_for_order_and_edgecount
        )

        makeg_canonical_g6_outfile_name = \
            f"n{order}.m{num_edges}.makeg.canonical.g6"
        self._move_file(
            g6_output_dir_for_order, makeg_canonical_g6_outfile_name,
            g6_output_dir_for_order_and_edgecount
        )

    def _move_planarity_output_files(
            self, order: int, num_edges:int,
            new_output_dir_for_order: Path
        ):
        for command in PLANARITY_ALGORITHM_SPECIFIERS():
            orig_planarity_output_dir = Path.joinpath(
                self.output_parent_dir, "results", f"{order}", f"{command}"
            )
            new_planarity_output_dir_for_order_and_edgecount = Path.joinpath(
                new_output_dir_for_order, "results", f"{command}", f"{num_edges}"
            )
            
            Path.mkdir(
                new_planarity_output_dir_for_order_and_edgecount,
                parents=True, exist_ok=True
            )

            g6_planarity_outfile_name = \
                f"n{order}.m{num_edges}.{command}.out.txt"
            self._move_file(
                orig_planarity_output_dir, g6_planarity_outfile_name,
                new_planarity_output_dir_for_order_and_edgecount
            )

            canonical_g6_planarity_outfile_name = \
                f"n{order}.m{num_edges}.canonical.{command}.out.txt"
            self._move_file(
                orig_planarity_output_dir, canonical_g6_planarity_outfile_name,
                new_planarity_output_dir_for_order_and_edgecount
            )

            makeg_g6_planarity_outfile_name = \
                f"n{order}.m{num_edges}.makeg.{command}.out.txt"
            self._move_file(
                orig_planarity_output_dir, makeg_g6_planarity_outfile_name,
                new_planarity_output_dir_for_order_and_edgecount
            )

            canonical_makeg_g6_planarity_outfile_name = \
                f"n{order}.m{num_edges}.makeg.canonical.{command}.out.txt"
            self._move_file(
                orig_planarity_output_dir,
                canonical_makeg_g6_planarity_outfile_name,
                new_planarity_output_dir_for_order_and_edgecount
            )
    
    def _move_file(
            self, src_dir: Path, outfile_name: str, dest_dir: Path
        ):
        src_path = Path.joinpath(src_dir, outfile_name)
        dest_path = Path.joinpath(dest_dir, outfile_name)
        if Path.is_file(dest_path):
            os.remove(dest_path)
        
        shutil.move(
            src_path,
            dest_dir
        )

    def identify_planarity_result_discrepancies(self):
        self._planarity_results = {}
        for order in self.orders:
            self._planarity_results[order] = {}
            for num_edges in range(max_num_edges_for_order(order) + 1):
                self._planarity_results[order][num_edges] = {}
                for command in PLANARITY_ALGORITHM_SPECIFIERS():
                    self._planarity_results[order][num_edges][command] = {}

                    planarity_output_dir = Path.joinpath(
                        self.output_parent_dir, f"{order}", "results",
                        f"{command}", f"{num_edges}"
                    )

                    geng_g6_output = Path.joinpath(
                        planarity_output_dir,
                        f"n{order}.m{num_edges}.{command}.out.txt"
                    )
                    self._extract_planarity_results(
                        geng_g6_output, order, num_edges, command, 'geng_g6'
                    )

                    geng_canonical_g6_output = Path.joinpath(
                        planarity_output_dir,
                        f"n{order}.m{num_edges}.canonical.{command}.out.txt"
                    )
                    self._extract_planarity_results(
                        geng_canonical_g6_output, order, num_edges, command,
                        'geng_canonical_g6'
                    )

                    makeg_g6_output = Path.joinpath(
                        planarity_output_dir,
                        f"n{order}.m{num_edges}.makeg.{command}.out.txt"
                    )
                    self._extract_planarity_results(
                        makeg_g6_output, order, num_edges, command, 'makeg_g6'
                    )

                    makeg_canonical_g6_output = Path.joinpath(
                        planarity_output_dir,
                        f"n{order}.m{num_edges}.makeg.canonical.{command}.out.txt"
                    )
                    self._extract_planarity_results(
                        makeg_canonical_g6_output, order, num_edges, command,
                        'makeg_canonical_g6'
                    )

                    if (
                        self._planarity_results[order][num_edges][command]['geng_g6'] == 
                        self._planarity_results[order][num_edges][command]['makeg_g6'] ==
                        self._planarity_results[order][num_edges][command]['geng_canonical_g6'] ==
                        self._planarity_results[order][num_edges][command]['makeg_canonical_g6']
                    ):
                        del(self._planarity_results[order][num_edges][command])
                
                if not self._planarity_results[order][num_edges]:
                    del(self._planarity_results[order][num_edges])
        # FIXME: Remove; only used for temporary examination of output
        PrettyPrinter(indent=4).pprint(self._planarity_results)

    def _extract_planarity_results(
            self, planarity_outfile: str, order: int, num_edges: int,
            command: str, file_type: str
        ):
        _, _, numGraphs_from_file, numOK_from_file, \
            numNONEMBEDDABLE_from_file, _ = \
                process_file_contents(planarity_outfile, command)

        self._planarity_results[order][num_edges][command][file_type] = {
            'numGraphs': numGraphs_from_file,
            'numOK': numOK_from_file,
            'numNONEMBEDDABLE': numNONEMBEDDABLE_from_file,
        }

    def get_all_diffs(self):
        for order in self.orders:
            output_dir_for_order = Path.joinpath(
                self.output_parent_dir, f"{order}"
            )

            log_dir_for_order = Path.joinpath(
                output_dir_for_order,
                'results',
                'difflogs',
            )

            Path.mkdir(log_dir_for_order, parents=True, exist_ok=True)

            log_path_for_geng_g6_vs_geng_canonical_g6 = Path.joinpath(
                log_dir_for_order,
                f"G6DiffFinder.n{order}.geng_vs_geng-canonical.log"
            )
            log_path_for_makeg_g6_vs_makeg_canonical_g6 = Path.joinpath(
                log_dir_for_order,
                f"G6DiffFinder.n{order}.makeg_vs_makeg-canonical.log"
            )
            log_path_for_geng_g6_vs_makeg_g6 = Path.joinpath(
                log_dir_for_order,
                f"G6DiffFinder.n{order}.geng_vs_makeg.log"
            )
            log_path_for_geng_canonical_g6_vs_makeg_canonical_g6 = \
                Path.joinpath(
                    log_dir_for_order,
                    f"G6DiffFinder.n{order}."
                    +"geng-canonical_vs_makeg-canonical.log"
                )
            log_path_for_geng_g6_vs_makeg_canonical_g6 = Path.joinpath(
                log_dir_for_order,
                f"G6DiffFinder.n{order}.geng_vs_makeg-canonical.log"
            )

            for num_edges in range(max_num_edges_for_order(order) + 1):
                g6_files_for_order_and_edgecount = Path.joinpath(
                    output_dir_for_order,
                    'graphs',
                    f"{num_edges}"
                )
                geng_g6_path = Path.joinpath(
                    g6_files_for_order_and_edgecount,
                    f"n{order}.m{num_edges}.g6"
                )
                geng_canonical_g6_path = Path.joinpath(
                    g6_files_for_order_and_edgecount,
                    f"n{order}.m{num_edges}.canonical.g6"
                )
                makeg_g6_path = Path.joinpath(
                    g6_files_for_order_and_edgecount,
                    f"n{order}.m{num_edges}.makeg.g6"
                )
                makeg_canonical_g6_path = Path.joinpath(
                    g6_files_for_order_and_edgecount,
                    f"n{order}.m{num_edges}.makeg.canonical.g6"
                )

                self._get_diffs(
                    geng_g6_path, geng_canonical_g6_path,
                    log_path_for_geng_g6_vs_geng_canonical_g6
                )

                if self.planarity_backup_path:
                    self._get_diffs(
                        geng_g6_path, makeg_g6_path,
                        log_path_for_geng_g6_vs_makeg_g6
                    )
                    self._get_diffs(
                        makeg_g6_path, makeg_canonical_g6_path,
                        log_path_for_makeg_g6_vs_makeg_canonical_g6
                    )
                    self._get_diffs(
                        geng_canonical_g6_path, makeg_canonical_g6_path,
                        log_path_for_geng_canonical_g6_vs_makeg_canonical_g6
                    )
                    self._get_diffs(
                        geng_g6_path, makeg_canonical_g6_path,
                        log_path_for_geng_g6_vs_makeg_canonical_g6
                    )

    def _get_diffs(
            self, first_comparand_infile: Path, second_comparand_infile: Path,
            log_path: Path
        ):
        try:
            g6_diff_finder = G6DiffFinder(
                first_comparand_infile, second_comparand_infile, log_path
            )
        except:
            raise G6DiffFinderException(
                "Unable to initialize G6DiffFinder with given input files: "
                f"'{first_comparand_infile}', '{second_comparand_infile}'"
            )

        try:
            g6_diff_finder.graph_set_diff()
        except:
            raise G6DiffFinderException(
                "Failed to discern diff between two .g6 input files."
            )


def parse_range(value: str)->tuple:
    """Parse a single integer or a range of integers.
    
    Args:
        value: A string of the form 'X[,Y]', i.e. either a single value for the
            desired order X, or an interval inclusive of the endpoints [X, Y]
    
    Returns:
        A tuple containing either a single element, X, or every integer from X
        up to and including Y
    """
    separator = ','
    if separator in value:
        if value.count(separator) > 1:
            raise argparse.ArgumentTypeError(
                f"Invalid range '{value}' contains multiple commas; range "
                "should be of the form 'X,Y'")
        
        start, end = value.split(separator)
        try:
            start, end = int(start), int(end)
        except ValueError:
            raise argparse.ArgumentTypeError(
                f"Invalid range '{value}': both start and end values must be "
                "integers.")
        if start > end:
            raise argparse.ArgumentTypeError(
                f"Invalid range '{value}': start value must not be greater "
                "than end value.")
        # Transforms to interval that includes endpoints: '5,8' corresponds to
        # the tuple (5, 6, 7, 8)
        return tuple(range(start, end + 1))
    else:
        try:
            return (int(value),)
        except ValueError:
            raise argparse.ArgumentTypeError(
                f"Invalid order specifier '{value}': should be a single "
                "integer 'X'")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage='python %(prog)s [options]',
        description="""G6 File Generation and Comparison Tool

For each graph order in the specified range, a child directory of the output
directory will be created named 'n{order}'. This directory will contain
subdirectories:
- graphs/
    - For each edge-count from 0 to (N * (N - 1)) / 2, a directory is created
    to contain:
    n{order}.m{num_edges}.g6
    n{order}.m{num_edges}.canonical.g6
    n{order}.m{num_edges}.makeg.g6 (req. planarity-backup path)
    n{order}.m{num_edges}.makeg.canonical.g6 (req. planarity-backup path)
    - results/ which will contain the diffs of these .g6 files, pairs of files:
        - graphs_in_{first_infile}_not_in_{second_infile}.g6
        - graphs_in_{second_infile}_not_in_{first_infile}.g6
    These pairs should contain the same number of graphs
- results/
    - For each graph algorithm command specifier, there will be a subdirectory
    containing one subdirectory for each edge-count (0 to (N * (N - 1)) / 2),
    which will contain files:
    n{order}.m{num_edges}.{command}.out.txt
    n{order}.m{num_edges}.canonical.{command}.out.txt
    n{order}.m{num_edges}.makeg.{command}.out.txt
    n{order}.m{num_edges}.makeg.canonical.{command}.out.txt
""")

    parser.add_argument(
        '-p', '--planaritypath',
        type=Path,
        required=True,
        help='Path to planarity executable',
        metavar='PATH_TO_PLANARITY_EXECUTABLE')
    parser.add_argument(
        '-g', '--gengpath',
        type=Path,
        required=True,
        help='Path to nauty geng executable',
        metavar='PATH_TO_GENG_EXECUTABLE')
    parser.add_argument(
        '-b', '--planaritybackuppath',
        type=Path,
        required=False,
        help='Path to planarity-backup executable (optional; not public!)',
        metavar='PATH_TO_PLANARITY_BACKUP_EXECUTABLE')
    parser.add_argument(
        '-n', '--orders',
        type=parse_range,
        required=True,
        help='Order(s) of graphs for which you wish to generate .g6 files',
        metavar='X[,Y]')
    parser.add_argument(
        '-o', '--outputdir',
        type=Path,
        default=None,
        metavar='OUTPUT_DIR_PATH',
        help="""Parent directory under which subdirectories will be created
for output results. If none provided, defaults to:
    TestSupport/results/g6_generation_and_comparison_driver"""
    )

    args = parser.parse_args()

    geng_path = args.gengpath
    planarity_path = args.planaritypath
    planarity_backup_path = args.planaritybackuppath
    output_parent_dir = args.outputdir
    orders = args.orders

    g6_generation_and_comparison_driver = G6GenerationAndComparisonDriver(
        geng_path=geng_path,
        planarity_path=planarity_path,
        planarity_backup_path=planarity_backup_path,
        output_parent_dir=output_parent_dir,
        orders=orders
    )

    g6_generation_and_comparison_driver.generate_g6_files()
    g6_generation_and_comparison_driver.run_planarity()
    g6_generation_and_comparison_driver.reorganize_files()
    g6_generation_and_comparison_driver.identify_planarity_result_discrepancies()
    # TODO: Might be interesting to only perform the diffs if the results of
    # planarity on particular .g6 vs. canonical file or makeg .g6 canonical vs
    # makeg .g6 differ
    g6_generation_and_comparison_driver.get_all_diffs()

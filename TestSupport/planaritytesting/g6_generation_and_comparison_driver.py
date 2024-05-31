#!/usr/bin/env python

__all__ = []

import os
import shutil
import argparse
from pathlib import Path
import subprocess

from graph_generation_orchestrator import distribute_geng_workload
from planarity_orchestrator import distribute_planarity_workload
from g6_diff_finder import G6DiffFinder, G6DiffFinderException

class G6GenerationAndComparisonDriver:
    """
    """
    def __init__(
            self, geng_path: Path, planarity_path: Path,
            planarity_backup_path: Path, output_parent_dir: Path,
            orders: tuple
        ):
        try:
            self._validate_input(
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

    def _validate_input(
            self, geng_path: Path, planarity_path: Path,
            planarity_backup_path: Path, output_parent_dir: Path,
            orders: tuple
        ):
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

        if (
            not output_parent_dir
            or not isinstance(output_parent_dir, Path)
            or output_parent_dir.is_file()
            ):
            raise argparse.ArgumentTypeError(
                "Output directory path is invalid."
            )

    def generate_g6_files(self):
        for order in self.orders:
            g6_output_dir_for_order = Path.joinpath(
                self.output_parent_dir, f"{order}"
            )
            self._generate_geng_g6_files_for_order(
                order, g6_output_dir_for_order
            )

            if self.planarity_backup_path:
                max_num_edges_for_order = (int)((order * (order - 1)) / 2) + 1
                for num_edges in range(max_num_edges_for_order):
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
            self.output_parent_dir, f"{order}", 'graphs', f"{num_edges}"
        )
        Path.mkdir(
            g6_output_dir_for_order_and_edgecount, parents=True, exist_ok=True
        )
        
        planarity_backup_outfile_dir = Path.joinpath(
            self.output_parent_dir, f"{order}", 'results', f"{command}",
            f"{num_edges}"
        )
        Path.mkdir(planarity_backup_outfile_dir, parents=True, exist_ok=True)
        
        planarity_backup_outfile_name = Path.joinpath(
            planarity_backup_outfile_dir,
            f"n{order}.m{num_edges}.makeg.{command}.out.txt"
        )
        with (
                open(planarity_backup_outfile_name, "w")
                as makeg_outfile
            ):
            subprocess.run(
                [
                    f"{planarity_backup_path}", '-gen',
                    f"{g6_output_dir_for_order_and_edgecount}", f"-{command}",
                    f"{order}", f"{num_edges}", f"{num_edges}"
                ],
                stdout=makeg_outfile, stderr=subprocess.PIPE
            )

        planarity_backup_canonical_outfile_name = Path.joinpath(
            planarity_backup_outfile_dir,
            f"n{order}.m{num_edges}.makeg.canonical.{command}.out.txt"
        )
        with (
                open(planarity_backup_canonical_outfile_name, "w")
                as canonical_makeg_outfile
            ):
            subprocess.run(
                [
                    f'{planarity_backup_path}', '-gen',
                    f"{g6_output_dir_for_order_and_edgecount}", f"-{command}",
                    '-l', f"{order}", f"{num_edges}", f"{num_edges}"
                ],
                stdout=canonical_makeg_outfile, stderr=subprocess.PIPE
            )

    def run_planarity(self):
        for order in self.orders:
            geng_g6_output_dir_for_order = Path.joinpath(
                self.output_parent_dir, f"{order}"
            )
            planarity_output_dir_for_order = Path.joinpath(
                self.output_parent_dir, 'results', f"{order}"
            )
            self._run_planarity_on_geng_g6_files_for_order(
                order, geng_g6_output_dir_for_order,
                planarity_output_dir_for_order
            )

    def _run_planarity_on_geng_g6_files_for_order(
            self, order: int, geng_g6_output_dir_for_order: Path,
            planarity_output_dir_for_order: Path
        ):
        distribute_planarity_workload(
            planarity_path=self.planarity_path, canonical_files=False,
            order=order, input_dir=geng_g6_output_dir_for_order,
            output_dir=planarity_output_dir_for_order
        )
        
        distribute_planarity_workload(
            planarity_path=self.planarity_path, canonical_files=True,
            order=order, input_dir=geng_g6_output_dir_for_order,
            output_dir=planarity_output_dir_for_order
        )
    
    def reorganize_files(self):
        for order in self.orders:
            orig_geng_g6_output_dir_for_order = Path.joinpath(
                    self.output_parent_dir, f"{order}"
            )
            max_num_edges_for_order = (int)((order * (order - 1)) / 2) + 1
            for num_edges in range(max_num_edges_for_order):
                self._move_geng_g6_files(
                    order, num_edges, orig_geng_g6_output_dir_for_order
                )
                self._move_planarity_output_files(
                    order, num_edges, orig_geng_g6_output_dir_for_order
                )

        shutil.rmtree(Path.joinpath(self.output_parent_dir, 'results'))

    def _move_geng_g6_files(
            self, order: int, num_edges:int,
            geng_g6_output_dir_for_order: Path
        ):
        geng_g6_output_dir_for_order_and_edgecount = Path.joinpath(
            geng_g6_output_dir_for_order, "graphs", f"{num_edges}"
        )
        Path.mkdir(
            geng_g6_output_dir_for_order_and_edgecount,
            parents=True, exist_ok=True
        )

        geng_outfile_path = Path.joinpath(
                geng_g6_output_dir_for_order,
                f"n{order}.m{num_edges}.g6"
        )
        
        geng_outfile_move_candidate_path = Path.joinpath(
                geng_g6_output_dir_for_order_and_edgecount,
                f"n{order}.m{num_edges}.g6"
        )
        if Path.is_file(geng_outfile_move_candidate_path):
            os.remove(geng_outfile_move_candidate_path)
        
        shutil.move(
            geng_outfile_path,
            geng_g6_output_dir_for_order_and_edgecount
        )
        
        geng_canonical_outfile_path = Path.joinpath(
                geng_g6_output_dir_for_order,
                f"n{order}.m{num_edges}.canonical.g6"
        )
        
        geng_canonical_move_candidate_path = Path.joinpath(
            geng_g6_output_dir_for_order_and_edgecount,
            f"n{order}.m{num_edges}.canonical.g6"
        )
        if Path.is_file(geng_canonical_move_candidate_path):
            os.remove(geng_canonical_move_candidate_path)
        
        shutil.move(
            geng_canonical_outfile_path,
            geng_g6_output_dir_for_order_and_edgecount
        )

    def _move_planarity_output_files(
            self, order: int, num_edges:int,
            new_output_dir_for_order: Path, command: str = '3'
        ):
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

        planarity_outfile_path = Path.joinpath(
                orig_planarity_output_dir,
                f"n{order}.m{num_edges}.{command}.out.txt"
        )
        
        planarity_outfile_move_candidate_path = Path.joinpath(
                new_planarity_output_dir_for_order_and_edgecount,
                f"n{order}.m{num_edges}.{command}.out.txt"
        )
        if Path.is_file(planarity_outfile_move_candidate_path):
            os.remove(planarity_outfile_move_candidate_path)
        
        shutil.move(
            planarity_outfile_path,
            new_planarity_output_dir_for_order_and_edgecount)
        
        planarity_canonical_outfile_path = Path.joinpath(
                orig_planarity_output_dir,
                f"n{order}.m{num_edges}.canonical.{command}.out.txt"
            )
        
        planarity_canonical_move_candidate_path = Path.joinpath(
            new_planarity_output_dir_for_order_and_edgecount,
            f"n{order}.m{num_edges}.canonical.{command}.out.txt")
        if Path.is_file(planarity_canonical_move_candidate_path):
            os.remove(planarity_canonical_move_candidate_path)
        
        shutil.move(
            planarity_canonical_outfile_path,
            new_planarity_output_dir_for_order_and_edgecount)

    def identify_planarity_result_discrepancies(self, command: str = '3'):
        pass

    def get_diffs(self):
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

            max_num_edges_for_order = (int)((order * (order - 1)) / 2) + 1
            for num_edges in range(max_num_edges_for_order):
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
                    geng_g6_path, makeg_g6_path,
                    log_path_for_geng_g6_vs_makeg_g6
                )
                self._get_diffs(
                    geng_g6_path, geng_canonical_g6_path,
                    log_path_for_geng_g6_vs_geng_canonical_g6
                )
                self._get_diffs(
                    makeg_g6_path, makeg_canonical_g6_path,
                    log_path_for_makeg_g6_vs_makeg_canonical_g6
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
        description="""G6 File Enumeration and Comparison Tool

For each graph order in the specified range, a child directory of the output
directory will be created named 'n{order}'. This directory will contain
one subdirectory per possible edge-count, i.e. from 1 to (N * (N - 1)) / 2.
These subdirectories will have paths:
    {output_dir}/n{order}/{num_edges}-{num_edges}
Upon successful execution, these directories will contain the following files:
- .g6 output files
    n{order}.m{num_edges}.g6
    n{order}.m{num_edges}.canonical.g6
    n{order}.m{num_edges}-{num_edges}.oldgeng.g6 (req. planarity-backup path)
    n{order}.m{num_edges}-{num_edges}.canonical.g6 (req. planarity-backup path)
- .g6 Comparison results
    - graphs_in_{first_infile}_not_in_{second_infile}.g6 - a valid .g6 file (no
    header) 
    - graphs_in_{second_infile}_not_in_{first_infile}.g6 - should have the same
    number of graphs as graphs_in_{first_infile}_not_in_{second_infile}.g6
    The results from comparing 

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
        default=Path(".").resolve(),
        help='Parent directory under which subdirectories will be created '\
            'for output results',
        metavar='OUTPUT_DIR_PATH')

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
    # TODO: Might be interesting to only perform the diffs if the results of
    # planarity on particular .g6 vs. canonical file or makeg .g6 canonical vs
    # makeg .g6 differ
    g6_generation_and_comparison_driver.get_diffs()

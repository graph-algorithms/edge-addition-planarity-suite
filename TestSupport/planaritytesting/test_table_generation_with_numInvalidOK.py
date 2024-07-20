"""Incorporate numInvalidOKs into Test Table

Classes:
    TestTableGenerationWithInvalidOKs

"""

__all__ = []

import argparse
from pathlib import Path
import sys
from typing import Optional

from edge_deletion_analysis import (
    EdgeDeletionAnalyzer,
)
from planaritytesting_utils import (
    parse_range,
    is_path_to_executable,
    PLANARITY_ALGORITHM_SPECIFIERS,
    EDGE_DELETION_ANALYSIS_SPECIFIERS,
    max_num_edges_for_order,
)
from test_table_generator import (
    TestTableGenerator,
)


class TestTableGenerationWithInvalidOKs:
    """
    Run edge-deletion analysis on all .g6 files for each each edge-count of
    each graph order for which we wish to generate the Test Tables.
    """

    def __init__(
        self,
        planarity_path: Optional[Path] = None,
        orders: Optional[tuple[int, ...]] = None,
        graph_dir: Optional[Path] = None,
        planarity_output_dir: Optional[Path] = None,
        test_table_output_dir: Optional[Path] = None,
        canonical_files: bool = False,
        makeg_files: bool = False,
    ) -> None:
        planarity_project_root = (
            Path(sys.argv[0]).resolve().parent.parent.parent
        )
        if not planarity_path:
            planarity_path = Path.joinpath(
                planarity_project_root, "Release", "planarity"
            )
        if not is_path_to_executable(planarity_path):
            raise argparse.ArgumentTypeError(
                f"Path for planarity executable '{planarity_path}' does not "
                "correspond to an executable."
            )

        if not orders:
            orders = tuple(range(6, 11))  # orders defaults to (6, 7, 8, 9, 10)

        self.test_support_dir = Path.joinpath(
            planarity_project_root, "TestSupport"
        )
        if not graph_dir:
            graph_dir = Path.joinpath(
                self.test_support_dir,
                "results",
                "graph_generation_orchestrator",
            )
        if graph_dir.is_file():
            raise argparse.ArgumentTypeError(
                f"graph_dir = '{graph_dir}' corresponds to a file."
            )
        if not any(graph_dir.iterdir()):
            raise argparse.ArgumentTypeError(
                f"graph_dir = '{graph_dir}' is empty."
            )

        if not planarity_output_dir:
            planarity_output_dir = Path.joinpath(
                self.test_support_dir,
                "results",
                "planarity_testAllGraphs_orchestrator",
            )
        if planarity_output_dir.is_file():
            raise argparse.ArgumentTypeError(
                f"planarity_output_dir = '{planarity_output_dir}' corresponds "
                "to a file."
            )
        if not any(planarity_output_dir.iterdir()):
            raise argparse.ArgumentTypeError(
                f"planarity_output_dir = '{planarity_output_dir}' is empty."
            )

        if not test_table_output_dir:
            test_table_output_dir = Path.joinpath(
                self.test_support_dir,
                "tables",
            )
        if test_table_output_dir.is_file():
            raise argparse.ArgumentTypeError(
                f"test_table_output_dir = '{test_table_output_dir}' "
                "corresponds to a file."
            )

        self.planarity_path = planarity_path
        self.orders = orders
        self.graph_dir = graph_dir
        self.planarity_output_dir = planarity_output_dir
        self.test_table_output_dir = test_table_output_dir

        self.canonical_files = canonical_files
        self.makeg_files = makeg_files

        self._numInvalidOKs = {}

    def determine_numInvalidOKs(
        self,
    ) -> None:
        """Get numInvalidOK for supported graph algorithm extensions"""
        canonical_ext = ".canonical" if self.canonical_files else ""
        makeg_ext = ".makeg" if self.makeg_files else ""
        for order in self.orders:
            graph_dir_for_order = Path.joinpath(
                self.graph_dir,
                f"{order}",
            )
            if not graph_dir_for_order.is_dir() or not any(
                graph_dir_for_order.iterdir()
            ):
                continue
            for num_edges in range(max_num_edges_for_order(order) + 1):
                g6_infile_path = Path.joinpath(
                    graph_dir_for_order,
                    f"n{order}.m{num_edges}{makeg_ext}{canonical_ext}.g6",
                )
                for command in EDGE_DELETION_ANALYSIS_SPECIFIERS():
                    self._call_edge_deletion_analysis(
                        order, command, g6_infile_path
                    )

    def _call_edge_deletion_analysis(
        self, order: int, command: str, infile_path: Path
    ) -> None:
        """For given command, get numInvalidOKs for each order and edge-count

        Args:
            command: string indicating graph search algorithm command specifier
                for which we wish to determine the number of invalid OKs using
                the edge-deletion analysis tool.
        """
        if not infile_path.exists():
            return
        eda = EdgeDeletionAnalyzer(
            planarity_path=self.planarity_path,
            infile_path=infile_path,
            output_dir=None,
            extension_choice=command,
        )

        numInvalidOK_for_infile = eda.analyze_graphs()
        infile_name = infile_path.name
        eda.logger.info(
            "NUMBER OF INVALID OKs in '%s': %d",
            infile_name,
            numInvalidOK_for_infile,
        )

        if not self._numInvalidOKs.get(order):
            self._numInvalidOKs[order] = {}
        if not self._numInvalidOKs[order].get(command):
            self._numInvalidOKs[order][command] = {}
        self._numInvalidOKs[order][command][
            infile_name
        ] = numInvalidOK_for_infile

    def generate_test_table_with_numInvalidOK(self) -> None:
        """Incorporate edge-deletion analysis results into Test Table"""
        for order in self.orders:
            input_dir_for_order = Path.joinpath(
                self.planarity_output_dir,
                f"{order}",
            )
            if not input_dir_for_order.is_dir() or not any(
                input_dir_for_order.iterdir()
            ):
                continue
            for command in PLANARITY_ALGORITHM_SPECIFIERS():
                input_dir_for_order_and_command = Path.joinpath(
                    input_dir_for_order, f"{command}"
                )
                if not input_dir_for_order_and_command.is_dir() or not any(
                    input_file_candidate
                    for input_file_candidate in input_dir_for_order_and_command.iterdir()
                    if (
                        self.canonical_files
                        == ("canonical" in input_file_candidate.name)
                    )
                    if (
                        self.makeg_files
                        == ("makeg" in input_file_candidate.name)
                    )
                ):
                    continue
                ttg = TestTableGenerator(
                    test_table_input_dir=input_dir_for_order_and_command,
                    test_table_output_dir=self.test_table_output_dir,
                    canonical_files=self.canonical_files,
                    makeg_files=self.makeg_files,
                    edge_deletion_analysis_results=self._numInvalidOKs.get(
                        order, {}
                    ).get(command, {}),
                )
                ttg.get_order_and_command_from_input_dir()
                ttg.process_files()
                ttg.write_table_formatted_data_to_file()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage="python %(prog)s [options]",
        description="Test Table Generation including numInvalidOKs\n\n",
    )
    parser.add_argument(
        "-p",
        "--planaritypath",
        type=Path,
        required=False,
        help="Path to planarity executable. Defaults to:\n"
        "\t{planarity_project_root}/Release/planarity",
        metavar="PATH_TO_PLANARITY_EXECUTABLE",
    )
    parser.add_argument(
        "-n",
        "--orders",
        type=parse_range,
        required=False,
        help="Order(s) of graphs for which you wish to generate Test Tables; "
        f"for commands {EDGE_DELETION_ANALYSIS_SPECIFIERS()}, will include "
        "numInvalidOKs derived by edge-deletion analysis.",
        metavar="X[,Y]",
    )
    parser.add_argument(
        "-g",
        "--graphdir",
        type=Path,
        required=False,
        help="Path to parent directory containing subdirectories of .g6 files."
        "Defaults to:\n"
        "\tTestSupport/results/graph_generation_orchestrator/\n",
        metavar="PATH_TO_GRAPH_PARENT_DIR",
    )
    parser.add_argument(
        "-t",
        "--planarityoutputdir",
        type=Path,
        required=False,
        help="Path to parent directory containing subdirectories of planarity "
        "Test All Graphs output for each graph order. Defaults to:\n"
        "\tTestSupport/results/planarity_testAllGraphs_orchestrator/\n"
        "The subdirectories are expected to be labelled with the graph order, "
        "and each of these subdirectories contains subdirectories for each "
        f"command {PLANARITY_ALGORITHM_SPECIFIERS()}, i.e.:\n"
        "TestSupport/results/planarity_testAllGraphs_orchestrator/{order}/"
        "{command}/n{order}.m{num_edges}(.makeg)?(.canonical)?.{command}.out"
        ".txt",
        metavar="PATH_TO_PLANARITY_OUTPUT",
    )
    parser.add_argument(
        "-o",
        "--outputdir",
        type=Path,
        default=None,
        metavar="OUTPUT_DIR_PATH",
        help="Parent directory under which subdirectory named after each"
        "{order} will be created for output results. If none "
        "provided, defaults to:\n"
        "\tTestSupport/tables/{order}/n{order}.mALL.{command}.out.txt",
    )
    parser.add_argument(
        "-l",
        "--canonicalfiles",
        action="store_true",
        help="Indicates .g6 input files are in canonical form",
    )
    parser.add_argument(
        "-m",
        "--makegfiles",
        action="store_true",
        help="Indicates .g6 input files were generated by makeg",
    )

    args = parser.parse_args()

    eda_ttg = TestTableGenerationWithInvalidOKs(
        planarity_path=args.planaritypath,
        orders=args.orders,
        graph_dir=args.graphdir,
        planarity_output_dir=args.planarityoutputdir,
        test_table_output_dir=args.outputdir,
        canonical_files=args.canonicalfiles,
        makeg_files=args.makegfiles,
    )

    eda_ttg.determine_numInvalidOKs()
    eda_ttg.generate_test_table_with_numInvalidOK()

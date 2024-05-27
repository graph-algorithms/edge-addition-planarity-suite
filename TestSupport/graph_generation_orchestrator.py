#!/usr/bin/env python

__all__ = []

import multiprocessing
import subprocess
import argparse
from pathlib import Path


def call_geng(geng_path:str, order:int, num_edges:int, output_dir:Path):
    """Call nauty geng as blocking process on multiprocessing thread

    Opens a file for write (overwrites file if it exists) within the output_dir
    and uses subprocess.run() to start a blocking process on the
    multiprocessing pool thread to call the nauty geng executable with the
    desired order and number of edges, with stdout redirected to the output
    file object.

    The resulting .g6 output file will contain all graphs of the desired order
    for a single edge count.

    Args:
        geng_path: Path to the nauty geng executable
        order: Desired number of vertices
        num_edges: Desired number of edges
        output_dir: Directory to which you wish to write the resulting .g6 file
    """
    filename = Path.joinpath(output_dir, f'n{order}.m{num_edges}.g6')
    with open(filename, "w") as outfile:
        subprocess.run(
            [f'{geng_path}', f'{order}', f'{num_edges}:{num_edges}'],
            stdout=outfile, stderr=subprocess.PIPE)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage='python %(prog)s [options]',
        description="""Graph Generation Orchestrator

Orchestrates calls to nauty's geng to generate graphs for a given order,
separated out into files for each edge count.
""")

    parser.add_argument('gengpath', type=str)
    parser.add_argument('-n', '--order', type=int, default=11)
    parser.add_argument('outputdir', type=Path)

    args = parser.parse_args()

    order = args.order
    geng_path = args.gengpath
    output_dir = Path.joinpath(args.outputdir, str(order))
    Path.mkdir(Path((output_dir)), parents=True, exist_ok=True)

    call_geng_args = [
        (geng_path, order, edge_count, output_dir) 
        for edge_count in range((int)((order * (order - 1)) / 2) + 1)
        ]

    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        _ = pool.starmap_async(call_geng, call_geng_args)
        pool.close()
        pool.join()

import multiprocessing
import subprocess
import argparse
from pathlib import Path


def call_geng(geng_path:str, order:int, num_edges:int, output_dir:Path):
    filename = Path.joinpath(output_dir, f'test.n{order}.m{num_edges}.g6')
    with open(filename, "w") as outfile:
        subprocess.run([f'{geng_path}', f'{order}', f'{num_edges}:{num_edges}'], stdout=outfile, stderr=subprocess.PIPE, shell=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Graph Generation Orchestrator',
        description='Orchestrates calls to nauty geng to generate graphs for a given order, separated out into files for each edge count.'
        )
    
    parser.add_argument('gengpath', type=str)
    parser.add_argument('-n', '--order', type=int, default=11)
    parser.add_argument('outputdir', type=Path)

    args = parser.parse_args()

    order = args.order
    geng_path = args.gengpath
    output_dir = args.outputdir
    Path.mkdir(Path((output_dir)), parents=True, exist_ok=True)

    call_geng_args = [(geng_path, order, edge_count, output_dir) for edge_count in  range((int)((order * (order - 1)) / 2) + 1)]
    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        _ = pool.starmap_async(call_geng, call_geng_args)
        pool.close()
        pool.join()

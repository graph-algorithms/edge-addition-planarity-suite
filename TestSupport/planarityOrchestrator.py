import multiprocessing
import subprocess
import argparse
from pathlib import Path


def call_planarity(planarity_path:str, command:str, order:int, num_edges:int, input_dir:Path, output_dir:Path):
    infile_path = Path.joinpath(input_dir, f'test.n{order}.m{num_edges}.g6')
    outfile_path = Path.joinpath(output_dir, f'{command}', f'n{order}.m{num_edges}.{command}.out.txt')
    subprocess.run([f'{planarity_path}', '-t', f'-{command}', f'{infile_path}', f'{outfile_path}'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Planarity execution orchestrator',
        description='Orchestrates calls to planarity Test All Graphs functionality.'
        )
    
    parser.add_argument('planaritypath', type=str)
    parser.add_argument('-n', '--order', type=int, default=11)
    parser.add_argument('inputdir', type=Path)
    parser.add_argument('outputdir', type=Path)

    args = parser.parse_args()

    order = args.order

    planarity_path = args.planaritypath

    input_dir = Path.joinpath(args.inputdir, f'{order}')

    output_dir = Path.joinpath(args.outputdir, f'{order}')
    Path.mkdir(output_dir, parents=True, exist_ok=True)

    planarity_commands = ('p', 'd', 'o', '2', '3', '4')
    for command in planarity_commands:
        path_to_make = Path.joinpath(output_dir, f'{command}')
        Path.mkdir(path_to_make, parents=True, exist_ok=True)

    call_planarity_args = [(planarity_path, command, order, num_edges, input_dir, output_dir) for num_edges in range(1, (int)((order * (order - 1)) / 2) + 1) for command in planarity_commands]
    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        _ = pool.starmap_async(call_planarity, call_planarity_args)
        pool.close()
        pool.join()

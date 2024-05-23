#!/usr/bin/env python

import json
import argparse
import logging
from pathlib import Path

logging.basicConfig(level=logging.DEBUG)

class G6DiffFinderException(Exception):
    """
    Custom exception for representing errors that arise when processing two
    .g6 files.
    """
    
    def __init__(self, message):
        super().__init__(message)
        self.message = message


class G6DiffFinder:
    _g6_header = '>>graph6<<'
    _g6_ext = '.g6'

    def __init__(self, first_comparand_infile_path: Path,
                 second_comparand_infile_path: Path):
        try:
            self._validate_infile_path(first_comparand_infile_path)
        except ValueError as e:
            raise e
        else:
            self._first_comparand_dict = \
                self._populate_comparand_dict(first_comparand_infile_path);

        try:
            self._validate_infile_path(second_comparand_infile_path)
        except ValueError as e:
            raise e
        else:
            self._second_comparand_dict = \
                self._populate_comparand_dict(second_comparand_infile_path);

    def _validate_infile_path(self, infile_path: Path):
        if not infile_path.is_file() \
            or infile_path.suffix != G6DiffFinder._g6_ext:
            raise ValueError(
                f'Path \'{infile_path}\' doesn\'t correspond to a .g6 infile.'
                )
    
    def _populate_comparand_dict(self, comparand_infile_path: Path) -> dict:
        try:
            logging.info(
                'Populating comparand dict from infile path '
                f'\'{comparand_infile_path}\'.'
                )

            with open(comparand_infile_path, 'r') as comparand_infile:
                comparand_dict = {
                    'infile_path': comparand_infile_path
                }
                line_num = 1
                for line in comparand_infile:
                    if line_num == 1:
                        line = line.replace(G6DiffFinder._g6_header, '')
                    line = line.strip()
                    if not line:
                        continue
                    if line in comparand_dict:
                        if not comparand_dict[line].get('duplicate_line_nums'):
                            comparand_dict[line]['duplicate_line_nums'] = []
                        comparand_dict[line]['duplicate_line_nums'].append(
                            line_num
                            )
                    else:
                        comparand_dict[line] = {
                            'first_line': line_num,
                            'duplicate_line_nums': []
                        }
                    line_num += 1
                return comparand_dict
        except FileNotFoundError as e:
            raise FileNotFoundError(
                'Unable to open comparand infile with path '
                f'\'{comparand_infile_path}\'.'
                ) from e

    def output_duplicates(self):
        try:
            self._output_duplicates(self._first_comparand_dict)
        except BaseException as e:
            raise G6DiffFinderException(
                'Unable to output duplicates for first .g6 file.'
                )

        try:
            self._output_duplicates(self._second_comparand_dict)
        except:
            raise G6DiffFinderException(
                'Unable to output duplicates for second .g6 file.'
                )

    def _output_duplicates(self, comparand_dict: dict):
        try:
            comparand_infile_path = \
                Path(comparand_dict['infile_path']).resolve()
        except KeyError as e:
            raise KeyError('Invalid dict structure: missing \'infile_path\'.')
        else:
            comparand_outfile_path = Path(
                str(comparand_infile_path) + '.duplicates.out.txt'
                )

            duplicated_g6_encodings = {
                    g6_encoded_graph: comparand_dict[g6_encoded_graph]
                    for g6_encoded_graph in comparand_dict
                    if g6_encoded_graph != 'infile_path'
                    if len(
                        comparand_dict[g6_encoded_graph]['duplicate_line_nums']
                        ) > 0
                }
            
            if not duplicated_g6_encodings:
                logging.info(
                    f'No duplicates present in \'{comparand_infile_path}\'.'
                    )
            else:
                with open(comparand_outfile_path, 'w') as comparand_outfile:
                    logging.info(
                        'Outputting duplicates present in '
                        f'\'{comparand_infile_path}\''
                        )
                    comparand_outfile.write(
                        'Comparand infile name: '
                        f'\'{comparand_infile_path.name}\'\n'
                        )
                    comparand_outfile.write(
                        json.dumps(duplicated_g6_encodings, indent=4)
                        )

    def graph_set_diff(self):
        self._graph_set_diff(
            self._first_comparand_dict, self._second_comparand_dict
            )
        self._graph_set_diff(
            self._second_comparand_dict, self._first_comparand_dict
            )

    def _graph_set_diff(
            self,
            first_comparand_dict: dict,
            second_comparand_dict: dict
        ):
        try:
            first_comparand_infile_dir, first_comparand_infile_name, \
                second_comparand_infile_name = self._get_infile_names(
                    first_comparand_dict, second_comparand_dict
                )
        except KeyError as e:
            raise G6DiffFinderException(
                'Unable to extract infile_names from dicts.'
                ) from e
        else:
            graphs_in_first_but_not_second = [
                g6_encoding
                for g6_encoding in first_comparand_dict
                if g6_encoding != 'infile_path'
                if g6_encoding not in second_comparand_dict
            ]
            
            if not graphs_in_first_but_not_second:
                logging.info(
                    'No graphs present in '
                    f'{first_comparand_infile_name} that aren\'t in '
                    f'{second_comparand_infile_name}'
                    )
            else:
                outfile_path = Path.joinpath(
                    first_comparand_infile_dir,
                    f'graphs_in_{first_comparand_infile_name}_not_in_' +
                    f'{second_comparand_infile_name}.txt'
                    )
                logging.info(
                    'Outputting graphs present in '
                    f'{first_comparand_infile_name} that aren\'t in '
                    f'{second_comparand_infile_name} to {outfile_path}.'
                    )
                with open(outfile_path, 'w') as graph_set_diff_outfile:
                    for g6_encoding in graphs_in_first_but_not_second:
                        graph_set_diff_outfile.write(g6_encoding)
                        graph_set_diff_outfile.write('\n')

    def graph_set_intersection_with_different_line_nums(self):
        try:
            first_comparand_infile_dir, first_comparand_infile_name, \
                second_comparand_infile_name = self._get_infile_names(
                    self._first_comparand_dict, self._second_comparand_dict
                )
        except KeyError as e:
            raise G6DiffFinderException(
                'Unable to extract infile_names from dicts.'
                ) from e
        else:
            graphs_in_first_and_second = {
                g6_encoding: (
                    self._first_comparand_dict[g6_encoding]['first_line'],
                    self._second_comparand_dict[g6_encoding]['first_line']
                    )
                for g6_encoding in self._first_comparand_dict
                if g6_encoding != 'infile_path'
                if g6_encoding in self._second_comparand_dict
                if self._first_comparand_dict[g6_encoding]['first_line'] != \
                    self._second_comparand_dict[g6_encoding]['first_line']
            }

            if not graphs_in_first_and_second:
                logging.info(
                    'No graphs present in both '
                    f'{first_comparand_infile_name} and '
                    f'{second_comparand_infile_name} that appear on different '
                    'lines.'
                    )
            else:
                outfile_path = Path.joinpath(
                    first_comparand_infile_dir,
                    f'graphs_in_{first_comparand_infile_name}_and_' +
                    f'{second_comparand_infile_name}.txt'
                    )
                logging.info(
                    'Outputting graphs present in both '
                    f'{first_comparand_infile_name} and '
                    f'{second_comparand_infile_name} that appear on different '
                    f'lines to {outfile_path}.'
                    )
                with open(outfile_path, 'w') as graph_set_intersection_outfile:
                    graph_set_intersection_outfile.write(
                        json.dumps(
                            graphs_in_first_and_second,
                            indent = 4
                        )
                    )

    def _get_infile_names(
            self,
            first_comparand_dict: dict,
            second_comparand_dict: dict
        ):
        try:
            first_comparand_infile_path = \
                Path(first_comparand_dict['infile_path']).resolve()
        except KeyError as e:
            raise KeyError('Invalid dict structure: missing \'infile_path\'.')
        else:
            first_comparand_infile_dir = first_comparand_infile_path.parent
            first_comparand_infile_name = \
                first_comparand_infile_path.with_suffix('').name
            try:
                second_comparand_infile_name = \
                    Path(
                        second_comparand_dict['infile_path']
                        ).with_suffix('').name
            except KeyError as e:
                raise KeyError(
                    'Invalid dict structure: missing \'infile_path\'.'
                    )
            else:
                return first_comparand_infile_dir, \
                    first_comparand_infile_name, \
                    second_comparand_infile_name


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage='python %(prog)s [options]',
        description="""Tool to help interrogate and compare two .g6 files.

- Determines if there are duplicates in the first and second comparand .g6 
files, output to separate files.
- Determines if there any graphs that appear in the first .g6 file that do not 
appear in the second .g6 file, and vice versa, output to separate files.
- Records graphs that occur in both files but which appear on different line 
numbers
""",
        )
    
    parser.add_argument(
        '--first_comparand', '-f',
        type=Path,
        help='The first .g6 file to compare.',
        metavar='FIRST_COMPARAND.g6',
        required=True
        )
    parser.add_argument(
        '--second_comparand', '-s',
        type=Path,
        help='The seconnd .g6 file to compare.',
        metavar='SECOND_COMPARAND.g6',
        required=True
        )

    args = parser.parse_args()

    try:
        g6_diff_finder = G6DiffFinder(
            args.first_comparand, args.second_comparand
            )
    except:
        raise G6DiffFinderException(
            'Unable to initialize G6DiffFinder with given input files.'
            )

    try:
        g6_diff_finder.output_duplicates()
    except:
        raise G6DiffFinderException(
            'Unable to output duplicates for given input files.'
            )

    try:
        g6_diff_finder.graph_set_diff()
    except:
        raise G6DiffFinderException(
            'Failed to discern diff between two .g6 input files.'
        )
    
    try:
        g6_diff_finder.graph_set_intersection_with_different_line_nums()
    except:
        raise G6DiffFinderException(
            'Failed to determine set intersection of two .g6 input files.'
        )

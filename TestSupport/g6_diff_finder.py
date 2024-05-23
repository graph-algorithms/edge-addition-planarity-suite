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
        """
        Initializes G6DiffFinder instance.

        For each .g6 path provided, validates the infile path and then
        populates the corresponding comparand dict.

        Args:
            first_comparand_infile_path: path to a .g6 file
            second_comparand_infile_path: path to a .g6 file
        """
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
        """
        Ensures the path provided corresponds to a file, and that the file has
        the expected extension. No further validation is performed to ensure
        the file actually corresponds to a .g6 file.

        Args:
            infile_path: path to a .g6 file
        """
        if not infile_path.is_file() \
            or infile_path.suffix != G6DiffFinder._g6_ext:
            raise ValueError(
                f'Path \'{infile_path}\' doesn\'t correspond to a .g6 infile.'
                )
    
    def _populate_comparand_dict(self, comparand_infile_path: Path) -> dict:
        """
        Opens the file corresponding to path comparand_infile_path, then
        iterates over the lines of the file object. If the first line contains
        the .g6 header, it is stripped from the line contents. Then, 
        the line contents are stripped of whitespace, after which we check to
        see if the graph encoding already has appeared in the file. If so, we
        add the current line_num to the duplicate_line_nums list corresponding
        to that encoding. If not, we insert a key-value pair into the comparand
        dict corresponding to the graph encoding mapped to a sub-dict with the
        first_line on which the encoding occurs and an empty list of
        duplicate_line_nums in case that same encoding appears again in the
        file.

        Args:
            comparand_infile_path: path to a .g6 file
        """
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
        """
        Calls self._output_duplicates() for each comparand dict so that we have
        two separate output files containing the duplicates within their
        respective .g6 input files.
        """
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
        """
        Performs a dictionary-comprehension to get each g6_encoded_graph whose
        corresponding value has a non-empty duplicate_line_nums list.

        If this dictionary is empty, emits a log message that no duplicates
        were found.

        If the dictionary is nonempty, json.dumps() to file whose name is 
        the comparand_infile_path with '.duplicates.out.txt' appended. The file
        is overwritten if it already exists.

        Args:
            comparand_dict: contains key-value pairs of graph encodings mapped
                to sub-dicts, which contain the first_line on which the
                encoding occurred and the list of duplicate_line_nums on which
                the graph recurred in the file.
        """
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
        """
        Calls self._graph_set_diff() for both the self._first_comparand_dict
        against the second self._second_comparand_dict and vice versa to output
        which graphs are in the first .g6 file that are absent from the second,
        and in the second .g6 file that are absent from the first.
        """
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
        """
        Gets the first_comparand_infile_dir (which is where the output
        file will be written to, if the result is nonempty) and the infile
        names that were used to populate the respective dicts; these are used
        to construct the outfile path.
        
        Performs a list-comprehension to determine which graph encodings appear
        in the first file that don't appear in the second.

        If the graphs_in_first_but_not_second is empty, emits a log message
        indicating that no graphs were found in the first dict that were not
        in the second.

        If graphs_in_first_but_not_second is nonempty, writes each graph
        encoding followed by a newline so that the output constitutes a valid
        .g6 file.

        The output filepath will be of the form
        {first_comparand_infile_dir}/graphs_in_{first_comparand_infile_name}_and_{second_comparand_infile_name}.g6

        Args:
            first_comparand_dict: contains key-value pairs of graph encodings
                mapped to sub-dicts, which contain the first_line on which the
                encoding occurred and the list of duplicate_line_nums on which
                the graph recurred in the file. 
            second_comparand_dict: Same structure as first_comparand_dict.
        """
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
                    f'{second_comparand_infile_name}.g6'
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
        """
        Takes the two comparand dicts associated with the G6DiffFinder and gets
        the first_comparand_infile_dir (which is where the output
        file will be written to, if the result is nonempty) and the infile
        names corresponding to the self._first_comparand_dict and
        self._second_comparand_dict; these are used to construct the outfile
        path.

        Performs a dictionary comprehension to produce key-value pairs of
        graph encoding mapped to a tuple containing the first_line on which the
        encoding occurred in the first .g6 infile and the first_line on which
        the encoding occurred in the second .g6 infile.

        If the graphs_in_first_and_second is empty, emits a log message
        indicating that the intersection is empty.

        If graphs_in_first_and_second is nonempty, json.dumps() the dict to the
        output file of the form:
        {first_comparand_infile_dir}/graphs_in_{first_comparand_infile_name}_and_{second_comparand_infile_name}.txt
        """
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
        """
        Uses pathlib.Path object's .parent attribute to get the directory of
        the first .g6 input file, stored in the first_comparand_dict's
        infile_path attribute. Then, gets the first_comparand_infile_name by
        stripping the .g6 extension, and likewise for the
        second_comparand_infile_name.

        Args:
            first_comparand_dict: Dict containing the key infile_name,
                in addition to the key-value pairs of graph encodings mapped to
                their first_line and the list of duplicate_line_nums
            second_comparand_dict: Same structure as first_comparand_dict
        
        Returns:
            first_comparand_infile_dir: Parent directory of the first .g6 file
            first_comparand_infile_name: base name of the first .g6 file
            second_comparand_infile_name: base name of the second .g6 file
        """
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
        help='The second .g6 file to compare.',
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

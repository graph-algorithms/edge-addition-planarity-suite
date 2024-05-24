#!/usr/bin/env python

__all__ = []

import argparse
from pathlib import Path
import re


class TestAllGraphsOutputFileContentsError(BaseException):
    """
    Custom exception for representing errors that arise when processing
    files which are purportedly the output files produced by running the
    planarity Test All Graphs functionality for a single algorithm command
    """

    def __init__(self, message):
        super().__init__(message)
        self.message = message


class TestAllGraphsPathError(BaseException):
    """
    Custom exception signalling issues with the paths of input for the Test
    Table Generator.
    
    For example:
    - input_dir or output_dir don't correspond to directories that exist
    - input filename contains an order or command that doesn't align with the
        order and command extracted from the parts of the parent directory
    """

    def __init__(self, message):
        super().__init__(message)
        self.message = message


class TestTableGenerator():
    # Prepending '__' for "name mangling" of this class attribute
    __planarity_commands = ('p', 'd', 'o', '2', '3', '4')

    def __init__(self, input_dir:Path, output_dir:Path):
        """Initializes TestTableGenerator instance.

        Checks that input_dir exists and is a directory, and that it contains
        input files. Also checks that the output_dir exists and is a directory.

        Args:
            input_dir: Directory containing the files output by running the
                planarity Test All Graphs functionality for a given algorithm
                command on .g6 input files containing all graphs of a given
                order and edge count.
            output_dir: Directory to which you wish to write the output file
                containing the tabulated results compiled from the input files
                in the input_dir.

        Raises:
            TestAllGraphsPathError: If the input_dir doesn't correspond to a
                directory, or if it is empty; or if the output_dir doesn't 
                correspond to a directory.
        """
        # According to PEP-8, one must use one leading underscore only for 
        # non-public methods and instance variables.
        self._processed_data = {}

        if not Path.is_dir(input_dir):
            raise TestAllGraphsPathError(
                f'\'{input_dir}\' is not a valid directory.')
        
        if len(list(input_dir.iterdir())) == 0:
            raise TestAllGraphsPathError(f'\'{input_dir}\' contains no files.')
        
        self.input_dir = input_dir

        if not Path.is_dir(output_dir):
            raise TestAllGraphsPathError(
                f'\'{output_dir}\' is not a valid directory.')
        
        self.output_dir = output_dir
    
    def get_order_and_command_from_input_dir(self):
        """Extract order and command from input_dir if possible

        Takes the self.input_dir pathlib.Path object's parts and attempts to
        extract the order and command from the directory structure. If the 
        input directory is such that the path is of the form
            {parent_dir}/{order}/{command}/
        Then we can extract these values early, allowing us to validate
        individual input files later in the process. Otherwise, sets
        self.order, self.max_num_edges, and self.command to None.
        """
        parts = self.input_dir.parts
        try:
            order = int(parts[-2])
        except ValueError:
            order = None

        command = parts[-1]
        # You may reference class attributes either by the name of the class,
        # seen here, or by using "self"
        if command not in TestTableGenerator.__planarity_commands:
            command = None

        self.order = order
        self.max_num_edges = ((order * (order - 1)) / 2) if order else None
        self.command = command
    
    def process_files(self):
        """Process input files to populate _processed_data and _totals dicts 

        Iterates over the filenames within the input_dir and constructs the
        full infile_path, then calls self._process_file() on this infile_path.

        After all files have been processed, constructs the self._totals dict
        containing the compiled results for the numGraphs, numOK,
        numNONEMBEDDABLE, numErrors, and duration. Their values are generated
        by summing the results of a list-comprehension, which extracts the
        values of the same key name in the list of dicts within
        self._processed_data.values()

        Raises:
            TestAllGraphsOutputFileError: If an error occurred processing the
                input file corresponding to path infile_path
        """
        for (dirpath, _, filenames) in Path.walk(input_dir):
            for filename in filenames:
                infile_path = Path.joinpath(dirpath, filename)
                try:
                    self._process_file(infile_path)
                except TestAllGraphsOutputFileContentsError as e:
                    raise TestAllGraphsOutputFileContentsError(
                        f'Error processing \'{infile_path}\'.') from e
                
        self._totals = {
            'numGraphs': sum(
                int(x.get('numGraphs')) for x in self._processed_data.values()
                ),
            'numOK': sum(
                int(x.get('numOK')) for x in self._processed_data.values()
                ),
            'numNONEMBEDDABLE': sum(
                int(x.get('numNONEMBEDDABLE')) 
                for x in self._processed_data.values()
                ),
            'numErrors': sum(
                1
                for x in self._processed_data.values()
                if x.get('errorFlag') == 'SUCCESS'
                ),
            'duration': sum(
                float(x.get('duration'))
                for x in self._processed_data.values()
                )
        }

    def _process_file(self, infile_path:Path):
        """Process infile and integrate into _processed_data dict

        Validates the infile name, then processes file contents and adds to the
        self._processed_data dict.

        Args:
            infile_path: Corresponds to a file within the self.input_dir
        
        Raises:
            TestAllGraphsPathError: If invalid infile_path
            TestAllGraphsOutputFileContentsError: If input file corresponds to
                results that have already been processed, or re-raises
                exception thrown by self._process_file_contents()
        """
        infile_name = infile_path.parts[-1]
        try:
            num_edges = self._validate_infile_name(infile_path)
        except TestAllGraphsPathError as e:
            raise TestAllGraphsPathError(
                'Unable to process file when given invalid infile name.'
                ) from e
        else:
            if num_edges in self._processed_data.keys():
                raise TestAllGraphsOutputFileContentsError(
                                'Already processed a file corresponding to ' \
                                f'{num_edges} edges.')
            try:
                planarity_infile_name, duration, numGraphs, numOK, \
                numNONEMBEDDABLE, errorFlag \
                    = self._process_file_contents(infile_path)
            except TestAllGraphsOutputFileContentsError as e:
                raise TestAllGraphsOutputFileContentsError(
                    f'Unable to process contents of \'{infile_path}\'.') from e
            else:
                self._processed_data[num_edges] = {
                    'infilename': planarity_infile_name,
                    'numGraphs': numGraphs,
                    'numOK': numOK,
                    'numNONEMBEDDABLE': numNONEMBEDDABLE,
                    'errorFlag': errorFlag,
                    'duration': duration
                }


    def _validate_infile_name(self, infile_path:Path):
        """Checks that infile_path corresponds to output of running planarity
        
        Args:
            infile_path: pathlib.Path object indicating the input file whose
                name should be validated before processing
        Raises:
            TestAllGraphsPathError: If infile_name doesn't match the expected
                pattern for an output file from planarity Test All Graphs,
                if the graph order indicated by the infile_name doesn't match
                previously processed files, if the num edges in the input graph
                doesn't make sense (greater than max_num_edges), if the
                algorithm command specifier isn't one of the supported values,
                or if the algorithm command specifier doesn't match previously 
                processed files.
        """
        infile_name = infile_path.parts[-1]
        match = re.match(
            r'n(?P<order>\d+)\.m(?P<num_edges>\d+)(?:\.g6)?\.' \
            r'(?P<command>[pdo234])\.out\.txt',
            infile_name)
        if not match:
            raise TestAllGraphsPathError(
                f'Infile name \'{infile_name}\' doesn\'t match pattern.')
        
        order = int(match.group('order'))
        num_edges = int(match.group('num_edges'))
        command = match.group('command')

        if self.order is None:
            self.order = order
            self.max_num_edges = ((order * (order - 1)) / 2)
        elif self.order != order:
            raise TestAllGraphsPathError(
                f'Infile name \'{infile_name}\' indicates graph order doesn\'t'
                ' equal previously derived order.')
        
        if self.max_num_edges and (num_edges > self.max_num_edges):
            raise TestAllGraphsPathError(
                f'Infile name \'{infile_name}\' indicates graph num_edges is'
                ' greater than possible for a simple graph.')
        
        if command not in TestTableGenerator.__planarity_commands:
            raise TestAllGraphsPathError(
                f'Infile name \'{infile_name}\' contains invalid algorithm '
                f'command \'{command}\'.')

        if not self.command:
            self.command = command
        elif command != self.command:
            raise TestAllGraphsPathError(
                'Command specified in input filename doesn\'t match previously'
                ' derived algorithm command.'
            )
        return num_edges
        
    def _process_file_contents(self, infile_path:Path):
        """Processes and validates input file contents

        Uses re.match() to determine whether the file contents are of the
        expected form and attempts to extract the values produced by running
        planarity Test All Graphs for a given algorithm command on a specific
        .g6 file containing all graphs of a given order and single edge-count.

        Args:
            infile_path: pathlib.Path object indicating the input file whose
                contents are validated and processed

        Returns:
            planarity_infile_name: extracted from infile_path.parts
            duration: How long it took to run the chosen graph algorithm on all
                graphs of the given order for the given number of edges
            numGraphs: total number of graphs processed in the .g6 infile
            numOK: number of graphs for which running the planarity algorithm
                specified by the command returned OK (i.e. gp_Embed() with
                embedFlags corresponding to the command returned OK and 
                gp_TestEmbedResultIntegrity() also returned OK)
            numNONEMBEDDABLE: number of graphs for which running the planarity 
                algorithm specified by the command returned NONEMBEDDABLE (i.e.
                gp_Embed() with embedFlags corresponding to the command
                returned NONEMBEDDABLE and gp_TestEmbedResultIntegrity() also 
                returned NONEMBEDDABLE) 
            errorFlag: either SUCCESS (if all graphs reported OK or
                NONEMBEDDABLE) or ERROR (if an error was encountered allocating
                memory for or managing the graph datastructures, if an error
                was raised by the G6ReadIterator, or if the Result from
                gp_Embed() doesn't concur with gp_TestEmbedResultIntegrity())

        Raises:
            TestAllGraphsOutputFileContentsError: If the input file's header
                doesn't have the expected format or values for those fields, if
                the body of the input file doesn't have the expected format, if
                the command derived doesn't match the expected algorithm
                command specifier, or if the 
        """
        with open(infile_path, 'r') as infile:
            line = infile.readline()
            match = re.match(
                r'FILENAME="(?P<filename>n\d+\.m\d+\.g6)"' \
                r' DURATION="(?P<duration>\d+\.\d{3})"', line)
            if not match:
                raise TestAllGraphsOutputFileContentsError(
                    'Invalid file header.')
            
            planarity_infile_name = match.group('filename')
            if not planarity_infile_name:
                raise TestAllGraphsOutputFileContentsError(
                    'Header doesn\'t contain input filename.')
            
            duration = match.group('duration')
            if not duration:
                raise TestAllGraphsOutputFileContentsError(
                    'Unable to extract duration from input file.')
            
            duration = float(duration)

            line = infile.readline()
            match = re.match(
                r'-(?P<command>\w) (?P<numGraphs>\d+) ' \
                r'(?P<numOK>\d+) (?P<numNONEMBEDDABLE>\d+) ' \
                r'(?P<errorFlag>SUCCESS|ERROR)', line
            )
            if not match:
                raise TestAllGraphsOutputFileContentsError(
                    'Invalid file contents.')
            
            command = match.group('command')
            if command != self.command:
                raise TestAllGraphsOutputFileContentsError(
                    'Command specified in input file doesn\'t match command '
                    'given in input filename.'
                )
            
            numGraphs = match.group('numGraphs')
            numOK = match.group('numOK')
            numNONEMBEDDABLE = match.group('numNONEMBEDDABLE')

            errorFlag = match.group('errorFlag')
            if not errorFlag or errorFlag not in ('SUCCESS', 'ERROR'):
                raise TestAllGraphsOutputFileContentsError(
                    'Invalid errorFlag; must be SUCCESS or ERROR')
            
            return planarity_infile_name, duration, numGraphs, numOK, \
                numNONEMBEDDABLE, errorFlag 

    def write_table_formatted_data_to_file(self):
        """Writes the data extracted from the input files and totals to table
        """
        self.output_dir = Path.joinpath(self.output_dir, f'{self.order}')
        Path.mkdir(self.output_dir, parents=True, exist_ok=True)

        output_path = Path.joinpath(
            self.output_dir,
            f'n{self.order}.mALL.{self.command}.out.txt'
        )

        infilename_heading = 'Input filename'
        num_edges_heading = '# Edges'
        num_graphs_heading = '# Graphs'
        numOK_heading = "# OK"
        num_NONEMBEDDABLE_heading = '# NONEMBEDDABLE'
        errorFlag_heading = 'Error flag'
        duration_heading = 'Duration'

        max_infilename_length = max(
            len(infilename_heading),
            max(
                    [
                        len(x.get('infilename')) 
                        for x in self._processed_data.values()
                    ]
            )
        )
        max_num_edges_length = max(
            len(num_edges_heading),
            max(
                    [
                        len(str(num_edges))
                        for num_edges in self._processed_data.keys()
                    ]
                )
        )
        max_numGraphs_length = max(
            len(str(self._totals.get('numGraphs'))), len(num_graphs_heading)
        )
        max_numOK_length = max(
            len(str(self._totals.get('numOK'))), len(numOK_heading)
        )
        max_numNONEMBEDDABLE_length = max(
            len(str(self._totals.get('numNONEMBEDDABLE'))),
            len(num_NONEMBEDDABLE_heading)
        )
        max_errorFlag_length = len(errorFlag_heading)
        max_duration_length = max(
            len(str(self._totals.get('duration'))),
            len(duration_heading)
        )

        with open(output_path, 'w') as outfile:
            # Print the table header
            outfile.write(
                "| {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} |\n"
                .format(
                    infilename_heading, max_infilename_length,
                    num_edges_heading, max_num_edges_length,
                    num_graphs_heading, max_numGraphs_length,
                    numOK_heading, max_numOK_length,
                    num_NONEMBEDDABLE_heading, max_numNONEMBEDDABLE_length,
                    errorFlag_heading, max_errorFlag_length,
                    duration_heading, max_duration_length
                )
            )
            # Print the table header separator
            outfile.write(
                "|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|\n"
                .format(
                    '', max_infilename_length,
                    '', max_num_edges_length,
                    '', max_numGraphs_length,
                    '', max_numOK_length,
                    '', max_numNONEMBEDDABLE_length,
                    '', max_errorFlag_length,
                    '', max_duration_length
                )
            )

            # Print the table rows
            for num_edges in sorted(self._processed_data.keys()):
                data = self._processed_data.get(num_edges)
                outfile.write(
                    "| {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} |\n"
                    .format(
                        data.get('infilename'), max_infilename_length,
                        num_edges, max_num_edges_length,
                        data.get('numGraphs'), max_numGraphs_length,
                        data.get('numOK'), max_numOK_length,
                        data.get('numNONEMBEDDABLE'), max_numNONEMBEDDABLE_length,
                        data.get('errorFlag'), max_errorFlag_length,
                        data.get('duration'), max_duration_length
                    )
                )

            # Print the table footer separator
            outfile.write(
                "|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|={:=<{}}=|\n"
                .format(
                    '', max_infilename_length,
                    '', max_num_edges_length,
                    '', max_numGraphs_length,
                    '', max_numOK_length,
                    '', max_numNONEMBEDDABLE_length,
                    '', max_errorFlag_length,
                    '', max_duration_length
                )
            )

            # Print the totals footer
            data = self._totals
            outfile.write(
                "| {:<{}}   {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} |\n"
                .format(
                    'TOTALS', max_infilename_length,
                    '', max_num_edges_length,
                    data.get('numGraphs'), max_numGraphs_length,
                    data.get('numOK'), max_numOK_length,
                    data.get('numNONEMBEDDABLE'), max_numNONEMBEDDABLE_length,
                    data.get('numErrors'), max_errorFlag_length,
                    data.get('duration'), max_duration_length
                )
            )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage='python %(prog)s [options]',
        description="""Test Table Generator

Tabulates results from output files produced by planarity's Test All Graphs
functionality.
""")

    parser.add_argument('inputdir', type=Path)
    parser.add_argument('outputdir', type=Path)

    args = parser.parse_args()

    input_dir = Path.absolute(args.inputdir)   
    output_dir = Path.absolute(args.outputdir)

    ttg = TestTableGenerator(input_dir, output_dir)
    ttg.get_order_and_command_from_input_dir()
    ttg.process_files()
    ttg.write_table_formatted_data_to_file()

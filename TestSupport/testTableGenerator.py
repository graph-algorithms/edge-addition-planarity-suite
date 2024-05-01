import argparse
from pathlib import Path
import re


class TestTableGenerator():
    # Prepending '__' for "name mangling" of this class attribute
    __planarity_commands = ('p', 'd', 'o', '2', '3', '4')

    def __init__(self, input_dir:Path, output_dir:Path):
        # According to PEP-8, one must use one leading underscore only for 
        # non-public methods and instance variables.
        self._processed_data = {}

        if (not Path.is_dir(input_dir)):
            raise ValueError(f'\'{input_dir}\' is not a valid directory.')
        
        if (len(list(input_dir.iterdir())) == 0):
            raise ValueError(f'\'{input_dir}\' contains no files.')
        
        self.input_dir = input_dir

        if (Path.is_file(output_dir)):
            raise ValueError(f'\'{output_dir}\' is a file.')
        
        Path.mkdir(output_dir, parents=True, exist_ok=True)
        self.output_dir = output_dir
    
    def getOrderAndCommandFromInputDir(self):
        parts = self.input_dir.parts
        order = int(parts[-2])
        command = parts[-1]
        # You may reference class attributes either by the name of the class,
        # seen here, or by using "self"
        if command not in TestTableGenerator.__planarity_commands:
            command = None
        if (not isinstance(order, int) or (order < 2) or (order > 100000)):
            order = None

        self.order = order
        self.max_num_edges = ((order * (order - 1)) / 2) if order else None
        self.command = command
    
    def processFiles(self):
        for (dirpath, _, filenames) in Path.walk(input_dir):
            for filename in filenames:
                infile_path = Path.joinpath(dirpath, filename)
                try:
                    self._processFile(infile_path)
                except ValueError as e:
                    raise ValueError(f'Error processing \'{infile_path}\'.') \
                        from e
                
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

    def _processFile(self, infile_path:Path):
        infile_name = infile_path.parts[-1]
        try:
            num_edges = self._validateInfileName(infile_path)
        except ValueError as e:
            raise ValueError('Invalid infile name.') from e
        else:
            if num_edges in self._processed_data.keys():
                raise ValueError(
                                f'Already processed a file corresponding to' \
                                ' {num_edges} edges.'
                                )
            try:
                planarity_infile_name, duration, numGraphs, numOK, \
                numNONEMBEDDABLE, errorFlag \
                    = self._processFileContents(infile_path)
            except ValueError as e:
                raise ValueError('Invalid file contents.') from e
            else:
                self._processed_data[num_edges] = {
                    'infilename': planarity_infile_name,
                    'numGraphs': numGraphs,
                    'numOK': numOK,
                    'numNONEMBEDDABLE': numNONEMBEDDABLE,
                    'errorFlag': errorFlag,
                    'duration': duration
                }


    def _validateInfileName(self, infile_path:Path):
        infile_name = infile_path.parts[-1]
        match = re.match(r'n(?P<order>\d+)\.m(?P<num_edges>\d+)(?:\.g6)?\.(?P<command>[pdo234])\.out\.txt', infile_name)
        if not match:
            raise ValueError(f'Infile name \'{infile_name}\' doesn\'t match '\
                             'expected pattern.')
        
        order = int(match.group('order'))
        num_edges = int(match.group('num_edges'))
        command = match.group('command')

        if self.order is None:
            self.order = order
            self.max_num_edges = ((order * (order - 1)) / 2)
        elif self.order != order:
            raise ValueError(
                f'Infile name \'{infile_name}\' indicates graph order doesn\'t'
                ' match previously processed files.'
            )
        
        if self.max_num_edges and (num_edges > self.max_num_edges):
            raise ValueError(
                f'Infile name \'{infile_name}\' indicates graph num_edges is'
                ' greater than possible for a simple graph.'
            )
        
        if command not in TestTableGenerator.__planarity_commands:
            raise ValueError(
                f'Infile name \'{infile_name}\' contains invalid algorithm '
                f'command \'{command}\'.'
            )
        if not self.command:
            self.command = command
        elif command != self.command:
            raise ValueError(
                'Command specified in input filename doesn\'t match expected '
                'value.'
            )
        return num_edges
        
    def _processFileContents(self, infile_path:Path):
        with open(infile_path, 'r') as infile:
            line = infile.readline()
            match = re.match(r'FILENAME="(?P<filename>test\.n\d+\.m\d+\.g6)"' \
                             r' DURATION="(?P<duration>\d+\.\d{3})"', line)
            if not match:
                raise ValueError(f'Invalid file header.')
            
            planarity_infile_name = match.group('filename')
            if not planarity_infile_name:
                raise ValueError(f'Header doesn\'t contain input filename.')
            
            duration = match.group('duration')
            if not duration:
                raise ValueError('Unable to extract duration from input file.')
            
            duration = float(duration)

            line = infile.readline()
            match = re.match(
                r'-(?P<command>\w) (?P<numGraphs>\d+) ' \
                r'(?P<numOK>\d+) (?P<numNONEMBEDDABLE>\d+) ' \
                r'(?P<errorFlag>SUCCESS|ERROR)', line
            )
            if not match:
                raise ValueError(f'Invalid file contents.')
            
            command = match.group('command')
            if command != self.command:
                raise ValueError(
                    'Command specified in input file doesn\'t match command '
                    'given in input filename.'
                )
            
            numGraphs = match.group('numGraphs')
            numOK = match.group('numOK')
            numNONEMBEDDABLE = match.group('numNONEMBEDDABLE')

            errorFlag = match.group('errorFlag')
            if not errorFlag:
                raise ValueError('Invalid errorFlag; must be SUCCESS or ERROR')
            
            return planarity_infile_name, duration, numGraphs, numOK, \
                numNONEMBEDDABLE, errorFlag 

    def printOutput(self):
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
        prog='Test Table Generator',
        description='Process output from execution of planarity'
        )
    
    parser.add_argument('inputdir', type=Path)
    parser.add_argument('outputdir', type=Path)

    args = parser.parse_args()

    input_dir = Path.absolute(args.inputdir)   
    output_dir = Path.absolute(args.outputdir)

    ttg = TestTableGenerator(input_dir, output_dir)
    ttg.getOrderAndCommandFromInputDir()
    ttg.processFiles()
    ttg.printOutput()

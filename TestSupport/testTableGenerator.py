import argparse
from pathlib import Path
import re
from pprint import PrettyPrinter 


class TestTableGenerator():
    __planarity_commands = ('p', 'd', 'o', '2', '3', '4')

    def __init__(self, input_dir:Path, output_dir:Path):
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
        if command not in TestTableGenerator.__planarity_commands:
            command = None
        if (not isinstance(order, int) or (order < 2) or (order > 100000)):
            order = None

        self.order = order
        self.max_cardinality = ((order * (order - 1)) / 2) if order else None
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
                
        self._processed_data['totals'] = {
            'numGraphs': sum([int(x.get('numGraphs')) for x in [self._processed_data[cardinality] for cardinality in self._processed_data.keys() if cardinality != 'totals']]),
            'numOK': sum([int(x.get('numOK')) for x in [self._processed_data[cardinality] for cardinality in self._processed_data.keys() if cardinality != 'totals']]),
            'numNONEMBEDDABLE': sum([int(x.get('numNONEMBEDDABLE')) for x in [self._processed_data[cardinality] for cardinality in self._processed_data.keys() if cardinality != 'totals']]),
            'numErrors': len([x.get('errorFlag') for x in [self._processed_data[cardinality] for cardinality in self._processed_data.keys() if cardinality != 'totals'] if x.get('errorFlag') == 'ERROR']),
            'duration': sum([float(x.get('duration')) for x in [self._processed_data[cardinality] for cardinality in self._processed_data.keys() if cardinality != 'totals']])
        }

    
    def _processFile(self, infile_path:Path):
        infile_name = infile_path.parts[-1]
        try:
            cardinality = self._validateInfileName(infile_path)
        except ValueError as e:
            raise ValueError('Invalid infile name.') from e
        else:
            if cardinality in self._processed_data.keys():
                raise ValueError(
                                f'Already processed a file corresponding to'\
                                ' {cardinality} number of edges.'
                                )
            try:
                planarity_infile_name, duration, numGraphs, numOK, numNONEMBEDDABLE, errorFlag = self._processFileContents(infile_path)
            except ValueError as e:
                raise ValueError('Invalid file contents.') from e
            else:
                self._processed_data[cardinality] = {
                    'infilename': planarity_infile_name,
                    'numGraphs': numGraphs,
                    'numOK': numOK,
                    'numNONEMBEDDABLE': numNONEMBEDDABLE,
                    'errorFlag': errorFlag,
                    'duration': duration
                }


    def _validateInfileName(self, infile_path:Path):
        infile_name = infile_path.parts[-1]
        match = re.match(r'n(?P<order>\d+)\.m(?P<cardinality>\d+)(?:\.g6)?\.(?P<command>[pdo234])\.out\.txt', infile_name)
        if not match:
            raise ValueError(f'Infile name \'{infile_name}\' doesn\'t match '\
                             'expected pattern.')
        
        order = int(match.group('order'))
        cardinality = int(match.group('cardinality'))
        command = match.group('command')

        if self.order is None:
            self.order = order
            self.max_cardinality = ((order * (order - 1)) / 2)
        elif self.order != order:
            raise ValueError(
                f'Infile name \'{infile_name}\' indicates graph order doesn\'t'
                ' match previously processed files.'
            )
        
        if self.max_cardinality and (cardinality > self.max_cardinality):
            raise ValueError(
                f'Infile name \'{infile_name}\' indicates graph cardinality is'
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
        return cardinality
        
    def _processFileContents(self, infile_path:Path):
        with open(infile_path, 'r') as infile:
            line = infile.readline()
            match = re.match(r'FILENAME="(?P<filename>test\.n\d+\.m\d+\.g6)" DURATION="(?P<duration>\d+\.\d{3})"', line)
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
            match = re.match(r'-(?P<command>\w) (?P<numGraphs>\d+) (?P<numOK>\d+) (?P<numNONEMBEDDABLE>\d+) (?P<errorFlag>SUCCESS|ERROR)', line)
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
            
            return planarity_infile_name, duration, numGraphs, numOK, numNONEMBEDDABLE, errorFlag 

    def printOutput(self):
        pp = PrettyPrinter(indent = 4)
        pp.pprint(self._processed_data)


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
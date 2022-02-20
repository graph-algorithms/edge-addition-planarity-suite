# Edge Addition Planarity Suite

The primary purpose of this repository is to provide implementations of the edge addition planar graph embedding algorithm and related algorithms, including a planar graph drawing method, an isolator for a minimal subgraph obstructing planarity in non-planar graphs, outerplanar graph embedder and obstruction isolator algorithms, and tester/isolator algorithms for subgraphs homeomorphic to _K<sub>2,3</sub>_, _K<sub>4</sub>_, and _K<sub>3,3</sub>_. The C implementations in this repository are the reference implementations of algorithms appearing in the following papers:

* [Subgraph Homeomorphism via the Edge Addition Planarity Algorithm](http://dx.doi.org/10.7155/jgaa.00268)

* [A New Method for Efficiently Generating Planar Graph Visibility Representations](http://dx.doi.org/10.1007/11618058_47)

* [On the Cutting Edge: Simplified O(n) Planarity by Edge Addition](http://dx.doi.org/10.7155/jgaa.00091)

* [Simplified O(n) Algorithms for Planar Graph Embedding, Kuratowski Subgraph Isolation, and Related Problems](https://dspace.library.uvic.ca/handle/1828/9918)

As secondary purpose of this repository is to provide a generalized graph API that enables implementation of a very wide range of in-memory graph algorithms including basic methods for reading, writing, depth first search, and lowpoint as well as advanced methods for solving planarity, outerplanarity, drawing, and selected subgraph homeomorphism problems. An extension mechanism is also provided to enable implementation of planarity-related algorithms by overriding and augmenting data structures and methods of the core planarity algorithm.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Pre-compiled Executable Releases for Non-Developers

On several distributions of Linux, you may be able to get the planarity executable with _sudo apt-get planarity_ , or you may already have it you have Matlab. For non-developer Windows users, there is also a pre-compiled executable version of the algorithm implementations. Download and decompress the _planarity-N.N.N.N.WindowsExe.zip_ file. 

If you run the _planarity_ executable program, it will offer a menu-drive mode that lets a user manually select algorithms to run and, where appropriate, files containing graphs on which to run the algorithms. 

The _planarity_ executable program also supports an extensive list of command-line parameters that make it possible to automate the execution of any of the algorithms included in the application. Run _planarity_ with the "-h" command-line parameter to get more information about the command line options, and use "-h -menu" for more extensive information about command-line mode.

### Setting up a Development Environment

A development environment for the C reference implementations can be set up based on Eclipse.

1. Install a recent version of the Java JDK (such as Java version 14 or higher)
2. Ensure that you set the JAVA_HOME system environment variable (e.g. to c:\Program Files\Java\jdk-14.0.1)
3. Ensure that you add %JAVA_HOME%\bin to your system PATH
4. Install Eclipse, such as the "Eclipse IDE for Enterprise Java Developers"
5. Install gcc, gdb, and msys (e.g. download and run mingw-get-setup.exe from [here](https://osdn.net/projects/mingw/releases/) and then use the package installer to install C and C++, GDB, MSYS, and any other packages you may want.)
6. Ensure your gcc is accessible from the command line (e.g. add C:\MinGW\bin to the system PATH)
7. In Eclipse, and install the C Development Tools (CDT)
    1. In Eclipse, choose the menu option Help > Install New Software
    2. Choose to work with the main repository (e.g. 2020 - 06 - http://download.eclipse.org/releases)
    3. Under Programming Languages, choose C/C++ Autotools, C/C++ Development Tools, C/C++ Development Tools SDK, C/C++ Library API Documentation Hover Help, and C/C++ Unit Testing Support
    
### Working with the Code in the Development Environment

1. Copy the HTTPS clone URL into the copy/paste buffer
     1. In this repository, the "Code" button provides the [HTTPS clone](https://github.com/graph-algorithms/edge-addition-planarity-suite.git) link to use to get the code.
2. Start by making a new eclipse workspace for 'graph-algorithms'
     1. You may need to select 'Prompt for Workspace on startup' in Window | Preferences | General | Startup and Shutdown | Workspaces, then close and reopen eclipse
     2. In the initial workspace dialogue, one can specify a new folder that gets created, e.g. c:\Users\\_you_\Documents\eclipse\workspaces-cpp\graph-algorithms
3. Click 'Checkout projects from Git' in the new workspace Welcome page 
4. Pick 'Clone URI' and hit Next
5. The URI, Host, and Repository are pre-filled correctly from the copy/paste clipboard.
6. Leave the User/Password blank (unless you have read/write access to the project), and hit Next
7. The master branch is selected by default, so just hit Next again
8. Change the destination directory to a subdirectory where you want to store the project code (e.g. c:\Users\\_you_\Documents\eclipse\workspaces-cpp\graph-algorithms\edge-addition-planarity-suite)
9. Hit Next (which downloads the project), Hit Next again (to Import existing Eclipse projects), Hit Finish

Now that the project is available, the code can be built and executed:

1. Open the C/C++ Perspective
     1. Use the Open Perspectives button (or use Windows | Perspective | Open Perspective | Other...)
     2. Select C/C++
2. Right-click Planarity-C project, Build Configurations, Build All
3. Right-click Planarity-C project, Build Configurations, Set Active, Release
4. Right-click Planarity-C project, Run As, Local Application, planarity.exe (release)

### Making the Distribution

Once one has set up the development environment and is able to work with the code in the development environment, it is possible to make the distribution with the following additional steps:

1. Ensure that the autotools, configure, and make are available on the command-line (e.g. add C:\MinGW\msys\1.0\bin to the system PATH before Windows Program Files to ensure that the _find_ program is the one from MSYS rather than the one from Windows (e.g., adjust the PATH variable as needed)). 
2. Navigate to .../edge-addition-planarity-suite (the directory containing _configure.ac_ and the _c_ subdirectory)
3. Get into _bash_ (e.g., type _bash_ in the Windows command-line), then enter the following commands:
    1. autogen.sh
    2. configure
    3. make dist
    4. make distcheck 

The result is a validated _planarity-N.N.N.N.tar.gz_ distribution, where _N.N.N.N_ is the version number expressed in the _configure.ac_ file. 

### Making and Running the Software from the Distribution

If you have done the steps to set up the development environment and work with the code, then you can make and run the software using the development environment, so you don't necessarily need to make or run the software using the process below.

You also don't necessarily need to make and install the planarity software on Linux if you are able to get it using _sudo apt-get planarity_ . 

However, you may have only downloaded the distribution (i.e., _planarity-N.N.N.N.tar.gz_ ) from a Release tag of this project. Once you have decompressed the distribution into a directory, you can make it by getting into _bash_ (e.g., type _bash_ in the Windows command-line) and then entering the following commands: 
1. configure
2. make

At this point, the planarity executable can be run from within the distribution directory. For example, on Windows, go to the ".libs" subdirectory containing the planarity executuable and the libplanarity DLL and run _planarity -test ../c/samples_ on the command-line. 

On Linux, the planarity program can also be installed by entering _sudo make install_ on the command-line. Note that the _libplanarity_ shared object and symlinks will be installed to _/usr/local/lib_ so it will be necessary to set LD_LIBRARY_PATH accordingly. For one session, this can be done with _export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib_ . To make it more permanent, you could use:
1. Create a new file " _/etc/ld.so.conf.d/planarity.conf_ " containing " _/usr/local/lib_ " (without the quotes)
2. sudo ldconfig 

## Contributing

Subject to your acceptance of the license agreement, contributions can be made via a pull request. Before submitting a pull request, please ensure that you have set your github user name and email within your development environment. For Eclipse, you can use the following steps:

1. Window > Preferences > Team > Git > Configuration
2. Add Entry... user.name (set the value to your github identity)
3. Add Entry... user.email (set the value to the primary email of your github identity)
4. Hit Apply and Close

## Versioning

The APIs for the graph library and the planarity algorithm implementations are versioned using the method documented in [configure.ac](configure.ac).

The _planarity.exe_ application, which provides command-line and menu-driven interfaces for the graph library and planarity algorithms, is versioned according to the _Major.Minor.Maintenance.Tweak_ numbering system documented in the comments in [planarity.c](c/planarity.c). 

## License

This project is licensed under a 3-clause BSD License appearing in [LICENSE.TXT](LICENSE.TXT).

## Related Works and Further Documentation

There have been successful technology transfers of the implementation code and/or algorithms of this project into other projects. To see a list of the related projects and for further documentation about this project, please see the [project wiki](https://github.com/graph-algorithms/edge-addition-planarity-suite/wiki).

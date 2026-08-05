---
title: 'Edge Addition Planarity Suite and Generalized Graph Library'
tags:
  - C
  - Python
  - Graph Algorithms
  - Planar Graphs
  - Artificial Intelligence
  - Bioinformatics
  - Chemistry
  - Physics
  - Visualization
authors:
  - name: John M. Boyer
    orcid: 0000-0002-4755-5535
    corresponding: true
    affiliation: 1
  - name: Wanda B. K. Boyer
    affiliation: 1
affiliations:
 - name: Independent Researcher, Canada
   index: 1
date: 4 August 2026
bibliography: paper.bib
---

# Summary

A _graph_ is a mathematical structure comprising a set $V$ of vertices and a set
$E$ of edges, with each edge $e$ corresponding to a pair of vertices called the
_endpoints_ of $e$. A graph is _planar_ if its vertices can be placed in
distinct locations on a plane surface and its edges can be drawn on the plane
without the edges intersecting, except at their common vertex endpoints. For a
planar graph, a planarity algorithm typically outputs a combinatorial data
structure that validation code can use to certify the planarity of the input
graph. A planar graph drawing is typically produced by a separate algorithm. On
the other hand, if an input graph is not planar, then a planarity algorithm
typically outputs a minimal subgraph of the input graph that obstructs
planarity. Validation code can use a minimal planarity-obstructing subgraph to
certify the non-planarity of an input graph. Moreover, a minimal subgraph
obstructing planarity can be used to help decide how to amend an input graph to
_planarize_ it.

Graphs are used to model a very wide array of real-world problems in which there
are objects and relationships between the objects. In artificial intelligence,
graphs are used to help with reasoning tasks, such as about the relationship
pathways between persons of interest in law enforcement or between genes,
tissues, diseases, and medications in bioinformatics research. In Physics,
graphs and planarity are used to help compute material phase transitions,
particle interactions, and electromagnetic duality. Similarly, in chemistry,
graphs may be used to represent atoms and their valence bonds in molecules.
Planarity testing can help determine feasible molecular arrangements because the
molecular graphs for many compounds of interest must be planar. In electronics,
a graph may be used to represent the electrical components and wiring in a
circuit, such as transistors and etchings on a silicon wafer. The graph of a
circuit must be planar or planarized to eliminate short circuits.

The open source software described in this paper provides a highly performant
generalized graph library that also implements state-of-the-art algorithms for
planarity-related problems. The generalized graph library and planarity-related
algorithms are available to C/C++ and Python graph application developers.

# Statement of need

The `Edge Addition Planarity Suite` is an open source software package that was
originally developed to implement a new planarity algorithm in
[@BoyerMyrvold:EdgeAddition]. The Edge Addition Planarity Algorithm is currently
regarded as one of two state-of-the-art planarity algorithms due to its
conceptual simplicity as well as its speed of execution
[@Wikipedia:PlanarityTesting]. The software package is available from the
[`edge-addition-planarity-suite` GitHub
repository](https://github.com/graph-algorithms/edge-addition-planarity-suite)
under a 3-clause BSD license. 

To enable development of the Edge Addition Planarity Algorithm, as well as the
other planarity-related algorithms in [@Boyer:GD2005] and
[@Boyer:SubgraphHomeomorphism], it was necessary to develop an extensible,
generalized graph library. This generalized graph library provides a
high-performance solution for the graph algorithm representation and processing
needs of a very wide array of graph applications. 

In addition to enabling graph application development in C/C++, the popularity
of Python as a scientific application development language has prompted the need
for making this  
graph library and the algorithms it implements available to Python developers.
The source code may be accessed by the [`planarity` Github
repository](https://github.com/graph-algorithms/planarity), and the [`planarity`
Python package](https://pypi.org/project/planarity/) may be installed via the
Python Package Index (`PyPI`) by simply running `pip install planarity`.

# State of the field

Several large scientific software packages offer a graph planarity algorithm,
which is a complex graph algorithm suitable for benchmarking graph libraries.
Mathematica and Wolfram Alpha include planarity algorithms, but these packages
are not meant for high performance graph algorithms, so the Library for
Efficient Data Types and Algorithms
([LEDA](https://leda.uni-trier.de/leda/guide/Index.html)) is significantly
faster than they are because it is an industrial library designed for
computational efficiency. Yet, in [@BoyerCortesePatrignaniDiBattista:GD2003],
all planarity algorithms implemented in LEDA were shown  to be several times
slower than the Edge Addition Planarity Algorithm implementation. LEDA also has
the disadvantage of being C++ only and has a proprietary license. The only graph
library shown (in [@BoyerCortesePatrignaniDiBattista:GD2003]) to have similar
performance to the Edge Addition Planarity Suite is the Public Implementation of
a Graph Algorithm Library and Editor
([PIGALE](https://sourceforge.net/projects/pigale/)). However, PIGALE is also
C++ only, has a GPLv2 license, and lacks advanced algorithms for homeomorphic
subgraph search. For example, torus embedding algorithms and torus obstruction
isolation algorithms are able to operate differently based on whether a
search for a subgraph homeomorphic to $K_{3, 3}$ finds a result
[@MyrvoldKocay:ErrorsInGraphEmbeddingAlgorithms;
@MyrvoldWoodcock:TorusObstructions]. Similarly, several graph theoretic
results are applicable to graphs that are known to be $K_{3, 3}$-free, including
results related to graph isomorphism, reachability, and finding matching cuts
[@DattaEtAl:GraphIsomorphism; @FeghaliEtAl:MatchingCuts; @ThieraufWagner:Reachability]. 

# Software design

The generalized graph library in the Edge Addition Planarity Suite has a
carefully curated API and an object-oriented architecture. The base **Graph**
class structure includes

- base graph, vertex, and edge structure definitions and getter/setter methods;
- graph, vertex and edge allocation, deallocation, and reset methods;
- methods for iterating through all vertices, edges, and edge adjacency lists; 
- graph readers and writers for several formats; and
- a dynamic subclassing/extension mechanisms and virtual function table.

The base Graph is extendable with utility methods for exploring a graph,
labelling vertices and edges according to a depth-first search (DFS), and for
managing connectivity and biconnectivity information. The vertex and edge
structures are extended to enable storage of the information generated by the
utility methods.

A Graph structure extended with the DFS-related utilities is further extendable
to a **Planarity Graph**, which also further extends the vertex and edge
structures. The main additional method provided by a Planarity Graph is
`gp_Embed()`, which takes a Planarity Graph as input and outputs either a
combinatorial planar embedding or a minimal planarity-obstructing subgraph.

Given a Planarity Graph, a number of subclass extensions are available for
planarity-related problems. One extension solves outerplanarity. Another
implements the planar graph drawing algorithm in [@Boyer:GD2005]. Three other
extensions provide Graph subclasses that solve the homeomorphic subgraph search
algorithms appearing in [@Boyer:SubgraphHomeomorphism]. All of these extensions
further extend the vertex and edge structures and overload the `gp_Embed()`
function.

In addition to being available to C and C++ developers, the APIs of the Edge
Addition Planarity Suite and Generalized Graph Library are also made available
via the `planarity` Python package with the source code made available in the
[`planarity` Github repository](https://github.com/graph-algorithms/planarity).
Cython is used to preserve high performance while also broadening availability
to the Python developer community.

A final aspect of this open source software project is our emphasis on software
reliability. We perform validation code on `gp_Embed()` and its five overloads
on billions of randomly generated graphs up to 10 million vertices and 30
million edges. The GitHub actions that run on every pull request include
sanitizer tests on all graphs on 8 vertices. Finally, the 
[`edge-addition-planarity-suite-testing` GitHub
repository](https://github.com/graph-algorithms/edge-addition-planarity-suite-testing)
provides our multithreaded Python code for validating `gp_Embed()` and its five
overloads on the more than one billion graphs having 11 or fewer vertices, as
generated by the `geng` tool in Nauty and Traces
[@McKayPiperno:nauty_traces_manual].

# Research impact statement

The journal publication of the Edge Addition Planarity Algorithm,
[@BoyerMyrvold:EdgeAddition], currently has over 360 scholarly citations
according to [Google
Scholar](https://web.archive.org/web/20260801175031/https://scholar.google.ca/citations?view_op=view_citation&hl=en&user=0J7uuHQAAAAJ&citation_for_view=0J7uuHQAAAAJ:u-x6o8ySG0sC).
The algorithm has been implemented in several large open source projects,
including
[Boost](https://web.archive.org/web/20260715092401/https://www.boost.org/doc/libs/latest/libs/graph/doc/bibliography.html),
[Magma](https://web.archive.org/web/20260801174446/https://magma.maths.usyd.edu.au/magma/handbook/text/1940),
nauty and Traces [@McKayPiperno:nauty_traces_manual], and the [Open Graph
Drawing Framework](https://ogdf.uos.de/wp-content/uploads/2019/04/ogdf.pdf)
([web
archive](https://web.archive.org/web/20260801181735/https://master--ogdf.netlify.app/classogdf_1_1_boyer_myrvold_planar.html)).
An executable program built from the source code was used to build a 
knot theory software package [@JablanSazdanovic:LinKnot, p. 7, 38]. 
The source code from the Edge Addition Planarity Suite repository has been
directly included in several large open source projects, including
[SageMath](https://web.archive.org/web/20260801180924/https://www.sagemath.org/development-ack.html),
[Digraphs](https://github.com/digraphs/Digraphs/tree/main/extern), and over 80
Linux platforms releases including for Debian, Devuan, Kali, openSUSE, Raspbian,
Ubuntu, Alpine, ALT, Fedora, FreeBSD, Gentoo, Manjaro, MSYS2, and Parabola,
according to
[repology/edge-addition-planarity-suite](https://repology.org/project/edge-addition-planarity-suite/versions)
([web
archive](https://web.archive.org/web/20260801173450/https://repology.org/project/edge-addition-planarity-suite/versions))
and [repology/planarity](https://repology.org/project/planarity/versions) ([web
archive](https://web.archive.org/web/20260801173130/https://repology.org/project/planarity/versions)). 

The Python package named `planarity` was originally developed in 2013 by Aric
Hagberg as a way to get the planarity testing and drawing algorithms from
[@BoyerMyrvold:EdgeAddition] and [@Boyer:GD2005] to work with graphs from
NetworkX [@HagbergEtAl:NetworkX]. Recently, Hagberg transferred the [`planarity`
Github repository](https://github.com/graph-algorithms/planarity) to the [GitHub
Graph Algorithms Organization](https://github.com/graph-algorithms) and the
[`planarity` Python package](https://pypi.org/project/planarity/) to the [PyPI
Graph Algorithms Organization](https://pypi.org/org/graph-algorithms/), which
are both maintained by the authors. On August 1, 2026, the [Top PyPI
Packages](https://web.archive.org/web/20260801164632/https://hugovk.dev/top-pypi-packages/)
website indicated that the `planarity` Python package had over 700,000 downloads
for the preceding month and was ranked in the top 1% of PyPI package downloads
(ranked 5262 out of [864,036
projects](https://web.archive.org/web/20260801170757/https://pypi.org/)).
According to [pepy.tech](https://pepy.tech/projects/planarity) ([web
archive](https://web.archive.org/web/20260804124534/https://pepy.tech/projects/planarity))
the `planarity` Python package has been downloaded over 2 million times in
total. The `planarity` Python package has now been included as a standard Python
package in several versions of Linux including Debian, Devuan, Kali, Raspbian,
and Ubuntu, according to
[repology/python:planarity](https://repology.org/project/python%3Aplanarity/versions)
([web
archive](https://web.archive.org/web/20260801172314/https://repology.org/project/python%3Aplanarity/versions)).

<!-- 
For the 700K downloads in the last month, see also the web archived snapshot of 
[PyPI Stats](https://web.archive.org/web/20260802041158/https://pypistats.org/packages/planarity),

For July 1-31, 2026, ranking, see also 
[ClickPy](https://clickpy.clickhouse.com/dashboard/planarity?min_date=2026-07-02&max_date=2026-08-01)
-->

# AI usage disclosure

No generative AI tools were used in the development of this software, the
writing of this manuscript, nor the preparation of supporting materials.

# References

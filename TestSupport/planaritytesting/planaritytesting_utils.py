"""Shared utilities for working with graph input files

Functions:
    PLANARITY_ALGORITHM_SPECIFIERS() -> tuple[str, ...]
    max_num_edges_for_order(order: int) -> int
    g6_header() -> str
    g6_suffix() -> str
    LEDA_header() -> str
"""

__all__ = [
    "PLANARITY_ALGORITHM_SPECIFIERS",
    "max_num_edges_for_order",
    "g6_header",
    "g6_suffix",
    "LEDA_header",
]


def PLANARITY_ALGORITHM_SPECIFIERS() -> (
    tuple[str, ...]
):  # pylint: disable=invalid-name
    """Returns immutable tuple containing algorithm command specifiers"""
    return ("p", "d", "o", "2", "3", "4")


def max_num_edges_for_order(order: int) -> int:
    """Returns max number of edges possible for given graph order"""
    return (int)((order * (order - 1)) / 2)


def g6_header() -> str:
    """Returns expected .g6 file header string"""
    return ">>graph6<<"


def g6_suffix() -> str:
    """Returns expected .g6 file suffix"""
    return ".g6"


def LEDA_header() -> str:  # pylint: disable=invalid-name
    """Returns expected LEDA file header string"""
    return "LEDA.GRAPH"

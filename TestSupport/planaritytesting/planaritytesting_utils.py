__all__ = [
    'PLANARITY_ALGORITHM_SPECIFIERS',
    'max_num_edges_for_order',
    'g6_header',
    'g6_suffix',
    'LEDA_header'
]

def PLANARITY_ALGORITHM_SPECIFIERS() -> tuple[str, ...]:
    return ('p', 'd', 'o', '2', '3', '4')

def max_num_edges_for_order(order: int) -> int:
    return (int)((order * (order - 1)) / 2)

def g6_header() -> str:
    return '>>graph6<<'

def g6_suffix() -> str:
    return '.g6'

def LEDA_header() -> str:
    return 'LEDA.GRAPH'

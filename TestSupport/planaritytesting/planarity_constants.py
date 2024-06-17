__all__ = [
    'PLANARITY_ALGORITHM_SPECIFIERS',
    'max_num_edges_for_order'
]

def PLANARITY_ALGORITHM_SPECIFIERS():
    return ('p', 'd', 'o', '2', '3', '4')

def max_num_edges_for_order(order: int):
    return (int)((order * (order - 1)) / 2)

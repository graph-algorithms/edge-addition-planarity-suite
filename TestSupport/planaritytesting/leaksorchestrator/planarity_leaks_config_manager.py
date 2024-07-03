"""Creates default config file, 
"""

#!/usr/bin/env python

__all__ = []

import configparser
from pathlib import Path
import sys


def initialize_planarity_leaks_orchestrator_config(config_path: Path) -> None:
    """Initialize planarity leaks orchestrator config file with defaults

    Args:
        config_path: Path to config file to initialize with defaults
    """
    initialize_test_random_graphs_config(config_path)
    initialize_test_max_planar_graph_generator_config(config_path)
    initialize_test_specific_graph_config(config_path)


def initialize_test_random_graphs_config(config_path: Path) -> None:
    """Initialize planarity leaks orchestrator RandomGraphs config

    Args:
        config_path: Path to config file to initialize with defaults for
            test_random_graphs()
    """
    test_random_graphs_config = configparser.ConfigParser()
    test_random_graphs_config.read(config_path)

    test_random_graphs_config["RandomGraphs"] = {
        "enabled": "False",
        "perform_full_analysis": "True",
        "num_graphs": "50",
        "order": "6",
        "commands_to_run": "",
    }

    with open(config_path, "w", encoding="utf-8") as config_file:
        test_random_graphs_config.write(config_file)


def initialize_test_max_planar_graph_generator_config(
    config_path: Path,
) -> None:
    """Initialize planarity leaks orchestrator callRandomMaxPlanarGraph config

    Args:
        config_path: Path to config file to initialize with defaults for
            PlanarityLeaksOrchestrator.test_max_planar_graph_generator()
    """
    test_random_max_planar_graph_config = configparser.ConfigParser()
    test_random_max_planar_graph_config.read(config_path)

    test_random_max_planar_graph_config["RandomMaxPlanarGraphGenerator"] = {
        "enabled": "False",
        "perform_full_analysis": "True",
        "order": "100",
    }

    with open(config_path, "w", encoding="utf-8") as config_file:
        test_random_max_planar_graph_config.write(config_file)


def initialize_test_specific_graph_config(
    config_path: Path,
) -> None:
    """Initialize planarity leaks orchestrator SpecificGraph config

    Args:
        config_path: Path to config file to initialize with defaults for
            PlanarityLeaksOrchestrator.test_specific_graph()
    """
    test_specific_graph_config = configparser.ConfigParser()
    test_specific_graph_config.read(config_path)

    test_specific_graph_config["SpecificGraph"] = {
        "enabled": "False",
        "perform_full_analysis": "True",
        "infile_path": ">>CHANGEME<<",
        "commands_to_run": "",
    }

    with open(config_path, "w", encoding="utf-8") as config_file:
        test_specific_graph_config.write(config_file)


if __name__ == "__main__":
    config = configparser.ConfigParser(
        converters={
            "list": lambda x: (
                [i.strip() for i in x.split(",") if i] if len(x) > 0 else []
            )
        }
    )
    leaks_orchestrator_package_root = Path(sys.argv[0]).resolve().parent
    default_config_path = Path.joinpath(
        leaks_orchestrator_package_root,
        "planarity_leaks_config.ini",
    )
    config.read(default_config_path)

    if not config.sections():
        initialize_planarity_leaks_orchestrator_config(default_config_path)
    else:
        response = input(
            "Do you wish to re-initialize the default config file? [Y/n]"
        )
        if response.lower() == "y":
            initialize_planarity_leaks_orchestrator_config(default_config_path)
        else:
            print(
                "No changes have been made to planarity leaks default "
                f"configuration file at '{default_config_path}'."
            )

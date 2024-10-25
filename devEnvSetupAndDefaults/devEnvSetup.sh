#!/usr/bin/env bash

echo -e "Copying VSCode configuration files.\n"
cp -R .vscode ../

echo -e "Copying Python TestSupport pylint configuration file.\n"
cp TestSupport/planaritytesting/.pylintrc ../TestSupport/planaritytesting/

echo -e "Copying planarity leaks orchestrator default configuration file.\n"
cp TestSupport/planaritytesting/leaksorchestrator/planarity_leaks_config.ini ../TestSupport/planaritytesting/leaksorchestrator/
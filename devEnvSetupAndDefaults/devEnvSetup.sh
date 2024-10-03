#!/usr/bin/env bash

echo -e "Copying VSCode configuration files.\n"
cp -R .vscode ../

echo -e "Copying Python TestSupport pylint configuration file.\n"
cp TestSupport/planaritytesting/.pylintrc ../TestSupport/planaritytesting/

echo -e "Copying planarity leaks orchestrator default configuration file.\n"
cp TestSupport/planaritytesting/leaksorchestrator/planarity_leaks_config.ini ../TestSupport/planaritytesting/leaksorchestrator/

echo -e "Copying Eclipse .project and .cproject files.\n"
cp eclipse/.project ../.project
cp eclipse/c/.project ../c/.project
cp eclipse/c/.cproject ../c/.cproject

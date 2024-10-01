#!/usr/bin/env bash

# $OSTYPE not recognized in original Bourne shell, i.e. /bin/sh
case "$OSTYPE" in
  solaris*)
    echo "Solaris environment detected, which is unsupported; exiting."
    exit 1
    ;;
  darwin*)
    echo -e "MacOS environment detected; proceeding to check for Xcode command line tools.\n"
    HASXCODETOOLS="$(xcode-select -p 1>/dev/null;echo $?)"
    if [[ "$HASXCODETOOLS" != 0 ]]; then
      echo "Missing dependency: Xcode command line tools must be installed."
      xcode-select --install &> /dev/null
      until $(xcode-select --print-path &> /dev/null); do
        sleep 5;
      done
    fi
    ;; 
  linux*)
    echo -e "Linux environment detected; proceeding with package update and install of build tools.\n"
    which apt-get && {
      sudo apt-get update && sudo apt-get -y upgrade && sudo apt-get install -y build-essential autotools-dev autoconf
    }
  ;;
  bsd*)
    echo "BSD environment detected, which is unsupported; exiting."
    exit 1
    ;;
  msys*)
    echo -e "Windows MinGW environment detected; proceeding with file default copy process.\n"
    ;;
  cygwin*)
    echo "Windows Cygwin environment detected, which is unsupported; exiting."
    exit 1
    ;;
  *)
    echo "Unknown environment detected; exiting."
    exit 1
    ;;
esac

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
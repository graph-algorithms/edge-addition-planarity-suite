#!/usr/bin/env bash

# $OSTYPE not recognized in original Bourne shell, i.e. /bin/sh
case "$OSTYPE" in
  solaris*)
    echo "Solaris environment detected, which is unsupported; exiting."
    exit 1
    ;;
  darwin*)
    echo "MacOS environment detected; proceeding."
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
    which apt-get && {
      sudo apt-get update && sudo apt-get -y upgrade && sudo apt-get install -y build-essential autotools-dev autoconf
    }
  ;;
  bsd*)
    echo "BSD environment detected, which is unsupported; exiting."
    exit 1
    ;;
  msys*)
    echo "Windows MinGW environment detected; proceeding."
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

cp -R .vscode ../
cp TestSupport/planaritytesting/.pylintrc ../TestSupport/planaritytesting/
cp TestSupport/planaritytesting/leaksorchestrator/planarity_leaks_config.ini ../TestSupport/planaritytesting/leaksorchestrator/
cp eclipse/.project ../.project
cp eclipse/c/.project ../c/.project
cp eclipse/c/.cproject ../c/.cproject
#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild"
  make -f /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/_deps/clap-juce-extensions-subbuild/CMakeScripts/ReRunCMake.make
fi


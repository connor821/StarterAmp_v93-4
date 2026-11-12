#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-subbuild"
  make -f /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/_deps/juce-subbuild/CMakeScripts/ReRunCMake.make
fi


#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-build"
  /opt/homebrew/bin/cmake -E echo_append
  /opt/homebrew/bin/cmake -E touch "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/juce-populate-install"
fi


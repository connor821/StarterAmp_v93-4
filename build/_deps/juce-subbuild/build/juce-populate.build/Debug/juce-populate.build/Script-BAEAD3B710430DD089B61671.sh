#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-subbuild"
  /opt/homebrew/bin/cmake -Dcfgdir=/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME -P "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-subbuild/juce-populate-prefix/tmp/juce-populate-mkdirs.cmake"
  /opt/homebrew/bin/cmake -E touch "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/juce-populate-mkdir"
fi


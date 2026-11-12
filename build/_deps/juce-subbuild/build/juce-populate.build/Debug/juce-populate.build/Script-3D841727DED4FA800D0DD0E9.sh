#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps"
  /opt/homebrew/bin/cmake -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE -P "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-subbuild/juce-populate-prefix/tmp/juce-populate-gitclone.cmake"
  /opt/homebrew/bin/cmake -E touch "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/juce-populate-download"
fi


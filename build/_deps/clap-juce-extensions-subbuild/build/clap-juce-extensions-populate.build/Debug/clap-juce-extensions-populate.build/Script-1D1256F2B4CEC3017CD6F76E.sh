#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps"
  /opt/homebrew/bin/cmake -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE -P "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/tmp/clap-juce-extensions-populate-gitclone.cmake"
  /opt/homebrew/bin/cmake -E touch "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/src/clap-juce-extensions-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/clap-juce-extensions-populate-download"
fi


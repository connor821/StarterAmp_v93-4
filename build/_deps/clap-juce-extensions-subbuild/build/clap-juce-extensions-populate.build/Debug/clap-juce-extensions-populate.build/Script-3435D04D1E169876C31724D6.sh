#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-build"
  /opt/homebrew/bin/cmake -E echo_append
  /opt/homebrew/bin/cmake -E touch "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/src/clap-juce-extensions-populate-stamp/$CONFIGURATION$EFFECTIVE_PLATFORM_NAME/clap-juce-extensions-populate-test"
fi


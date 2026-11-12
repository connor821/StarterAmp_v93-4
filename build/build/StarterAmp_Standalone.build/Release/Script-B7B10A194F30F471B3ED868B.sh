#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_Standalone/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Debug/Standalone/Starter Amp v0.1.app/Contents"
fi
if test "$CONFIGURATION" = "Release"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_Standalone/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Release/Standalone/Starter Amp v0.1.app/Contents"
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_Standalone/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/MinSizeRel/Standalone/Starter Amp v0.1.app/Contents"
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_Standalone/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/RelWithDebInfo/Standalone/Starter Amp v0.1.app/Contents"
fi


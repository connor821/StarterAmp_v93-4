#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-build/tools/extras/Build/juceaide/juceaide_artefacts/Debug/juceaide" pkginfo VST3 "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo"
fi
if test "$CONFIGURATION" = "Release"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-build/tools/extras/Build/juceaide/juceaide_artefacts/Debug/juceaide" pkginfo VST3 "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo"
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-build/tools/extras/Build/juceaide/juceaide_artefacts/Debug/juceaide" pkginfo VST3 "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo"
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-build/tools/extras/Build/juceaide/juceaide_artefacts/Debug/juceaide" pkginfo VST3 "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo"
fi


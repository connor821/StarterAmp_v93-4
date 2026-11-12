#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Debug/VST3/Starter Amp v0.1.vst3/Contents"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake "-Dsrc=/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Debug/VST3/Starter Amp v0.1.vst3" -P "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-src/extras/Build/CMake/checkBundleSigning.cmake"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo removing\ moduleinfo.json
  /opt/homebrew/bin/cmake -E remove -f /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/Debug/VST3/Starter\ Amp\ v0.1.vst3/Contents/moduleinfo.json /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/Debug/VST3/Starter\ Amp\ v0.1.vst3/Contents/Resources/moduleinfo.json
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo "creating /Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Debug/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
  /opt/homebrew/bin/cmake -E make_directory "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Debug/VST3/Starter Amp v0.1.vst3/Contents/Resources"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/Debug/juce_vst3_helper" -create -version 0.1.0 -path "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Debug/VST3/Starter Amp v0.1.vst3" -output "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Debug/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
fi
if test "$CONFIGURATION" = "Release"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Release/VST3/Starter Amp v0.1.vst3/Contents"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake "-Dsrc=/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Release/VST3/Starter Amp v0.1.vst3" -P "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-src/extras/Build/CMake/checkBundleSigning.cmake"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo removing\ moduleinfo.json
  /opt/homebrew/bin/cmake -E remove -f /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/Release/VST3/Starter\ Amp\ v0.1.vst3/Contents/moduleinfo.json /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/Release/VST3/Starter\ Amp\ v0.1.vst3/Contents/Resources/moduleinfo.json
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo "creating /Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Release/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
  /opt/homebrew/bin/cmake -E make_directory "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Release/VST3/Starter Amp v0.1.vst3/Contents/Resources"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/Release/juce_vst3_helper" -create -version 0.1.0 -path "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Release/VST3/Starter Amp v0.1.vst3" -output "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/Release/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter Amp v0.1.vst3/Contents"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake "-Dsrc=/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter Amp v0.1.vst3" -P "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-src/extras/Build/CMake/checkBundleSigning.cmake"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo removing\ moduleinfo.json
  /opt/homebrew/bin/cmake -E remove -f /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter\ Amp\ v0.1.vst3/Contents/moduleinfo.json /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter\ Amp\ v0.1.vst3/Contents/Resources/moduleinfo.json
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo "creating /Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
  /opt/homebrew/bin/cmake -E make_directory "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter Amp v0.1.vst3/Contents/Resources"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/MinSizeRel/juce_vst3_helper" -create -version 0.1.0 -path "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter Amp v0.1.vst3" -output "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/MinSizeRel/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E copy "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/JuceLibraryCode/StarterAmp_VST3/PkgInfo" "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter Amp v0.1.vst3/Contents"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake "-Dsrc=/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter Amp v0.1.vst3" -P "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/juce-src/extras/Build/CMake/checkBundleSigning.cmake"
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo removing\ moduleinfo.json
  /opt/homebrew/bin/cmake -E remove -f /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter\ Amp\ v0.1.vst3/Contents/moduleinfo.json /Users/connor/Downloads/StarterAmp_v0.1\ 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter\ Amp\ v0.1.vst3/Contents/Resources/moduleinfo.json
  cd "/Users/connor/Downloads/StarterAmp_v0.1 22/build"
  /opt/homebrew/bin/cmake -E echo "creating /Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
  /opt/homebrew/bin/cmake -E make_directory "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter Amp v0.1.vst3/Contents/Resources"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/RelWithDebInfo/juce_vst3_helper" -create -version 0.1.0 -path "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter Amp v0.1.vst3" -output "/Users/connor/Downloads/StarterAmp_v0.1 22/build/StarterAmp_artefacts/RelWithDebInfo/VST3/Starter Amp v0.1.vst3/Contents/Resources/moduleinfo.json"
fi


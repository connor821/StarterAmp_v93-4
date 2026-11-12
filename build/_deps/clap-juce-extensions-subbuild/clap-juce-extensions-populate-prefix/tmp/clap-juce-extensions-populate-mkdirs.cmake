# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-src")
  file(MAKE_DIRECTORY "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-src")
endif()
file(MAKE_DIRECTORY
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-build"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/tmp"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/src/clap-juce-extensions-populate-stamp"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/src"
  "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/src/clap-juce-extensions-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/src/clap-juce-extensions-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/connor/Downloads/StarterAmp_v0.1 22/build/_deps/clap-juce-extensions-subbuild/clap-juce-extensions-populate-prefix/src/clap-juce-extensions-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/deblomper/CppGram/build/_deps/td-src")
  file(MAKE_DIRECTORY "/home/deblomper/CppGram/build/_deps/td-src")
endif()
file(MAKE_DIRECTORY
  "/home/deblomper/CppGram/build/_deps/td-build"
  "/home/deblomper/CppGram/build/_deps/td-subbuild/td-populate-prefix"
  "/home/deblomper/CppGram/build/_deps/td-subbuild/td-populate-prefix/tmp"
  "/home/deblomper/CppGram/build/_deps/td-subbuild/td-populate-prefix/src/td-populate-stamp"
  "/home/deblomper/CppGram/build/_deps/td-subbuild/td-populate-prefix/src"
  "/home/deblomper/CppGram/build/_deps/td-subbuild/td-populate-prefix/src/td-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/deblomper/CppGram/build/_deps/td-subbuild/td-populate-prefix/src/td-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/deblomper/CppGram/build/_deps/td-subbuild/td-populate-prefix/src/td-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

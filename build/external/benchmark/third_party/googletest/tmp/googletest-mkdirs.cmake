# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/tehroux/MyProject/external/benchmark/googletest"
  "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest/build"
  "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest"
  "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest/tmp"
  "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest/src/googletest-stamp"
  "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest/download"
  "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest/src/googletest-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest/src/googletest-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/tehroux/MyProject/build/external/benchmark/third_party/googletest/src/googletest-stamp${cfgdir}") # cfgdir has leading slash
endif()

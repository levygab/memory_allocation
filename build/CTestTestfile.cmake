# CMake generated Testfile for 
# Source directory: /home/gabi/Documents/ecole/sepc/allocateur_memoire
# Build directory: /home/gabi/Documents/ecole/sepc/allocateur_memoire/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(AllTestsAllocator "alloctest")
set_tests_properties(AllTestsAllocator PROPERTIES  _BACKTRACE_TRIPLES "/home/gabi/Documents/ecole/sepc/allocateur_memoire/CMakeLists.txt;77;add_test;/home/gabi/Documents/ecole/sepc/allocateur_memoire/CMakeLists.txt;0;")
subdirs("gtest")

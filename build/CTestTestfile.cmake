# CMake generated Testfile for 
# Source directory: /home/autin/Desktop/project/Lumenshot
# Build directory: /home/autin/Desktop/project/Lumenshot/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(crop "/home/autin/Desktop/project/Lumenshot/build/test_crop")
set_tests_properties(crop PROPERTIES  _BACKTRACE_TRIPLES "/home/autin/Desktop/project/Lumenshot/CMakeLists.txt;38;add_test;/home/autin/Desktop/project/Lumenshot/CMakeLists.txt;0;")
add_test(masks "/home/autin/Desktop/project/Lumenshot/build/test_masks")
set_tests_properties(masks PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "/home/autin/Desktop/project/Lumenshot/CMakeLists.txt;41;add_test;/home/autin/Desktop/project/Lumenshot/CMakeLists.txt;0;")

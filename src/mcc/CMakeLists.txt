include_directories(${CMAKE_SOURCE_DIR}/src/mcc/include)

set(mcc_SRCS mcc.cpp)
add_executable(mcc ${mcc_SRCS})
target_link_libraries(mcc mcc-core cmdline)

set(test_cc_SRCS cc_test.cpp)
add_executable(test_cc ${test_cc_SRCS})
target_link_libraries(test_cc mcc-core)

add_subdirectory(core)
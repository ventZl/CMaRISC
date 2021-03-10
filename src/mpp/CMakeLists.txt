set(mpp_SRCS preprocess.cpp macro.cpp lexer.cpp expansion.cpp mpp.cpp)
add_executable(mpp ${mpp_SRCS})
target_link_libraries(mpp cmdline)
set(vmtest_SRCS vmtest.c)
add_executable(vmtest ${vmtest_SRCS})
target_link_libraries(vmtest vm cmdline)

#add_subdirectory(asm)
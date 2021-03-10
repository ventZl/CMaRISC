set(mdbg_SRCS mdbg.c)
add_executable(mdbg ${mdbg_SRCS})
target_link_libraries(mdbg vm cmdline object)
INSTALL(TARGETS mdbg RUNTIME DESTINATION bin)
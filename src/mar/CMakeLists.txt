set(mar_SRCS mar.c)
add_executable(mar ${mar_SRCS})
target_link_libraries(mar cmdline object)


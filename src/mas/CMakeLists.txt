set(mas_SRCS mas.c analyzer.c assembler.c)
add_executable(mas ${mas_SRCS})
target_link_libraries(mas cmdline object)
INSTALL(TARGETS mas RUNTIME DESTINATION bin)

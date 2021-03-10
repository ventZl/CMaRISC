set(ml_SRCS ml.c)
add_executable(ml ${ml_SRCS})
target_link_libraries(ml cmdline object)
INSTALL(TARGETS ml RUNTIME DESTINATION bin)
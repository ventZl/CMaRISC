add_custom_command(OUTPUT syntax.cpp COMMAND bison -d -o syntax.cpp ${CMAKE_SOURCE_DIR}/src/mcc/core/cc.y)
add_custom_command(OUTPUT lexical.cpp COMMAND lex -o lexical.cpp ${CMAKE_SOURCE_DIR}/src/mcc/core/cc.l)

set(mcc_SRCS syntax.cpp lexical.cpp compile.cpp nodes.cpp assembly.cpp)
add_library(mcc-core ${mcc_SRCS})



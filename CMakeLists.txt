cmake_minimum_required(VERSION 4.0)
project(Chess)

set(CMAKE_CXX_STANDARD 20)

add_executable(Chess main.cpp
        src/search.cpp
        src/search.h
        src/eval.cpp
        src/eval.h

        src/OpeningDB.cpp
        src/OpeningDB.h
        src/TranspositionTable.cpp
        src/TranspositionTable.h
        src/EndgameDB.cpp
        src/EndgameDB.h

        lib/surge/src/position.cpp
        lib/surge/src/position.h
        lib/surge/src/tables.cpp
        lib/surge/src/tables.h
        lib/surge/src/types.cpp
        lib/surge/src/types.h
        src/SearchThreadpool.cpp
        src/SearchThreadpool.h
)

add_library(fathom SHARED
        lib/Fathom/src/tbprobe.c
)

# Fathom uses C99 and needs PIC for a shared lib
set_target_properties(fathom PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
)

target_include_directories(fathom PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/Fathom/src
)

target_compile_options(fathom PRIVATE -std=gnu99 -O2 -Wall -Wshadow)

target_link_libraries(Chess PRIVATE fathom)
add_executable(Openings
        util/create_openings.cpp

        lib/surge/src/position.cpp
        lib/surge/src/position.h
        lib/surge/src/tables.cpp
        lib/surge/src/tables.h
        lib/surge/src/types.cpp
        lib/surge/src/types.h
)

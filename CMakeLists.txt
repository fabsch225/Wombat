cmake_minimum_required(VERSION 4.0)
project(Chess)

set(CMAKE_CXX_STANDARD 20)

add_executable(Chess main.cpp
        src/search.cpp
        src/eval.cpp
        src/movegen.cpp
        src/move.cpp
        src/ui.cpp
        src/transposition.cpp
        src/board.h
        src/constants.h
        src/eval.h
        src/move.h
        src/movegen.h
        src/search.h
        src/transposition.h
        src/ui.h
        src/openingdb.cpp
        src/openingdb.h
)

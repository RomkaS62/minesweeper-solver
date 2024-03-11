project(minesweeper-solver)

cmake_minimum_required(VERSION 3.16)

add_executable(mss main.c board.c buf.c)
target_link_libraries(mss PRIVATE gramas)

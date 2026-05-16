cmake_minimum_required(VERSION 3.13)
project(KSolve LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_library (KSolveAStar Game.cpp KSolveAStar.cpp GameStateMemory.cpp MoveStorage.cpp)

add_executable(unittests unittests.cpp)
target_link_libraries(unittests PRIVATE KSolveAStar)
add_executable(ran ran.cpp)
target_link_libraries(ran PRIVATE KSolveAStar)
add_executable(KSolve KSolve.cpp)
target_link_libraries(KSolve PRIVATE KSolveAStar)
add_executable(KSolve2Solvitaire KSolve2Solvitaire.cpp)
target_link_libraries(KSolve2Solvitaire PRIVATE KSolveAStar)

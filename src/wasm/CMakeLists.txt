# finish this up
# em++ main.cpp -o main.js   -s EXPORTED_FUNCTIONS='["_init","_player1_key_event", "_player2_key_event"]'   -s EXPORTED_RUNTIME_METHODS='["cwrap"]'   -s MODULARIZE=1  -s EXPORT_ES6=1 -O2
cmake_minimum_required(VERSION 3.28)

project(wasm)

set(CMAKE_CPP_COMPILER "em++")
set(CMAKE_CPP_FLAGS "-s EXPORTED_FUNCTIONS=['_fib'] -s EXPORTED_RUNTIME_METHODS=['cwrap'] -s EXPORT_ES6=1 -O2")

add_executable(main.js main.cpp)

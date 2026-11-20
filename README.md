## Prerequisites
emsdk is installed and environment is activated. Instructions can be found [here](https://emscripten.org/docs/getting_started/downloads.html)
Make sure that the following commands are run in a terminal that has the emscripten environment activated otherwise you will see 

## Checking it out
```sh
# ui
pnpm install
pnpm dev

# wasm build
cd src/wasm
em++ main.cpp -o main.js -s "EXPORTED_FUNCTIONS=['_init','_player1_input','_player2_input']" -s "EXPORTED_RUNTIME_METHODS=['cwrap']" -s ASYNCIFY -s MODULARIZE=1 -s EXPORT_ES6=1 -O2
```

## Windows
```bat
rem Run the build.bat for getting a build in the build directory
build.bat

rem If you also prefer running the python server to run the game, run the run.bat
run.bat
```

## todo checklist

### performance / tech
- shared memory buffer between cpp and js - reduce heap copying
- debug routes for info on js dashboard
- partial updation of the canvas heap memory (cpp)
- benchmarking and perf testing

### features
- curved entities and bouncing
- score counter
- bounce takes random angles and the ball speeds up (do playtesting)
- site hosting

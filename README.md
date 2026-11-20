## Prerequisites
Make sure you have Clang installed.

## Windows
```bat
rem Run the build.bat for getting a build in the build directory
build.bat

rem If you also prefer running the python server to run the game, run the run.bat
run.bat
```

## todo checklist

### Planned
- Plugin system
- Zig plugins that drives the ball upgrades
- browser detects and loads the plugins 
- shared memory buffer that holds the game state. Upon game load, the main game.wasm creates the GameState on the shared buffer.
- The rest of the plugins use it and make updates

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

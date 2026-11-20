@echo off

if not exist build (
	mkdir build
)

rem em++ src\wasm\main.cpp -o build\main.js -s "EXPORTED_FUNCTIONS=['_init','_player1_input','_player2_input']" -s "EXPORTED_RUNTIME_METHODS=['cwrap']" -s ASYNCIFY -s MODULARIZE=1 -s EXPORT_ES6=1 -O2

clang -Os -fno-builtin -Wall -Wextra -Wswitch-enum --target=wasm32 --no-standard-libraries -Wl,--export=game_init -Wl,--export=game_render -Wl,--export=game_update -Wl,--export=game_keydown -Wl,--export=game_keyup -Wl,--no-entry -Wl,--allow-undefined -o build\game.wasm src\wasm\game.c

if %errorlevel% neq 0 (
    echo Build failed! Exit Code %errorlevel%.
    pause
    exit /b %errorlevel%
)

echo F | xcopy static\index.html build\index.html /Y
echo F | xcopy static\wasm_main.js build\wasm_main.js /Y

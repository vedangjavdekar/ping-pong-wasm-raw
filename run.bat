@echo off

call build.bat

if %errorlevel% neq 0 (
    echo Build failed with exit code %errorlevel%.
    pause
    exit /b %errorlevel%
)

pushd build
python -m http.server 8000
popd

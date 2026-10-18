@echo off
echo ================================================
echo Building and extracting FreeSerf.wgt from Docker
echo ================================================
echo.

echo [1/6] Building Docker image...
docker build -t freeserf-tizen .
if errorlevel 1 (
    echo ERROR: Docker image build failed!
    pause
    exit /b 1
)

echo [2/6] Creating temporary container...
docker create --name freeserf-tmp freeserf-tizen
if errorlevel 1 (
    echo ERROR: Failed to create container!
    pause
    exit /b 1
)

echo [3/6] Starting container...
docker start freeserf-tmp
if errorlevel 1 (
    echo ERROR: Failed to start container!
    docker rm freeserf-tmp
    pause
    exit /b 1
)

echo [4/6] Waiting...
timeout /t 2 /nobreak >nul

echo [5/6] Copying FreeSerf.wgt...
docker cp freeserf-tmp:/home/freeserf/FreeSerf.wgt .
if errorlevel 1 (
    echo ERROR: Failed to copy file!
    docker stop freeserf-tmp
    docker rm freeserf-tmp
    pause
    exit /b 1
)

echo [6/6] Cleaning up...
docker stop freeserf-tmp
docker rm freeserf-tmp

echo.
echo ================================================
echo SUCCESS! FreeSerf.wgt extracted successfully.
echo ================================================
pause

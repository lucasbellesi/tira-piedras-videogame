# Tira Piedras (C++ / raylib)

Port del juego top-down “Tirapiedras” a C++ usando raylib.

## Requisitos
- CMake 3.25+
- Visual Studio 2019 Build Tools (x64)
- Windows SDK 10

## Build (Debug)
`powershell
cmake -S cpp-raylib -B build -G "Visual Studio 16 2019" -A x64 -T host=x64
cmake --build build --config Debug -j 12
`

## Ejecutar
Asegúrate de tener aylib.dll junto al exe (la build ya lo deja):
`powershell
Copy-Item build/_deps/raylib-build/raylib/Debug/raylib.dll build/Debug/ -ErrorAction SilentlyContinue
Start-Process -FilePath ".\build\Debug\tirapiedras.exe" -WorkingDirectory ".\build\Debug"
`

## Controles
- Click izquierdo: mover o recoger piedra
- Click derecho / Space: lanzar piedra
- R: reiniciar si perdiste

## Notas
- Las piedras aleatorias spawnean más rápido (2s)
- Enemigos aumentan 25% de velocidad por ciclo de dificultad
- 50% de probabilidad de soltar piedra grande al morir
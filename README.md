# Tira Piedras (C++ · raylib)

Port del juego top‑down "Tirapiedras" a C++ usando [raylib](https://www.raylib.com/).

## Características
- Movimiento por click/tap; recoger y lanzar piedras
- Dificultad por ciclos: +25% de velocidad de enemigos por ciclo
- 50% de probabilidad de que los enemigos suelten piedra grande al morir
- Spawns de piedras más rápidos (cada 2 s)
- HUD de puntaje, vidas, HP y banner de nivel

## Requisitos (Windows)
- CMake 3.25+
- Visual Studio 2019 Build Tools (x64) y Windows 10 SDK
- Git (opcional, para clonar/pushopear)

## Build (Debug)
`powershell
cmake -S cpp-raylib -B build -G "Visual Studio 16 2019" -A x64 -T host=x64
cmake --build build --config Debug -j 12
`

## Ejecutar
Asegúrate de que aylib.dll esté junto al ejecutable (la build normalmente lo deja):
`powershell
Copy-Item build/_deps/raylib-build/raylib/Debug/raylib.dll build/Debug/ -ErrorAction SilentlyContinue
Start-Process -FilePath ".\build\Debug\tirapiedras.exe" -WorkingDirectory ".\build\Debug"
`

## Controles
- Click izquierdo: mover o recoger piedra
- Click derecho / Space: lanzar piedra
- R: reiniciar si perdiste

## Configuración rápida
- Resolución: cpp-raylib/src/main.cpp:73 (W, H)
- Aparición de piedras: cpp-raylib/src/main.cpp:89 (ockSpawnInterval = 2.0f)
- Aumento de velocidad por dificultad: cpp-raylib/src/main.cpp (speedMultiplier = pow(1.25f, cycle))
- Drop de piedra al morir: cpp-raylib/src/main.cpp (50% en lógica de muerte de enemigo)

## Estructura del proyecto
- cpp-raylib/: CMake + código fuente del juego
  - CMakeLists.txt: obtiene raylib vía FetchContent
  - src/main.cpp: juego completo
- ersion html css js/: prototipo original HTML/JS de una sola página

## Origen del Proyecto
Este port en C++/raylib toma como base el prototipo HTML/JS de un solo archivo:
- [demo_html_top_down_piedras_js_canvas_single_file (2).html](version%20html%20css%20js/demo_html_top_down_piedras_js_canvas_single_file%20%282%29.html)

## Roadmap (ideas)
- Empaquetado Android (Gradle/NDK) con controles táctiles (doble‑tap para lanzar)
- Accesibilidad/ajustes: sliders de volumen, dificultad, sensibilidad
- SFX/Música y assets dedicados

## Créditos
- Librería: [raylib](https://github.com/raysan5/raylib)
- Prototipo original: HTML/JS single‑file (ver enlace arriba)

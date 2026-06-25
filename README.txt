QUORIDOR PAC-MAN — LP1
=======================

Implementación en C + raylib del TP "Quoridor Pac-Man"
------------------------------------------------------------
1) CÓMO COMPILAR
------------------------------------------------------------

Una vez descargados todos los archivos se debe crear una carpeta con el nombre "maps"
que contenga los cuatro archivos de texto de mapas(mapa1_clasico, mapa2_chico, mapa3_grande y personalizado)

Hace falta tener raylib instalado (https://www.raylib.com).

En Linux, si raylib ya está instalado en el sistema:

    gcc main.c game.c map.c ai.c editor.c -o quoridor_pacman \
        -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

En Windows con MinGW (siguiendo la guía oficial de raylib):

    gcc main.c game.c map.c ai.c editor.c -o quoridor_pacman.exe \
        -lraylib -lopengl32 -lgdi32 -lwinmm

En macOS:

    gcc main.c game.c map.c ai.c editor.c -o quoridor_pacman \
        -lraylib -framework OpenGL -framework Cocoa -framework IOKit

El programa crea solo, la primera vez que corre, una carpeta "maps/"
con los 3 mapas predefinidos (si la carpeta "maps/" ya viene incluida
en el .zip, los usa directamente).

------------------------------------------------------------
2) CÓMO JUGAR
------------------------------------------------------------

Pantalla de configuración:
  - Flechas arriba/abajo: elegir el campo a cambiar.
  - Flechas izquierda/derecha: cambiar el valor de ese campo.
  - ENTER: confirmar y pasar a elegir el mapa.

Pantalla de selección de mapa:
  - Flechas arriba/abajo: navegar.
  - ENTER: cargar el mapa elegido y arrancar la partida (o abrir el
    editor si se elige la última opción).
  - ESC: volver a la configuración.

Durante la partida:
  - Flechas: mover a Pac-Man (o al fantasma, en modo Player vs Player).
  - M: activa el "modo muro" (el cursor aparece sobre tu personaje),las flechas
    mueven el cursor libremente por toda la grilla.
  - W + flecha: confirma en qué lado de la celda del cursor va el muro. M de nuevo
    cancela sin gastar muro.
  - ENTER: termina el turno de Pac-Man aunque le queden acciones.
  - R: reinicia la partida actual (mismo mapa y configuración).
  - ESC: sale del programa.

  Pac-Man tiene 2 acciones por turno (3 si comió una pac-bola en ese
  turno). Cada acción es "avanzar 1 casilla" o "colocar 1 muro"; se
  pueden combinar libremente y se puede girar entre una acción y otra.

  Cada fantasma, en su turno, hace UNA sola cosa: se mueve o coloca un
  muro (colocar muro le gasta el turno completo, no se mueve ese turno).
  En modo "IA vs Player" esto lo decide la computadora sola (con un
  pequeño retraso para que se pueda ver jugada por jugada). En modo
  "Player vs Player" el segundo jugador controla a cada fantasma, uno
  por uno, con las mismas teclas (flechas para moverse, M + flecha
  para poner un muro).

  Modo frenético: si al empezar su turno un fantasma ve a Pac-Man en
  línea recta sin muros en el medio, y elige moverse, se mueve 2
  casillas en lugar de 1 (tanto en IA como con jugador humano).

Pantalla de fin de partida:
  - ENTER: jugar de nuevo con la misma configuración y mapa.
  - ESC: volver a la pantalla de configuración.

------------------------------------------------------------
3) EDITOR DE MAPAS
------------------------------------------------------------

Se accede desde la pantalla de selección de mapa, última opción.

  - Flechas: mover el cursor por la grilla.
  - 1: pone a Pac-Man en la celda del cursor.
  - 2, 3, 4, 5: pone a Blinky, Inky, Pinky y Clyde respectivamente.
  - 6, 7, 8, 9: pone las 4 pac-bolas.
  - W + flecha: agrega o quita un muro PERMANENTE en el lado de la
    celda indicado por la flecha (si ya hay uno, lo borra).
  - A / Z: agrega / quita una fila (mínimo 5, máximo 20).
  - S / X: agrega / quita una columna (mínimo 5, máximo 20).
  - ENTER: guarda el mapa en maps/personalizado.txt y vuelve a la
    selección de mapas (ya queda disponible para elegir y jugar).
  - ESC: vuelve a la selección de mapas SIN guardar.

Un mapa guardado contiene: tamaño del tablero, posiciones iniciales de
Pac-Man, los 4 fantasmas y las 4 pac-bolas, y los muros permanentes.
Los muros temporales NO se guardan en el mapa: son siempre parte de
una partida en curso, se colocan y se gastan durante el juego.

------------------------------------------------------------
4) QUÉ SIGNIFICA LA DIFICULTAD DE CADA FANTASMA
------------------------------------------------------------

La dificultad (1 = fácil, 2 = medio, 3 = difícil) sólo afecta al modo
"IA vs Player" (en Player vs Player el fantasma lo maneja una persona,
así que no aplica). Cambia dos cosas: qué tan bien persigue a Pac-Man,
y qué tan seguido prefiere colocar un muro en vez de moverse.

  - Fácil (1): se mueve en una dirección válida CUALQUIERA (no
    persigue a Pac-Man). Coloca un muro ~15% de las veces que le toca
    jugar (si todavía tiene muros en la mano).
  - Medio (2): mira las 4 direcciones posibles y elige la que más
    achica la distancia (en línea recta) hacia Pac-Man. No calcula el
    camino completo, así que se puede trabar contra muros largos.
    Coloca un muro ~20% de las veces.
  - Difícil (3): calcula el camino MÁS CORTO real hacia Pac-Man con
    un recorrido en anchura (BFS) que respeta todos los muros del
    tablero, y avanza el primer paso de ese camino. Coloca un muro
    ~25% de las veces, lo que sumado al buen movimiento lo hace
    notablemente más peligroso.

En los tres casos, si en un turno la IA decide colocar un muro, busca
un lugar válido (que no tenga ya un muro) adyacente a su posición; si
no tiene muros en la mano, esa jugada nunca se intenta y directamente
elige moverse.


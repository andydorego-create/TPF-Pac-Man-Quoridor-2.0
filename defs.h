#ifndef DEFS_H
#define DEFS_H

#include <stdbool.h>

/* ---------- Constantes generales ---------- */
#define MAX_SIZE     20   /* tamaño máximo de grilla soportado */
#define NUM_GHOSTS   4
#define NUM_BALLS    4
#define MAX_WALLS_HAND 20

/* Tipos de muro en una celda de la matriz de muros */
#define WALL_NONE      0
#define WALL_PERM      1
#define WALL_TEMP      2

/* Dueños de un muro temporal (para devolverlo a la mano correcta) */
#define OWNER_PACMAN   0
#define OWNER_GHOST    1

/* Pantallas / estados del programa */
typedef enum {
    SCREEN_CONFIG,      /* pantalla de configuración inicial      */
    SCREEN_MAP_SELECT,  /* elegir mapa predefinido / cargar uno    */
    SCREEN_EDITOR,      /* editor de mapas                         */
    SCREEN_PLAYING,     /* partida en curso                        */
    SCREEN_GAMEOVER     /* fin de partida                          */
} Screen;

typedef enum {
    MODE_AI_VS_PLAYER,
    MODE_PVP
} GameMode;

typedef struct {
    int row, col;
} Cell;

/* ---------- Datos de un mapa ---------- */
typedef struct {
    int rows, cols;

    /* wallH[r][c] = muro permanente entre la celda (r,c) y (r+1,c) */
    /* wallV[r][c] = muro permanente entre la celda (r,c) y (r,c+1) */
    int wallH[MAX_SIZE][MAX_SIZE];
    int wallV[MAX_SIZE][MAX_SIZE];

    Cell pacmanStart;
    Cell ghostStart[NUM_GHOSTS];
    Cell ballStart[NUM_BALLS];
} MapData;

/* ---------- Muro temporal individual colocado en partida ---------- */
/* Reutilizamos las mismas matrices que el mapa, pero les agregamos   */
/* "vida" y "dueño" para los muros temporales puestos durante el juego */
typedef struct {
    int typeH[MAX_SIZE][MAX_SIZE];   /* WALL_NONE / WALL_PERM / WALL_TEMP */
    int typeV[MAX_SIZE][MAX_SIZE];
    int lifeH[MAX_SIZE][MAX_SIZE];   /* turnos globales restantes (temp) */
    int lifeV[MAX_SIZE][MAX_SIZE];
    int ownerH[MAX_SIZE][MAX_SIZE];  /* OWNER_PACMAN / OWNER_GHOST */
    int ownerV[MAX_SIZE][MAX_SIZE];
} Board;

/* ---------- Configuración elegida antes de jugar ---------- */
typedef struct {
    GameMode mode;
    bool ghostEnabled[NUM_GHOSTS];
    int  ghostDifficulty[NUM_GHOSTS]; /* 1 = fácil, 2 = medio, 3 = difícil */
    int  pacWallsHand;
    int  ghostWallsHand;
    int  wallLifetime;                /* turnos globales que dura un muro temporal */
} Config;

/* ---------- Estado completo de una partida en curso ---------- */
typedef struct {
    MapData map;
    Board   board;
    Config  cfg;

    Cell pacman;
    int  pacLives;

    Cell ghosts[NUM_GHOSTS];
    bool ghostAlive[NUM_GHOSTS];   /* false = comido, fuera del tablero */

    Cell balls[NUM_BALLS];
    bool ballEaten[NUM_BALLS];
    int  ballsEatenCount;

    int pacWallsLeft;
    int ghostWallsLeft;

    int  turnOwner;        /* 0 = Pac-Man, 1 = fantasmas */
    int  actionsLeft;      /* acciones restantes de Pac-Man en su turno */
    int  ghostTurnIndex;   /* en PvP / AI: qué fantasma está jugando ahora */
    int  globalTurn;       /* contador de turnos globales (para vida de muros) */

    int  winner;           /* 0 = nadie, 1 = Pac-Man, 2 = fantasmas */
    bool pacAteBallThisTurn; /* si comió una pac-bola en este turno (para saber si come fantasma) */

    /* estado auxiliar de input */
    bool placingWall;      /* true si el jugador apretó M: muestra el cursor de muro */
    Cell wallCursor;       /* celda donde está el cursor de colocación de muro (se mueve libre por todo el tablero) */
    bool waitingWallSide;  /* true tras apretar W: la próxima flecha indica el lado del cursor donde va el muro */
    float aiTimer;         /* retardo entre jugadas de la IA, para que se vea */

    /* navegación de menúes (config y selección de mapa reusan este índice) */
    int menuIndex;

    /* estado del editor de mapas */
    MapData editorMap;
    Cell    editorCursor;
    bool    editorWaitingWallDir;
} Game;

#endif

#ifndef AI_H
#define AI_H

#include "defs.h"

/* Decisión que toma un fantasma en su turno (sólo decide, no ejecuta;
   la ejecución la hace game.c con la misma función que usa el jugador
   humano en modo PvP, para que las reglas se apliquen siempre igual). */
typedef struct {
    bool placeWall;   /* true = colocar muro, false = moverse */
    int  dirRow;       /* -1, 0 o 1 */
    int  dirCol;       /* -1, 0 o 1 */
} GhostDecision;

/* Decide qué hace el fantasma `idx` según su dificultad configurada. */
GhostDecision AI_Decidir(Game *g, int idx);

#endif

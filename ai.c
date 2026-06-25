#include <stdlib.h>
#include "ai.h"
#include "map.h"

static const int DR[4] = { -1, 1, 0, 0 };
static const int DC[4] = { 0, 0, -1, 1 };

static bool EnTablero(Game *g, Cell c) {
    return c.row >= 0 && c.row < g->map.rows && c.col >= 0 && c.col < g->map.cols;
}

static bool MovimientoValido(Game *g, Cell desde, int dr, int dc) {
    Cell hacia = { desde.row + dr, desde.col + dc };
    if (!EnTablero(g, hacia)) return false;
    if (Board_HayMuroEntre(&g->board, desde, hacia)) return false;
    return true;
}

/* Distancia Manhattan, usada por la dificultad media para elegir la
   dirección "que más acerca" sin hacer pathfinding completo. */
static int DistManhattan(Cell a, Cell b) {
    int dr = a.row - b.row; if (dr < 0) dr = -dr;
    int dc = a.col - b.col; if (dc < 0) dc = -dc;
    return dr + dc;
}

/* BFS simple sobre la grilla respetando muros, para la dificultad difícil.
   Devuelve la primera dirección del camino más corto hacia Pac-Man, o
   {0,0} si no encuentra camino (no debería pasar en mapas normales). */
static void PrimerPasoBFS(Game *g, Cell origen, Cell destino, int *outDr, int *outDc) {
    int rows = g->map.rows, cols = g->map.cols;
    bool visitado[MAX_SIZE][MAX_SIZE];
    Cell prev[MAX_SIZE][MAX_SIZE];
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) visitado[r][c] = false;

    Cell cola[MAX_SIZE * MAX_SIZE];
    int ini = 0, fin = 0;
    cola[fin++] = origen;
    visitado[origen.row][origen.col] = true;

    bool encontrado = false;
    while (ini < fin && !encontrado) {
        Cell actual = cola[ini++];
        for (int d = 0; d < 4; d++) {
            Cell vecino = { actual.row + DR[d], actual.col + DC[d] };
            if (!EnTablero(g, vecino)) continue;
            if (visitado[vecino.row][vecino.col]) continue;
            if (Board_HayMuroEntre(&g->board, actual, vecino)) continue;
            visitado[vecino.row][vecino.col] = true;
            prev[vecino.row][vecino.col] = actual;
            cola[fin++] = vecino;
            if (vecino.row == destino.row && vecino.col == destino.col) { encontrado = true; break; }
        }
    }

    *outDr = 0; *outDc = 0;
    if (!encontrado) return;

    /* reconstruir el camino hacia atrás hasta encontrar el primer paso */
    Cell paso = destino;
    while (!(prev[paso.row][paso.col].row == origen.row && prev[paso.row][paso.col].col == origen.col)) {
        paso = prev[paso.row][paso.col];
    }
    *outDr = paso.row - origen.row;
    *outDc = paso.col - origen.col;
}

/* Intenta encontrar un lugar válido para poner un muro cerca del fantasma. */
static bool BuscarMuroValido(Game *g, Cell desde, int *dr, int *dc) {
    int orden[4] = {0,1,2,3};
    /* desordenar un poco */
    for (int i = 0; i < 4; i++) {
        int j = rand() % 4;
        int tmp = orden[i]; orden[i] = orden[j]; orden[j] = tmp;
    }
    for (int i = 0; i < 4; i++) {
        int d = orden[i];
        if (MovimientoValido(g, desde, DR[d], DC[d])) {
            *dr = DR[d]; *dc = DC[d];
            return true;
        }
    }
    return false;
}

GhostDecision AI_Decidir(Game *g, int idx) {
    GhostDecision dec = { false, 0, 0 };
    Cell yo = g->ghosts[idx];
    int dificultad = g->cfg.ghostDifficulty[idx];

    bool tieneMuros = g->ghostWallsLeft > 0;

    /* probabilidad de elegir colocar un muro en vez de moverse */
    int probMuro = (dificultad == 1) ? 15 : (dificultad == 2) ? 20 : 25;
    if (tieneMuros && (rand() % 100) < probMuro) {
        int dr, dc;
        if (BuscarMuroValido(g, yo, &dr, &dc)) {
            dec.placeWall = true;
            dec.dirRow = dr;
            dec.dirCol = dc;
            return dec;
        }
    }

    /* si no colocó muro, decide hacia dónde moverse según la dificultad */
    if (dificultad == 1) {
        /* fácil: dirección aleatoria entre las válidas */
        int orden[4] = {0,1,2,3};
        for (int i = 0; i < 4; i++) { int j = rand()%4; int t=orden[i]; orden[i]=orden[j]; orden[j]=t; }
        for (int i = 0; i < 4; i++) {
            int d = orden[i];
            if (MovimientoValido(g, yo, DR[d], DC[d])) {
                dec.dirRow = DR[d]; dec.dirCol = DC[d];
                return dec;
            }
        }
    } else if (dificultad == 2) {
        /* medio: prueba la dirección que más reduce la distancia a Pac-Man */
        int mejorDist = 999, mejorD = -1;
        int orden[4] = {0,1,2,3};
        for (int i = 0; i < 4; i++) { int j = rand()%4; int t=orden[i]; orden[i]=orden[j]; orden[j]=t; }
        for (int i = 0; i < 4; i++) {
            int d = orden[i];
            if (!MovimientoValido(g, yo, DR[d], DC[d])) continue;
            Cell destino = { yo.row + DR[d], yo.col + DC[d] };
            int dist = DistManhattan(destino, g->pacman);
            if (dist < mejorDist) { mejorDist = dist; mejorD = d; }
        }
        if (mejorD >= 0) { dec.dirRow = DR[mejorD]; dec.dirCol = DC[mejorD]; }
    } else {
        /* difícil: camino más corto real (BFS) respetando muros */
        int dr, dc;
        PrimerPasoBFS(g, yo, g->pacman, &dr, &dc);
        if (dr == 0 && dc == 0) {
            /* no encontró camino: fallback aleatorio */
            int orden[4] = {0,1,2,3};
            for (int i = 0; i < 4; i++) { int j = rand()%4; int t=orden[i]; orden[i]=orden[j]; orden[j]=t; }
            for (int i = 0; i < 4; i++) {
                int d = orden[i];
                if (MovimientoValido(g, yo, DR[d], DC[d])) { dr = DR[d]; dc = DC[d]; break; }
            }
        }
        dec.dirRow = dr; dec.dirCol = dc;
    }

    return dec;
}

#include <stdio.h>
#include <string.h>
#include "map.h"

/* ----------------------------------------------------------------------- */
/* Formato de archivo de mapa (texto plano, simple de leer a mano):        */
/*                                                                         */
/*   filas columnas                                                       */
/*   pacRow pacCol                                                        */
/*   ghost0Row ghost0Col                                                  */
/*   ghost1Row ghost1Col                                                  */
/*   ghost2Row ghost2Col                                                  */
/*   ghost3Row ghost3Col                                                  */
/*   ball0Row ball0Col                                                    */
/*   ball1Row ball1Col                                                    */
/*   ball2Row ball2Col                                                    */
/*   ball3Row ball3Col                                                    */
/*   cantidadDeMurosPermanentes                                           */
/*   H fila col      <- muro permanente entre (fila,col) y (fila+1,col)   */
/*   V fila col      <- muro permanente entre (fila,col) y (fila,col+1)   */
/*   ...                                                                  */
/* ----------------------------------------------------------------------- */

void Map_Vacio(MapData *map, int rows, int cols) {
    map->rows = rows;
    map->cols = cols;
    for (int r = 0; r < MAX_SIZE; r++)
        for (int c = 0; c < MAX_SIZE; c++) {
            map->wallH[r][c] = WALL_NONE;
            map->wallV[r][c] = WALL_NONE;
        }
    map->pacmanStart.row = rows / 2;
    map->pacmanStart.col = cols / 2;
    for (int i = 0; i < NUM_GHOSTS; i++) {
        map->ghostStart[i].row = 0;
        map->ghostStart[i].col = 0;
    }
    for (int i = 0; i < NUM_BALLS; i++) {
        map->ballStart[i].row = 0;
        map->ballStart[i].col = 0;
    }
}

bool Map_Cargar(const char *path, MapData *map) {
    FILE *f = fopen(path, "r");
    if (!f) return false;

    int rows, cols;
    if (fscanf(f, "%d %d", &rows, &cols) != 2) { fclose(f); return false; }
    if (rows < 1 || rows > MAX_SIZE || cols < 1 || cols > MAX_SIZE) { fclose(f); return false; }

    Map_Vacio(map, rows, cols); /* limpia matrices de muros */

    if (fscanf(f, "%d %d", &map->pacmanStart.row, &map->pacmanStart.col) != 2) { fclose(f); return false; }
    for (int i = 0; i < NUM_GHOSTS; i++)
        if (fscanf(f, "%d %d", &map->ghostStart[i].row, &map->ghostStart[i].col) != 2) { fclose(f); return false; }
    for (int i = 0; i < NUM_BALLS; i++)
        if (fscanf(f, "%d %d", &map->ballStart[i].row, &map->ballStart[i].col) != 2) { fclose(f); return false; }

    int numWalls = 0;
    if (fscanf(f, "%d", &numWalls) != 1) { fclose(f); return false; }
    for (int i = 0; i < numWalls; i++) {
        char tipo;
        int r, c;
        if (fscanf(f, " %c %d %d", &tipo, &r, &c) != 3) break; /* línea corrupta: dejamos de leer muros */
        if (r < 0 || r >= MAX_SIZE || c < 0 || c >= MAX_SIZE) continue; /* índice inválido: se ignora */
        if (tipo == 'H' || tipo == 'h')      map->wallH[r][c] = WALL_PERM;
        else if (tipo == 'V' || tipo == 'v') map->wallV[r][c] = WALL_PERM;
        /* cualquier otro carácter de tipo: se ignora esa línea */
    }

    fclose(f);
    return true;
}

bool Map_Guardar(const char *path, const MapData *map) {
    FILE *f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "%d %d\n", map->rows, map->cols);
    fprintf(f, "%d %d\n", map->pacmanStart.row, map->pacmanStart.col);
    for (int i = 0; i < NUM_GHOSTS; i++)
        fprintf(f, "%d %d\n", map->ghostStart[i].row, map->ghostStart[i].col);
    for (int i = 0; i < NUM_BALLS; i++)
        fprintf(f, "%d %d\n", map->ballStart[i].row, map->ballStart[i].col);

    int numWalls = 0;
    for (int r = 0; r < map->rows; r++)
        for (int c = 0; c < map->cols; c++) {
            if (map->wallH[r][c] == WALL_PERM) numWalls++;
            if (map->wallV[r][c] == WALL_PERM) numWalls++;
        }
    fprintf(f, "%d\n", numWalls);
    for (int r = 0; r < map->rows; r++)
        for (int c = 0; c < map->cols; c++) {
            if (map->wallH[r][c] == WALL_PERM) fprintf(f, "H %d %d\n", r, c);
            if (map->wallV[r][c] == WALL_PERM) fprintf(f, "V %d %d\n", r, c);
        }

    fclose(f);
    return true;
}

/* ----------------------------------------------------------------------- */
/* Generación de los 3 mapas predefinidos pedidos por el enunciado.        */
/* Se escriben a disco una sola vez (si no existen) para que después se    */
/* carguen con la misma función Map_Cargar que cualquier otro mapa.        */
/* ----------------------------------------------------------------------- */

static bool ArchivoExiste(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) { fclose(f); return true; }
    return false;
}

static void CrearMapa1(void) {
    /* 9x9 clásico, con algunos muros sueltos */
    MapData m;
    Map_Vacio(&m, 9, 9);
    m.pacmanStart.row = 4; m.pacmanStart.col = 4;
    m.ghostStart[0].row = 0; m.ghostStart[0].col = 0;
    m.ghostStart[1].row = 0; m.ghostStart[1].col = 8;
    m.ghostStart[2].row = 8; m.ghostStart[2].col = 0;
    m.ghostStart[3].row = 8; m.ghostStart[3].col = 8;
    m.ballStart[0].row = 1; m.ballStart[0].col = 1;
    m.ballStart[1].row = 1; m.ballStart[1].col = 7;
    m.ballStart[2].row = 7; m.ballStart[2].col = 1;
    m.ballStart[3].row = 7; m.ballStart[3].col = 7;

    m.wallV[3][3] = WALL_PERM;
    m.wallV[3][4] = WALL_PERM;
    m.wallH[2][5] = WALL_PERM;
    m.wallH[5][2] = WALL_PERM;
    m.wallV[6][5] = WALL_PERM;

    Map_Guardar("maps/mapa1_clasico.txt", &m);
}

static void CrearMapa2(void) {
    /* 7x7, tablero chico para partidas rápidas */
    MapData m;
    Map_Vacio(&m, 7, 7);
    m.pacmanStart.row = 3; m.pacmanStart.col = 3;
    m.ghostStart[0].row = 0; m.ghostStart[0].col = 0;
    m.ghostStart[1].row = 0; m.ghostStart[1].col = 6;
    m.ghostStart[2].row = 6; m.ghostStart[2].col = 0;
    m.ghostStart[3].row = 6; m.ghostStart[3].col = 6;
    m.ballStart[0].row = 1; m.ballStart[0].col = 3;
    m.ballStart[1].row = 5; m.ballStart[1].col = 3;
    m.ballStart[2].row = 3; m.ballStart[2].col = 1;
    m.ballStart[3].row = 3; m.ballStart[3].col = 5;

    m.wallH[1][1] = WALL_PERM;
    m.wallH[4][4] = WALL_PERM;
    m.wallV[2][2] = WALL_PERM;

    Map_Guardar("maps/mapa2_chico.txt", &m);
}

static void CrearMapa3(void) {
    /* 11x11, tablero grande con un "cruz" de muros en el medio */
    MapData m;
    Map_Vacio(&m, 11, 11);
    m.pacmanStart.row = 5; m.pacmanStart.col = 5;
    m.ghostStart[0].row = 0; m.ghostStart[0].col = 0;
    m.ghostStart[1].row = 0; m.ghostStart[1].col = 10;
    m.ghostStart[2].row = 10; m.ghostStart[2].col = 0;
    m.ghostStart[3].row = 10; m.ghostStart[3].col = 10;
    m.ballStart[0].row = 2; m.ballStart[0].col = 2;
    m.ballStart[1].row = 2; m.ballStart[1].col = 8;
    m.ballStart[2].row = 8; m.ballStart[2].col = 2;
    m.ballStart[3].row = 8; m.ballStart[3].col = 8;

    for (int c = 3; c < 7; c++) m.wallH[4][c] = WALL_PERM;
    for (int r = 3; r < 7; r++) m.wallV[r][4] = WALL_PERM;

    Map_Guardar("maps/mapa3_grande.txt", &m);
}

void Map_CrearPredefinidosSiNoExisten(void) {
    if (!ArchivoExiste("maps/mapa1_clasico.txt")) CrearMapa1();
    if (!ArchivoExiste("maps/mapa2_chico.txt"))   CrearMapa2();
    if (!ArchivoExiste("maps/mapa3_grande.txt"))  CrearMapa3();
}

/* ----------------------------------------------------------------------- */
/* Lógica de muros en partida (Board)                                     */
/* ----------------------------------------------------------------------- */

void Board_Inicializar(Board *b, const MapData *map) {
    for (int r = 0; r < MAX_SIZE; r++)
        for (int c = 0; c < MAX_SIZE; c++) {
            b->typeH[r][c] = map->wallH[r][c];
            b->typeV[r][c] = map->wallV[r][c];
            b->lifeH[r][c] = 0;
            b->lifeV[r][c] = 0;
            b->ownerH[r][c] = 0;
            b->ownerV[r][c] = 0;
        }
}

/* Dado dos celdas adyacentes (a y c difieren en 1 solo en row o col),
   indica si hay un muro (de cualquier tipo) bloqueando el paso entre ellas. */
bool Board_HayMuroEntre(const Board *b, Cell a, Cell c) {
    if (a.row == c.row) {
        /* movimiento horizontal -> wallV en la columna menor */
        int col = (a.col < c.col) ? a.col : c.col;
        return b->typeV[a.row][col] != WALL_NONE;
    } else {
        /* movimiento vertical -> wallH en la fila menor */
        int row = (a.row < c.row) ? a.row : c.row;
        return b->typeH[row][a.col] != WALL_NONE;
    }
}

bool Board_ColocarMuroTemporal(Board *b, Cell a, Cell c, int owner) {
    if (Board_HayMuroEntre(b, a, c)) return false; /* ya hay muro ahí */

    if (a.row == c.row) {
        int col = (a.col < c.col) ? a.col : c.col;
        b->typeV[a.row][col]  = WALL_TEMP;
        b->lifeV[a.row][col]  = -1; /* se setea afuera con la vida configurada */
        b->ownerV[a.row][col] = owner;
    } else {
        int row = (a.row < c.row) ? a.row : c.row;
        b->typeH[row][a.col]  = WALL_TEMP;
        b->lifeH[row][a.col]  = -1;
        b->ownerH[row][a.col] = owner;
    }
    return true;
}

void Board_AvanzarTurnoMuros(Board *b, int rows, int cols, int *expiradosPac, int *expiradosGhost) {
    *expiradosPac = 0;
    *expiradosGhost = 0;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) {
            if (b->typeH[r][c] == WALL_TEMP) {
                b->lifeH[r][c]--;
                if (b->lifeH[r][c] <= 0) {
                    b->typeH[r][c] = WALL_NONE;
                    if (b->ownerH[r][c] == OWNER_PACMAN) (*expiradosPac)++;
                    else (*expiradosGhost)++;
                }
            }
            if (b->typeV[r][c] == WALL_TEMP) {
                b->lifeV[r][c]--;
                if (b->lifeV[r][c] <= 0) {
                    b->typeV[r][c] = WALL_NONE;
                    if (b->ownerV[r][c] == OWNER_PACMAN) (*expiradosPac)++;
                    else (*expiradosGhost)++;
                }
            }
        }
}

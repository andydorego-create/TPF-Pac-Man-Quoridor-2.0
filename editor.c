#include "raylib.h"
#include "editor.h"
#include "map.h"

#define CELL 28
#define ORIGEN_X 40
#define ORIGEN_Y 90

void Editor_Iniciar(Game *g) {
    /* arranca de un mapa 9x9 vacío; si ya existe un mapa personalizado
       guardado antes, lo carga para poder seguir editándolo */
    if (!Map_Cargar("maps/personalizado.txt", &g->editorMap)) {
        Map_Vacio(&g->editorMap, 9, 9);
    }
    g->editorCursor.row = 0;
    g->editorCursor.col = 0;
    g->editorWaitingWallDir = false;
}

static void ToggleMuro(MapData *m, Cell cur, int dr, int dc) {
    Cell vecino = { cur.row + dr, cur.col + dc };
    if (vecino.row < 0 || vecino.row >= m->rows || vecino.col < 0 || vecino.col >= m->cols) return;

    if (dr != 0) {
        int fila = (cur.row < vecino.row) ? cur.row : vecino.row;
        m->wallH[fila][cur.col] = (m->wallH[fila][cur.col] == WALL_PERM) ? WALL_NONE : WALL_PERM;
    } else {
        int col = (cur.col < vecino.col) ? cur.col : vecino.col;
        m->wallV[cur.row][col] = (m->wallV[cur.row][col] == WALL_PERM) ? WALL_NONE : WALL_PERM;
    }
}

void Editor_Update(Game *g) {
    MapData *m = &g->editorMap;

    if (IsKeyPressed(KEY_ESCAPE)) {
        g->menuIndex = 0;
        /* no se guarda */
        extern Screen pantallaActual;
        pantallaActual = SCREEN_MAP_SELECT;
        return;
    }
    if (IsKeyPressed(KEY_ENTER)) {
        Map_Guardar("maps/personalizado.txt", m);
        extern Screen pantallaActual;
        pantallaActual = SCREEN_MAP_SELECT;
        return;
    }

    /* cambiar tamaño del tablero */
    if (IsKeyPressed(KEY_A) && m->rows < MAX_SIZE) m->rows++;
    if (IsKeyPressed(KEY_Z) && m->rows > 5) m->rows--;
    if (IsKeyPressed(KEY_S) && m->cols < MAX_SIZE) m->cols++;
    if (IsKeyPressed(KEY_X) && m->cols > 5) m->cols--;
    if (g->editorCursor.row >= m->rows) g->editorCursor.row = m->rows - 1;
    if (g->editorCursor.col >= m->cols) g->editorCursor.col = m->cols - 1;

    /* colocar muro permanente: W y luego una flecha indica el lado */
    if (IsKeyPressed(KEY_W)) g->editorWaitingWallDir = true;

    int dr = 0, dc = 0;
    bool flecha = false;
    if (IsKeyPressed(KEY_UP))    { dr = -1; dc = 0; flecha = true; }
    if (IsKeyPressed(KEY_DOWN))  { dr = 1;  dc = 0; flecha = true; }
    if (IsKeyPressed(KEY_LEFT))  { dr = 0;  dc = -1; flecha = true; }
    if (IsKeyPressed(KEY_RIGHT)) { dr = 0;  dc = 1;  flecha = true; }

    if (flecha) {
        if (g->editorWaitingWallDir) {
            ToggleMuro(m, g->editorCursor, dr, dc);
            g->editorWaitingWallDir = false;
        } else {
            int nr = g->editorCursor.row + dr;
            int nc = g->editorCursor.col + dc;
            if (nr >= 0 && nr < m->rows) g->editorCursor.row = nr;
            if (nc >= 0 && nc < m->cols) g->editorCursor.col = nc;
        }
    }

    /* colocar elementos: 1=Pac-Man, 2-5=fantasmas, 6-9=pac-bolas */
    if (IsKeyPressed(KEY_ONE))   m->pacmanStart = g->editorCursor;
    if (IsKeyPressed(KEY_TWO))   m->ghostStart[0] = g->editorCursor;
    if (IsKeyPressed(KEY_THREE)) m->ghostStart[1] = g->editorCursor;
    if (IsKeyPressed(KEY_FOUR))  m->ghostStart[2] = g->editorCursor;
    if (IsKeyPressed(KEY_FIVE))  m->ghostStart[3] = g->editorCursor;
    if (IsKeyPressed(KEY_SIX))   m->ballStart[0] = g->editorCursor;
    if (IsKeyPressed(KEY_SEVEN)) m->ballStart[1] = g->editorCursor;
    if (IsKeyPressed(KEY_EIGHT)) m->ballStart[2] = g->editorCursor;
    if (IsKeyPressed(KEY_NINE)) m->ballStart[3] = g->editorCursor;
}

void Editor_Draw(Game *g) {
    MapData *m = &g->editorMap;

    DrawText("EDITOR DE MAPAS", 40, 15, 28, RAYWHITE);
    DrawText("1 PacMan | 2-5 Fantasmas | 6-9 Pac-bolas | W+flecha: muro | A/Z filas  S/X columnas | ENTER guardar | ESC volver",
             40, 50, 14, LIGHTGRAY);

    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++) {
            int x = ORIGEN_X + c * CELL;
            int y = ORIGEN_Y + r * CELL;
            DrawRectangleLines(x, y, CELL, CELL, GRAY);
        }
    }

    /* muros permanentes */
    for (int r = 0; r < m->rows; r++)
        for (int c = 0; c < m->cols; c++) {
            if (m->wallH[r][c] == WALL_PERM) {
                int x = ORIGEN_X + c * CELL;
                int y = ORIGEN_Y + (r + 1) * CELL;
                DrawRectangle(x, y - 3, CELL, 6, ORANGE);
            }
            if (m->wallV[r][c] == WALL_PERM) {
                int x = ORIGEN_X + (c + 1) * CELL;
                int y = ORIGEN_Y + r * CELL;
                DrawRectangle(x - 3, y, 6, CELL, ORANGE);
            }
        }

    /* elementos */
    for (int i = 0; i < NUM_BALLS; i++) {
        int x = ORIGEN_X + m->ballStart[i].col * CELL + CELL/2;
        int y = ORIGEN_Y + m->ballStart[i].row * CELL + CELL/2;
        DrawCircle(x, y, 4, PINK);
    }
    for (int i = 0; i < NUM_GHOSTS; i++) {
        Color colores[4] = { RED, SKYBLUE, MAGENTA, ORANGE };
        int x = ORIGEN_X + m->ghostStart[i].col * CELL;
        int y = ORIGEN_Y + m->ghostStart[i].row * CELL;
        DrawRectangle(x + 3, y + 3, CELL - 6, CELL - 6, colores[i]);
    }
    {
        int x = ORIGEN_X + m->pacmanStart.col * CELL + CELL/2;
        int y = ORIGEN_Y + m->pacmanStart.row * CELL + CELL/2;
        DrawCircle(x, y, CELL/2 - 3, YELLOW);
    }

    /* cursor */
    int cx = ORIGEN_X + g->editorCursor.col * CELL;
    int cy = ORIGEN_Y + g->editorCursor.row * CELL;
    Color colorCursor = g->editorWaitingWallDir ? ORANGE : GREEN;
    DrawRectangleLines(cx, cy, CELL, CELL, colorCursor);
    DrawRectangleLines(cx - 1, cy - 1, CELL + 2, CELL + 2, colorCursor);

    DrawText(TextFormat("Tamano: %dx%d", m->rows, m->cols), 40, ORIGEN_Y + m->rows * CELL + 20, 18, RAYWHITE);
    if (g->editorWaitingWallDir)
        DrawText("Elegi una direccion para el muro...", 40, ORIGEN_Y + m->rows * CELL + 45, 18, ORANGE);
}

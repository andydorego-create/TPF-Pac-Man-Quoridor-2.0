#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "game.h"
#include "map.h"
#include "ai.h"
#include "editor.h"

Screen pantallaActual = SCREEN_CONFIG;
bool salirPrograma = false;

static const char *NOMBRE_FANTASMA[NUM_GHOSTS] = { "Blinky", "Inky", "Pinky", "Clyde" };
static const Color COLOR_FANTASMA[NUM_GHOSTS]  = { RED, SKYBLUE, MAGENTA, ORANGE };

/* ------------------------------------------------------------------ */
/* Helpers generales                                                  */
/* ------------------------------------------------------------------ */

static bool EnTableroG(Game *g, Cell c) {
    return c.row >= 0 && c.row < g->map.rows && c.col >= 0 && c.col < g->map.cols;
}

int Game_TamanioCelda(const Game *g) {
    int anchoDisponible = 1000 - 260; /* deja lugar para el panel de info a la izquierda */
    int altoDisponible  = 700 - 110;
    int porAncho = anchoDisponible / g->map.cols;
    int porAlto  = altoDisponible / g->map.rows;
    int t = (porAncho < porAlto) ? porAncho : porAlto;
    if (t > 48) t = 48;
    if (t < 14) t = 14;
    return t;
}

static bool ArchivoExisteG(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) { fclose(f); return true; }
    return false;
}

/* ------------------------------------------------------------------ */
/* Inicialización general / nueva partida                             */
/* ------------------------------------------------------------------ */

void Game_Init(Game *g) {
    Map_CrearPredefinidosSiNoExisten();

    g->cfg.mode = MODE_AI_VS_PLAYER;
    for (int i = 0; i < NUM_GHOSTS; i++) {
        g->cfg.ghostEnabled[i] = true;
        g->cfg.ghostDifficulty[i] = 2;
    }
    g->cfg.pacWallsHand = 3;
    g->cfg.ghostWallsHand = 1;
    g->cfg.wallLifetime = 4;

    /* mapa por defecto mientras el jugador no eligió ninguno todavía */
    Map_Cargar("maps/mapa1_clasico.txt", &g->map);

    g->menuIndex = 0;
    pantallaActual = SCREEN_CONFIG;
}

void Game_NuevaPartida(Game *g) {
    Board_Inicializar(&g->board, &g->map);

    g->pacman = g->map.pacmanStart;
    g->pacLives = 3;

    for (int i = 0; i < NUM_GHOSTS; i++) {
        g->ghosts[i] = g->map.ghostStart[i];
        g->ghostAlive[i] = true;
    }

    for (int i = 0; i < NUM_BALLS; i++) {
        g->balls[i] = g->map.ballStart[i];
        g->ballEaten[i] = false;
    }
    g->ballsEatenCount = 0;

    g->pacWallsLeft = g->cfg.pacWallsHand;
    g->ghostWallsLeft = g->cfg.ghostWallsHand;

    g->turnOwner = 0;
    g->actionsLeft = 2;
    g->ghostTurnIndex = -1;
    g->globalTurn = 0;
    g->winner = 0;
    g->pacAteBallThisTurn = false;
    g->placingWall = false;
    g->waitingWallSide = false;
    g->aiTimer = 0.5f;
}

/* ------------------------------------------------------------------ */
/* Reglas de movimiento / captura                                     */
/* ------------------------------------------------------------------ */

static void AtraparPacman(Game *g) {
    g->pacLives--;
    g->pacman = g->map.pacmanStart;
    for (int i = 0; i < NUM_GHOSTS; i++) {
        if (g->cfg.ghostEnabled[i]) {
            g->ghosts[i] = g->map.ghostStart[i];
            g->ghostAlive[i] = true;
        }
    }
    if (g->pacLives <= 0) {
        g->winner = 2;
        pantallaActual = SCREEN_GAMEOVER;
    }
}

static bool MoverPacman(Game *g, int dr, int dc) {
    Cell siguiente = { g->pacman.row + dr, g->pacman.col + dc };
    if (!EnTableroG(g, siguiente)) return false;
    if (Board_HayMuroEntre(&g->board, g->pacman, siguiente)) return false;

    g->pacman = siguiente;
    g->actionsLeft--;

    /* ¿comió una pac-bola? */
    for (int i = 0; i < NUM_BALLS; i++) {
        if (!g->ballEaten[i] && g->balls[i].row == siguiente.row && g->balls[i].col == siguiente.col) {
            g->ballEaten[i] = true;
            g->ballsEatenCount++;
            g->actionsLeft++;            /* acción extra */
            g->pacAteBallThisTurn = true;
        }
    }

    /* ¿hay un fantasma en esa celda? */
    for (int i = 0; i < NUM_GHOSTS; i++) {
        if (g->cfg.ghostEnabled[i] && g->ghostAlive[i] &&
            g->ghosts[i].row == siguiente.row && g->ghosts[i].col == siguiente.col) {
            if (g->pacAteBallThisTurn) {
                g->ghostAlive[i] = false;   /* Pac-Man se lo come */
            } else {
                AtraparPacman(g);           /* el fantasma lo atrapa */
            }
        }
    }

    if (g->ballsEatenCount >= NUM_BALLS) {
        g->winner = 1;
        pantallaActual = SCREEN_GAMEOVER;
    }
    return true;
}

static bool ColocarMuroPacman(Game *g, Cell desde, int dr, int dc) {
    if (g->pacWallsLeft <= 0) return false;
    Cell destino = { desde.row + dr, desde.col + dc };
    if (!EnTableroG(g, desde)) return false;
    if (!EnTableroG(g, destino)) return false;
    if (!Board_ColocarMuroTemporal(&g->board, desde, destino, OWNER_PACMAN)) return false;

    if (desde.row == destino.row) {
        int col = (desde.col < destino.col) ? desde.col : destino.col;
        g->board.lifeV[desde.row][col] = g->cfg.wallLifetime;
    } else {
        int row = (desde.row < destino.row) ? desde.row : destino.row;
        g->board.lifeH[row][desde.col] = g->cfg.wallLifetime;
    }
    g->pacWallsLeft--;
    g->actionsLeft--;
    return true;
}

static bool EsFrenetico(Game *g, int idx) {
    Cell f = g->ghosts[idx];
    Cell p = g->pacman;
    if (f.row == p.row && f.col != p.col) {
        int c1 = (f.col < p.col) ? f.col : p.col;
        int c2 = (f.col < p.col) ? p.col : f.col;
        for (int c = c1; c < c2; c++) {
            Cell a = { f.row, c }, b = { f.row, c + 1 };
            if (Board_HayMuroEntre(&g->board, a, b)) return false;
        }
        return true;
    }
    if (f.col == p.col && f.row != p.row) {
        int r1 = (f.row < p.row) ? f.row : p.row;
        int r2 = (f.row < p.row) ? p.row : f.row;
        for (int r = r1; r < r2; r++) {
            Cell a = { r, f.col }, b = { r + 1, f.col };
            if (Board_HayMuroEntre(&g->board, a, b)) return false;
        }
        return true;
    }
    return false;
}

/* Ejecuta la acción de un fantasma (mover o colocar muro). Usada tanto
   por la IA como por el jugador humano en modo PvP, para que las reglas
   se apliquen siempre de la misma forma. Devuelve true si efectivamente
   se realizó una acción (para saber si hay que pasar al siguiente fantasma). */
static bool EjecutarAccionFantasma(Game *g, int idx, Cell origenMuro, int dr, int dc, bool colocarMuro) {
    if (colocarMuro) {
        if (g->ghostWallsLeft <= 0) return false;
        Cell destino = { origenMuro.row + dr, origenMuro.col + dc };
        if (!EnTableroG(g, origenMuro)) return false;
        if (!EnTableroG(g, destino)) return false;
        if (!Board_ColocarMuroTemporal(&g->board, origenMuro, destino, OWNER_GHOST)) return false;

        if (origenMuro.row == destino.row) {
            int col = (origenMuro.col < destino.col) ? origenMuro.col : destino.col;
            g->board.lifeV[origenMuro.row][col] = g->cfg.wallLifetime;
        } else {
            int row = (origenMuro.row < destino.row) ? origenMuro.row : destino.row;
            g->board.lifeH[row][origenMuro.col] = g->cfg.wallLifetime;
        }
        g->ghostWallsLeft--;
        return true;
    }

    if (dr == 0 && dc == 0) return false;
    bool frenetico = EsFrenetico(g, idx);
    int pasos = frenetico ? 2 : 1;
    bool semovio = false;

    for (int s = 0; s < pasos; s++) {
        Cell siguiente = { g->ghosts[idx].row + dr, g->ghosts[idx].col + dc };
        if (!EnTableroG(g, siguiente)) break;
        if (Board_HayMuroEntre(&g->board, g->ghosts[idx], siguiente)) break;
        g->ghosts[idx] = siguiente;
        semovio = true;
        if (siguiente.row == g->pacman.row && siguiente.col == g->pacman.col) {
            AtraparPacman(g);
            break;
        }
    }
    return semovio;
}

static int SiguienteFantasmaValido(Game *g, int desde) {
    for (int i = desde; i < NUM_GHOSTS; i++)
        if (g->cfg.ghostEnabled[i] && g->ghostAlive[i]) return i;
    return -1;
}

static void TerminarTurnoFantasmas(Game *g) {
    int expPac = 0, expGhost = 0;
    Board_AvanzarTurnoMuros(&g->board, g->map.rows, g->map.cols, &expPac, &expGhost);
    g->pacWallsLeft += expPac;
    g->ghostWallsLeft += expGhost;
    g->globalTurn++;
    g->turnOwner = 0;
    g->actionsLeft = 2;
    g->pacAteBallThisTurn = false;
    g->placingWall = false;
    g->waitingWallSide = false;
}

static void IniciarTurnoFantasmas(Game *g) {
    g->turnOwner = 1;
    g->placingWall = false;
    g->waitingWallSide = false;
    g->aiTimer = 0.4f;
    g->ghostTurnIndex = SiguienteFantasmaValido(g, 0);
    if (g->ghostTurnIndex < 0) TerminarTurnoFantasmas(g);
}

/* ------------------------------------------------------------------ */
/* Pantalla: configuración                                            */
/* ------------------------------------------------------------------ */

#define NUM_CAMPOS_CONFIG 12

static void Config_Update(Game *g) {
    if (IsKeyPressed(KEY_DOWN)) g->menuIndex = (g->menuIndex + 1) % NUM_CAMPOS_CONFIG;
    if (IsKeyPressed(KEY_UP))   g->menuIndex = (g->menuIndex - 1 + NUM_CAMPOS_CONFIG) % NUM_CAMPOS_CONFIG;

    int delta = 0;
    if (IsKeyPressed(KEY_RIGHT)) delta = 1;
    if (IsKeyPressed(KEY_LEFT))  delta = -1;

    if (delta != 0) {
        switch (g->menuIndex) {
            case 0: g->cfg.mode = (g->cfg.mode == MODE_AI_VS_PLAYER) ? MODE_PVP : MODE_AI_VS_PLAYER; break;
            case 1: g->cfg.ghostEnabled[0] = !g->cfg.ghostEnabled[0]; break;
            case 2: g->cfg.ghostDifficulty[0] += delta; break;
            case 3: g->cfg.ghostEnabled[1] = !g->cfg.ghostEnabled[1]; break;
            case 4: g->cfg.ghostDifficulty[1] += delta; break;
            case 5: g->cfg.ghostEnabled[2] = !g->cfg.ghostEnabled[2]; break;
            case 6: g->cfg.ghostDifficulty[2] += delta; break;
            case 7: g->cfg.ghostEnabled[3] = !g->cfg.ghostEnabled[3]; break;
            case 8: g->cfg.ghostDifficulty[3] += delta; break;
            case 9:  g->cfg.pacWallsHand += delta; break;
            case 10: g->cfg.ghostWallsHand += delta; break;
            case 11: g->cfg.wallLifetime += delta; break;
        }
        for (int i = 0; i < NUM_GHOSTS; i++) {
            if (g->cfg.ghostDifficulty[i] < 1) g->cfg.ghostDifficulty[i] = 1;
            if (g->cfg.ghostDifficulty[i] > 3) g->cfg.ghostDifficulty[i] = 3;
        }
        if (g->cfg.pacWallsHand < 0) g->cfg.pacWallsHand = 0;
        if (g->cfg.pacWallsHand > 10) g->cfg.pacWallsHand = 10;
        if (g->cfg.ghostWallsHand < 0) g->cfg.ghostWallsHand = 0;
        if (g->cfg.ghostWallsHand > 10) g->cfg.ghostWallsHand = 10;
        if (g->cfg.wallLifetime < 1) g->cfg.wallLifetime = 1;
        if (g->cfg.wallLifetime > 15) g->cfg.wallLifetime = 15;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        g->menuIndex = 0;
        pantallaActual = SCREEN_MAP_SELECT;
    }
    if (IsKeyPressed(KEY_ESCAPE)) salirPrograma = true;
}

static void DrawCampo(int y, int indiceCampo, int seleccion, const char *texto) {
    Color color = (indiceCampo == seleccion) ? YELLOW : RAYWHITE;
    if (indiceCampo == seleccion) DrawText(">", 30, y, 20, color);
    DrawText(texto, 55, y, 20, color);
}

static void Config_Draw(Game *g) {
    DrawText("CONFIGURACION DE LA PARTIDA", 30, 20, 28, RAYWHITE);
    DrawText("Flechas arriba/abajo: elegir campo | Izq/Der: cambiar valor | ENTER: continuar | ESC: salir",
             30, 55, 14, LIGHTGRAY);

    int y = 100;
    char buf[128];

    snprintf(buf, sizeof(buf), "Modo de juego: %s", g->cfg.mode == MODE_AI_VS_PLAYER ? "IA vs Player" : "Player vs Player");
    DrawCampo(y, 0, g->menuIndex, buf); y += 32;

    for (int i = 0; i < NUM_GHOSTS; i++) {
        snprintf(buf, sizeof(buf), "Fantasma %s: %s", NOMBRE_FANTASMA[i], g->cfg.ghostEnabled[i] ? "Habilitado" : "Deshabilitado");
        DrawCampo(y, 1 + i*2, g->menuIndex, buf); y += 32;
        const char *dif = g->cfg.ghostDifficulty[i] == 1 ? "Facil" : g->cfg.ghostDifficulty[i] == 2 ? "Medio" : "Dificil";
        snprintf(buf, sizeof(buf), "  Dificultad de %s: %s", NOMBRE_FANTASMA[i], dif);
        DrawCampo(y, 2 + i*2, g->menuIndex, buf); y += 32;
    }

    snprintf(buf, sizeof(buf), "Muros en mano de Pac-Man: %d", g->cfg.pacWallsHand);
    DrawCampo(y, 9, g->menuIndex, buf); y += 32;
    snprintf(buf, sizeof(buf), "Muros en mano de los fantasmas: %d", g->cfg.ghostWallsHand);
    DrawCampo(y, 10, g->menuIndex, buf); y += 32;
    snprintf(buf, sizeof(buf), "Tiempo de vida de los muros (turnos): %d", g->cfg.wallLifetime);
    DrawCampo(y, 11, g->menuIndex, buf); y += 32;
}

/* ------------------------------------------------------------------ */
/* Pantalla: selección de mapa                                        */
/* ------------------------------------------------------------------ */

static const char *RUTAS_MAPA[4] = {
    "maps/mapa1_clasico.txt",
    "maps/mapa2_chico.txt",
    "maps/mapa3_grande.txt",
    "maps/personalizado.txt"
};
static const char *NOMBRES_MAPA[4] = {
    "Mapa 1 - Clasico (9x9)",
    "Mapa 2 - Chico (7x7)",
    "Mapa 3 - Grande (11x11)",
    "Mapa personalizado (creado con el editor)"
};

static int CantidadOpcionesMapa(void) {
    int n = 3; /* los 3 predefinidos siempre están */
    if (ArchivoExisteG("maps/personalizado.txt")) n++;
    return n + 1; /* + opción "crear/editar mapa" */
}

static void MapSelect_Update(Game *g) {
    int total = CantidadOpcionesMapa();
    if (IsKeyPressed(KEY_DOWN)) g->menuIndex = (g->menuIndex + 1) % total;
    if (IsKeyPressed(KEY_UP))   g->menuIndex = (g->menuIndex - 1 + total) % total;

    if (IsKeyPressed(KEY_ENTER)) {
        int opcionesMapa = total - 1;
        if (g->menuIndex < opcionesMapa) {
            Map_Cargar(RUTAS_MAPA[g->menuIndex], &g->map);
            Game_NuevaPartida(g);
            pantallaActual = SCREEN_PLAYING;
        } else {
            Editor_Iniciar(g);
            pantallaActual = SCREEN_EDITOR;
        }
        g->menuIndex = 0;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        g->menuIndex = 0;
        pantallaActual = SCREEN_CONFIG;
    }
}

static void MapSelect_Draw(Game *g) {
    DrawText("ELEGIR MAPA", 30, 20, 28, RAYWHITE);
    DrawText("Flechas: navegar | ENTER: elegir | ESC: volver a configuracion", 30, 55, 14, LIGHTGRAY);

    int total = CantidadOpcionesMapa();
    int opcionesMapa = total - 1;
    int y = 110;
    for (int i = 0; i < opcionesMapa; i++) {
        DrawCampo(y, i, g->menuIndex, NOMBRES_MAPA[i]);
        y += 32;
    }
    DrawCampo(y, opcionesMapa, g->menuIndex, "Crear / editar mapa nuevo (editor)");
}

/* ------------------------------------------------------------------ */
/* Pantalla: jugando                                                  */
/* ------------------------------------------------------------------ */

static void LeerFlecha(int *dr, int *dc, bool *hubo) {
    *dr = 0; *dc = 0; *hubo = false;
    if (IsKeyPressed(KEY_UP))    { *dr = -1; *dc = 0; *hubo = true; }
    if (IsKeyPressed(KEY_DOWN))  { *dr = 1;  *dc = 0; *hubo = true; }
    if (IsKeyPressed(KEY_LEFT))  { *dr = 0;  *dc = -1; *hubo = true; }
    if (IsKeyPressed(KEY_RIGHT)) { *dr = 0;  *dc = 1;  *hubo = true; }
}

static void Playing_Update(Game *g, float dt) {
    if (IsKeyPressed(KEY_ESCAPE)) { salirPrograma = true; return; }
    if (IsKeyPressed(KEY_R)) { Game_NuevaPartida(g); return; }
    if (g->winner != 0) return;

    if (g->turnOwner == 0) {
        /* turno de Pac-Man: siempre lo maneja un humano */
        if (IsKeyPressed(KEY_M)) {
            g->placingWall = !g->placingWall;
            if (g->placingWall) g->wallCursor = g->pacman; /* el cursor arranca sobre Pac-Man */
            g->waitingWallSide = false;
        }

        if (g->placingWall) {
            /* modo colocar muro: las flechas mueven el CURSOR (no a Pac-Man) por
               cualquier parte del tablero; W + flecha coloca el muro en ese lado. */
            if (IsKeyPressed(KEY_W)) g->waitingWallSide = true;

            int dr, dc; bool hubo;
            LeerFlecha(&dr, &dc, &hubo);
            if (hubo) {
                if (g->waitingWallSide) {
                    if (ColocarMuroPacman(g, g->wallCursor, dr, dc)) g->placingWall = false;
                    g->waitingWallSide = false;
                } else {
                    int nr = g->wallCursor.row + dr;
                    int nc = g->wallCursor.col + dc;
                    if (nr >= 0 && nr < g->map.rows) g->wallCursor.row = nr;
                    if (nc >= 0 && nc < g->map.cols) g->wallCursor.col = nc;
                }
            }
        } else {
            int dr, dc; bool hubo;
            LeerFlecha(&dr, &dc, &hubo);
            if (hubo) MoverPacman(g, dr, dc);
        }

        if (g->winner == 0 && (IsKeyPressed(KEY_ENTER) || g->actionsLeft <= 0)) {
            IniciarTurnoFantasmas(g);
        }
    } else {
        if (g->ghostTurnIndex < 0) { TerminarTurnoFantasmas(g); return; }

        if (g->cfg.mode == MODE_AI_VS_PLAYER) {
            g->aiTimer -= dt;
            if (g->aiTimer <= 0) {
                g->aiTimer = 0.45f;
                int idx = g->ghostTurnIndex;
                GhostDecision d = AI_Decidir(g, idx);
                EjecutarAccionFantasma(g, idx, g->ghosts[idx], d.dirRow, d.dirCol, d.placeWall);
                if (g->winner == 0) {
                    g->ghostTurnIndex = SiguienteFantasmaValido(g, idx + 1);
                    if (g->ghostTurnIndex < 0) TerminarTurnoFantasmas(g);
                }
            }
        } else {
            /* PvP: el segundo jugador maneja al fantasma actual */
            int idx = g->ghostTurnIndex;
            if (IsKeyPressed(KEY_M)) {
                g->placingWall = !g->placingWall;
                if (g->placingWall) g->wallCursor = g->ghosts[idx];
                g->waitingWallSide = false;
            }

            if (g->placingWall) {
                if (IsKeyPressed(KEY_W)) g->waitingWallSide = true;

                int dr, dc; bool hubo;
                LeerFlecha(&dr, &dc, &hubo);
                if (hubo) {
                    if (g->waitingWallSide) {
                        bool ok = EjecutarAccionFantasma(g, idx, g->wallCursor, dr, dc, true);
                        g->waitingWallSide = false;
                        if (ok) {
                            g->placingWall = false;
                            if (g->winner == 0) {
                                g->ghostTurnIndex = SiguienteFantasmaValido(g, idx + 1);
                                if (g->ghostTurnIndex < 0) TerminarTurnoFantasmas(g);
                            }
                        }
                    } else {
                        int nr = g->wallCursor.row + dr;
                        int nc = g->wallCursor.col + dc;
                        if (nr >= 0 && nr < g->map.rows) g->wallCursor.row = nr;
                        if (nc >= 0 && nc < g->map.cols) g->wallCursor.col = nc;
                    }
                }
            } else {
                int dr, dc; bool hubo;
                LeerFlecha(&dr, &dc, &hubo);
                if (hubo) {
                    bool ok = EjecutarAccionFantasma(g, idx, g->ghosts[idx], dr, dc, false);
                    if (ok && g->winner == 0) {
                        g->ghostTurnIndex = SiguienteFantasmaValido(g, idx + 1);
                        if (g->ghostTurnIndex < 0) TerminarTurnoFantasmas(g);
                    }
                }
            }
        }
    }
}

static void DibujarTablero(Game *g) {
    int cell = Game_TamanioCelda(g);
    int ox = 240, oy = 90;

    for (int r = 0; r < g->map.rows; r++)
        for (int c = 0; c < g->map.cols; c++)
            DrawRectangleLines(ox + c*cell, oy + r*cell, cell, cell, GRAY);

    /* muros: permanentes en naranja, temporales en celeste */
    for (int r = 0; r < g->map.rows; r++)
        for (int c = 0; c < g->map.cols; c++) {
            if (g->board.typeH[r][c] != WALL_NONE) {
                Color col = (g->board.typeH[r][c] == WALL_PERM) ? ORANGE : SKYBLUE;
                DrawRectangle(ox + c*cell, oy + (r+1)*cell - 3, cell, 6, col);
            }
            if (g->board.typeV[r][c] != WALL_NONE) {
                Color col = (g->board.typeV[r][c] == WALL_PERM) ? ORANGE : SKYBLUE;
                DrawRectangle(ox + (c+1)*cell - 3, oy + r*cell, 6, cell, col);
            }
        }

    /* pac-bolas */
    for (int i = 0; i < NUM_BALLS; i++) {
        if (g->ballEaten[i]) continue;
        int x = ox + g->balls[i].col*cell + cell/2;
        int y = oy + g->balls[i].row*cell + cell/2;
        DrawCircle(x, y, cell/6 + 2, PINK);
    }

    /* fantasmas */
    for (int i = 0; i < NUM_GHOSTS; i++) {
        if (!g->cfg.ghostEnabled[i] || !g->ghostAlive[i]) continue;
        int x = ox + g->ghosts[i].col*cell;
        int y = oy + g->ghosts[i].row*cell;
        DrawRectangle(x + 2, y + 2, cell - 4, cell - 4, COLOR_FANTASMA[i]);
    }

    /* pac-man */
    {
        int x = ox + g->pacman.col*cell + cell/2;
        int y = oy + g->pacman.row*cell + cell/2;
        DrawCircle(x, y, cell/2 - 3, YELLOW);
    }

    /* cursor de colocación de muro: se puede mover a cualquier celda del tablero */
    if (g->placingWall) {
        int cx = ox + g->wallCursor.col*cell;
        int cy = oy + g->wallCursor.row*cell;
        Color colorCursor = g->waitingWallSide ? ORANGE : SKYBLUE;
        DrawRectangleLines(cx, cy, cell, cell, colorCursor);
        DrawRectangleLines(cx - 1, cy - 1, cell + 2, cell + 2, colorCursor);
    }
}

static void DibujarPanelInfo(Game *g) {
    int y = 20;
    DrawText("INFORMACION", 20, y, 22, RAYWHITE); y += 35;

    char buf[64];
    snprintf(buf, sizeof(buf), "Vidas: %d", g->pacLives);
    DrawText(buf, 20, y, 18, RAYWHITE); y += 24;
    snprintf(buf, sizeof(buf), "Pac-bolas: %d/4", g->ballsEatenCount);
    DrawText(buf, 20, y, 18, RAYWHITE); y += 24;
    snprintf(buf, sizeof(buf), "Muros Pac-Man: %d", g->pacWallsLeft);
    DrawText(buf, 20, y, 18, RAYWHITE); y += 24;
    snprintf(buf, sizeof(buf), "Muros Fantasmas: %d", g->ghostWallsLeft);
    DrawText(buf, 20, y, 18, RAYWHITE); y += 24;
    snprintf(buf, sizeof(buf), "Turno global: %d", g->globalTurn);
    DrawText(buf, 20, y, 18, RAYWHITE); y += 30;

    if (g->turnOwner == 0) {
        DrawText("Turno: Pac-Man", 20, y, 18, YELLOW); y += 24;
        snprintf(buf, sizeof(buf), "Acciones: %d", g->actionsLeft);
        DrawText(buf, 20, y, 18, YELLOW); y += 24;
        if (g->placingWall) {
            const char *txt = g->waitingWallSide ? "Elegi el lado del muro..." : "Moviendo cursor de muro...";
            DrawText(txt, 20, y, 16, SKYBLUE); y += 22;
        }
    } else {
        int idx = g->ghostTurnIndex;
        if (idx >= 0) {
            snprintf(buf, sizeof(buf), "Turno: %s", NOMBRE_FANTASMA[idx]);
            DrawText(buf, 20, y, 18, COLOR_FANTASMA[idx]); y += 24;
            if (g->cfg.mode == MODE_AI_VS_PLAYER) DrawText("(la IA esta jugando)", 20, y, 14, LIGHTGRAY);
            else if (g->placingWall) {
                const char *txt = g->waitingWallSide ? "Elegi el lado del muro..." : "Moviendo cursor de muro...";
                DrawText(txt, 20, y, 16, SKYBLUE);
            }
        }
        y += 22;
    }

    y += 20;
    DrawText("Controles:", 20, y, 16, LIGHTGRAY); y += 20;
    DrawText("Flechas: mover", 20, y, 14, LIGHTGRAY); y += 18;
    DrawText("M: modo muro (cualquier celda)", 20, y, 14, LIGHTGRAY); y += 18;
    DrawText("  flechas mueven el cursor", 20, y, 14, LIGHTGRAY); y += 18;
    DrawText("  W + flecha: coloca el muro", 20, y, 14, LIGHTGRAY); y += 18;
    DrawText("ENTER: fin de turno", 20, y, 14, LIGHTGRAY); y += 18;
    DrawText("R: reiniciar", 20, y, 14, LIGHTGRAY); y += 18;
    DrawText("ESC: salir", 20, y, 14, LIGHTGRAY); y += 18;
}

static void Playing_Draw(Game *g) {
    DibujarPanelInfo(g);
    DibujarTablero(g);
}

/* ------------------------------------------------------------------ */
/* Pantalla: fin de partida                                           */
/* ------------------------------------------------------------------ */

static const char *NivelAlcanzado(int comidas) {
    switch (comidas) {
        case 1: return "Pac-Man novato";
        case 2: return "Pac-Man prometedor";
        case 3: return "Pac-Man de categoria";
        case 4: return "Pac-Man de elite";
        default: return "Sin nivel (no comio ninguna pac-bola)";
    }
}

static void GameOver_Update(Game *g) {
    if (IsKeyPressed(KEY_ENTER)) {
        Game_NuevaPartida(g);
        pantallaActual = SCREEN_PLAYING;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        g->menuIndex = 0;
        pantallaActual = SCREEN_CONFIG;
    }
}

static void GameOver_Draw(Game *g) {
    const char *titulo = (g->winner == 1) ? "GANO PAC-MAN" : "GANARON LOS FANTASMAS";
    Color color = (g->winner == 1) ? YELLOW : RED;
    DrawText(titulo, 300, 220, 40, color);

    char buf[64];
    snprintf(buf, sizeof(buf), "Pac-bolas comidas: %d/4", g->ballsEatenCount);
    DrawText(buf, 300, 280, 22, RAYWHITE);

    snprintf(buf, sizeof(buf), "Nivel alcanzado: %s", NivelAlcanzado(g->ballsEatenCount));
    DrawText(buf, 300, 310, 22, RAYWHITE);

    DrawText("ENTER: jugar de nuevo (misma config)", 300, 380, 18, LIGHTGRAY);
    DrawText("ESC: volver a configuracion", 300, 405, 18, LIGHTGRAY);
}

/* ------------------------------------------------------------------ */
/* Dispatch general                                                    */
/* ------------------------------------------------------------------ */

void Game_Update(Game *g, float dt) {
    switch (pantallaActual) {
        case SCREEN_CONFIG:     Config_Update(g); break;
        case SCREEN_MAP_SELECT: MapSelect_Update(g); break;
        case SCREEN_EDITOR:     Editor_Update(g); break;
        case SCREEN_PLAYING:    Playing_Update(g, dt); break;
        case SCREEN_GAMEOVER:   GameOver_Update(g); break;
    }
}

void Game_Draw(Game *g) {
    switch (pantallaActual) {
        case SCREEN_CONFIG:     Config_Draw(g); break;
        case SCREEN_MAP_SELECT: MapSelect_Draw(g); break;
        case SCREEN_EDITOR:     Editor_Draw(g); break;
        case SCREEN_PLAYING:    Playing_Draw(g); break;
        case SCREEN_GAMEOVER:   GameOver_Draw(g); break;
    }
}

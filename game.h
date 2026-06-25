#ifndef GAME_H
#define GAME_H

#include "defs.h"

/* Pantalla actual a nivel global (todas las funciones leen/escriben sobre
   un único Game que vive en main.c y se pasa por puntero). */
extern Screen pantallaActual;
extern bool salirPrograma;

void Game_Init(Game *g);

/* Arranca una partida nueva con el mapa y la config ya elegidos */
void Game_NuevaPartida(Game *g);

/* Se llama una vez por frame; dt = delta time en segundos */
void Game_Update(Game *g, float dt);
void Game_Draw(Game *g);

/* Tamaño de celda en píxeles usado para dibujar (lo calculamos según el
   tamaño del mapa para que cualquier grilla entre en la ventana) */
int Game_TamanioCelda(const Game *g);

#endif

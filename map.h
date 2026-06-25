#ifndef MAP_H
#define MAP_H

#include "defs.h"

/* Crea en disco los 3 mapas predefinidos (si no existen ya). */
void Map_CrearPredefinidosSiNoExisten(void);

/* Carga un mapa desde archivo de texto. Devuelve true si pudo. */
bool Map_Cargar(const char *path, MapData *map);

/* Guarda un mapa a archivo de texto. Devuelve true si pudo. */
bool Map_Guardar(const char *path, const MapData *map);

/* Arma un mapa 9x9 vacío con valores por defecto (usado por el editor). */
void Map_Vacio(MapData *map, int rows, int cols);

/* Devuelve true si hay un muro (permanente o temporal) entre dos celdas adyacentes */
bool Board_HayMuroEntre(const Board *b, Cell a, Cell c);

/* Inicializa el Board (matrices de muros de partida) a partir del mapa elegido */
void Board_Inicializar(Board *b, const MapData *map);

/* Coloca un muro temporal entre dos celdas adyacentes, restando de la mano */
bool Board_ColocarMuroTemporal(Board *b, Cell a, Cell c, int owner);

/* Avanza el reloj de los muros temporales un turno global; devuelve cuántos */
/* muros de Pac-Man y de fantasmas expiraron (para devolverlos a la mano)    */
void Board_AvanzarTurnoMuros(Board *b, int rows, int cols, int *expiradosPac, int *expiradosGhost);

#endif

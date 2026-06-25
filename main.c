#include "raylib.h"
#include "defs.h"
#include "game.h"

int main(void) {
    InitWindow(1000, 700, "Quoridor Pac-Man - LP1");
    SetExitKey(KEY_NULL); /* desactiva el cierre automático de raylib con ESC */
    SetTargetFPS(60);

    Game g;
    Game_Init(&g);

    while (!WindowShouldClose() && !salirPrograma) {
        float dt = GetFrameTime();
        Game_Update(&g, dt);

        BeginDrawing();
        ClearBackground((Color){ 20, 20, 30, 255 });
        Game_Draw(&g);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

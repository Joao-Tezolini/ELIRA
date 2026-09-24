#include "raylib.h"
#include "game.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ELIRA - Decifra.IA (Prototipo)");
    SetTargetFPS(60);
    HideCursor(); // o mouse do sistema some; desenhamos nosso proprio cursor

    Game game;
    Game_Init(&game);

    while (!WindowShouldClose()) {
        Game_Update(&game);

        BeginDrawing();
        Game_Draw(&game);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

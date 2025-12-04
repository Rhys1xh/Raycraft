#include "raylib.h"
#include "raymath.h"
#include "Character.hpp"
#include "World.hpp"
#include <iostream>

int main() {
    // Window setup
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Raycraft (I know it's lame im just trying to learn math here hehe :D)");
    SetTargetFPS(144);
    SetExitKey(KEY_NULL);

    // Initialize world and player
    OptimizedWorld world(1337);
    Character player(&world, { 32, 40, 32 });

    // Generate crosshair
    Image crosshairImg = GenImageColor(32, 32, BLANK);
    ImageDrawPixel(&crosshairImg, 16, 16, WHITE);
    ImageDrawLine(&crosshairImg, 16, 12, 16, 8, WHITE);
    ImageDrawLine(&crosshairImg, 16, 20, 16, 24, WHITE);
    ImageDrawLine(&crosshairImg, 12, 16, 8, 16, WHITE);
    ImageDrawLine(&crosshairImg, 20, 16, 24, 16, WHITE);
    Texture2D crosshair = LoadTextureFromImage(crosshairImg);
    UnloadImage(crosshairImg);

    // Game state
    bool showDebug = false;
    float timeOfDay = 12.0f;

    DisableCursor();

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Toggle debug
        if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

        // Update systems
        player.Update();
        world.Update(player.GetPosition());

        // Time cycle
        timeOfDay += deltaTime * 0.05f;
        if (timeOfDay > 24.0f) timeOfDay -= 24.0f;

        // Sky color
        Color skyColor;
        if (timeOfDay > 6.0f && timeOfDay < 18.0f) {
            float brightness = 0.7f + 0.3f * sinf((timeOfDay - 12.0f) * PI / 12.0f);
            skyColor = Color{
                (unsigned char)(135 * brightness),
                (unsigned char)(206 * brightness),
                (unsigned char)(235 * brightness),
                255
            };
        }
        else {
            skyColor = Color{ 10, 10, 40, 255 };
        }

        // ========== RENDERING ==========
        BeginDrawing();
        ClearBackground(skyColor);

        // 3D Scene
        BeginMode3D(player.GetCamera());
        world.Draw();
        EndMode3D();

        // 2D UI
        // Crosshair
        DrawTexture(crosshair,
            screenWidth / 2 - crosshair.width / 2,
            screenHeight / 2 - crosshair.height / 2,
            WHITE);

        // Hotbar
        DrawRectangle(screenWidth / 2 - 200, screenHeight - 60, 400, 50, Color{ 0, 0, 0, 180 });
        for (int i = 0; i < 5; i++) {
            int x = screenWidth / 2 - 200 + i * 80;
            bool selected = (i + 1) == player.GetSelectedBlock();

            Color blockColor;
            switch (i + 1) {
            case 1: blockColor = GREEN; break;
            case 2: blockColor = BROWN; break;
            case 3: blockColor = GRAY; break;
            case 4: blockColor = Color{ 139, 69, 19, 255 }; break;
            case 5: blockColor = Color{ 34, 139, 34, 255 }; break;
            default: blockColor = DARKGRAY;
            }

            DrawRectangle(x + 10, screenHeight - 50, 60, 30, blockColor);
            DrawRectangleLines(x + 10, screenHeight - 50, 60, 30, BLACK);

            if (selected) {
                DrawRectangleLines(x, screenHeight - 60, 80, 50, YELLOW);
            }

            DrawText(TextFormat("%d", i + 1), x + 35, screenHeight - 55, 20, WHITE);
        }

        // Debug info
        if (showDebug) {
            Vector3 pos = player.GetPosition();

            DrawRectangle(10, 10, 250, 120, Color{ 0, 0, 0, 180 });
            DrawText(TextFormat("FPS: %d", GetFPS()), 20, 20, 18, GREEN);
            DrawText(TextFormat("Pos: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z), 20, 45, 18, WHITE);
            DrawText(TextFormat("Block: %d", player.GetSelectedBlock()), 20, 70, 18, SKYBLUE);
            DrawText(TextFormat("Flying: %s", player.IsFlying() ? "YES" : "NO"), 20, 95, 18,
                player.IsFlying() ? PURPLE : WHITE);
        }

        EndDrawing();
    }

    // Cleanup
    UnloadTexture(crosshair);
    CloseWindow();

    return 0;
}
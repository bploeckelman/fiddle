#include <stdio.h>

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// NOTE - for convenience when primary monitor is otherwise in use
#define USE_SECONDARY_MONITOR 1

int windowWidth = 1280;
int windowHeight = 720;
char *windowTitle = "Fiddle";


enum UI_SizeKind
{
    UI_SizeKind_Null,
    UI_SizeKind_Pixels,
    UI_SizeKind_TextContent,
    UI_SizeKind_PercentOfParent,
    UI_SizeKind_ChildrenSum,
};

struct UI_Size
{
    enum UI_SizeKind kind;
    float value;
    float strictness;
};



void UpdateDrawFrame(void);

int main() {
    InitWindow(windowWidth, windowHeight, windowTitle);

    if (USE_SECONDARY_MONITOR) {
        // NOTE - this forces fullscreen which is kind of shitty...
        // SetWindowMonitor(1);

        // manually position it in the center of the secondary monitor
        int secondaryMonitorWidth = GetMonitorWidth(1);
        int secondaryMonitorHeight = GetMonitorHeight(1);
        int winPosX = -secondaryMonitorWidth + (secondaryMonitorWidth - windowWidth) / 2;
        int winPosY = (secondaryMonitorHeight - windowHeight) / 2;
        SetWindowPosition(winPosX, winPosY);
    }

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();

    return 0;
}

void UpdateDrawFrame(void) {
    BeginDrawing();
        ClearBackground(SKYBLUE);
        DrawText("Look at me, I'm a window!", 190, 200, 20, DARKGRAY);
    EndDrawing();
}

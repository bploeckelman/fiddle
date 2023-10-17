#include <stdio.h>

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "dark/style_dark.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include "common.h"
#include "rlgl.h"

// NOTE - for convenience when primary monitor is otherwise in use
//#define USE_SECONDARY_MONITOR 1

// ----------------------------------------------------------------------------
// Game state data
// ----------------------------------------------------------------------------

enum ConstExpr {
    MAP_SIZE = 9
};

typedef struct State {
    struct Window {
        int width;
        int height;
        char *title;
    } window;

    Camera2D overheadCamera;
    Camera3D firstPersonCamera;

    struct Player {
        Vector2 pos;
        Vector2 speed;
    } player;

    struct RenderTextures {
        RenderTexture overhead;
        RenderTexture firstPerson;
    } renderTextures;

    Rectangle splitScreenRect;

    u8 map[MAP_SIZE * MAP_SIZE];
    Rectangle tiles[MAP_SIZE * MAP_SIZE];
} State;

static State state = {
        .window = {
                .width = 1280,
                .height = 720,
                .title = "Fiddle"
        },

        .map = {
                1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 3, 0, 0, 0, 4, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 4, 1,
                1, 0, 2, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 0, 0, 0, 0, 0, 0, 0, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1,
        }
};

// ----------------------------------------------------------------------------
// Forward declarations
// ----------------------------------------------------------------------------

static void UpdateDrawFrame(void);

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------

int main() {
    InitWindow(state.window.width, state.window.height, state.window.title);

    GuiLoadStyleDark();

#if defined(USE_SECONDARY_MONITOR)
    // NOTE - this forces fullscreen which is kind of shitty...
    // SetWindowMonitor(1);

    // manually position it in the center of the secondary monitor
    int secondaryMonitorWidth = GetMonitorWidth(1);
    int secondaryMonitorHeight = GetMonitorHeight(1);
    int winPosX = -secondaryMonitorWidth + (secondaryMonitorWidth - windowWidth) / 2;
    int winPosY = (secondaryMonitorHeight - windowHeight) / 2;
    SetWindowPosition(winPosX, winPosY);
#endif

    state.player.pos   = (Vector2) { .x = 100, .y = 100 };
    state.player.speed = (Vector2) { .x = 500, .y = 500 };

    state.overheadCamera = (Camera2D) {
        .offset = (Vector2) {
            .x = (float) state.window.width / 2.0f,
            .y = (float) state.window.height / 2.0f
        },
        .target = state.player.pos,
        .rotation = 0,
        .zoom = 1
    };
    state.firstPersonCamera = (Camera3D) {
        .position = (Vector3) { -3, 3, 0 },
        .target = (Vector3) { 0, 3, 0 },
        .up = (Vector3) { 0, 1, 0 },
        .fovy = 45,
        .projection = CAMERA_PERSPECTIVE
    };

    state.renderTextures = (struct RenderTextures) {
        .overhead = LoadRenderTexture(state.window.width / 2, state.window.height),
        .firstPerson = LoadRenderTexture(state.window.width / 2, state.window.height)
    };

    state.splitScreenRect = (Rectangle) {
        0, 0,
        (float) state.renderTextures.overhead.texture.width,
        (float) -state.renderTextures.overhead.texture.height
    };

    // load map data for visualization
    const float tileSize = 50;
    for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
        int x = i % MAP_SIZE;
        int y = i / MAP_SIZE;
        state.tiles[i] = (Rectangle) {
            .x = x * tileSize,
            .y = y * tileSize,
            .width = tileSize,
            .height = tileSize
        };
    }

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    UnloadRenderTexture(state.renderTextures.overhead);
    UnloadRenderTexture(state.renderTextures.firstPerson);

    CloseWindow();

    return 0;
}

// ----------------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------------

static Color getMapColor(int mapIndex) {
    if (mapIndex < 0 || mapIndex >= MAP_SIZE * MAP_SIZE) {
        return MAGENTA;
    }

    switch (state.map[mapIndex]) {
        default: return MAGENTA;
        case 0: return DARKGRAY;
        case 1: return BLUE;
        case 2: return GREEN;
        case 3: return YELLOW;
        case 4: return RED;
    }
}

static void UpdateFrame(struct Player *player, Camera2D *camera, Camera3D *firstPersonCamera) {
    // handle movement input
    if      (IsKeyDown(KEY_A)) player->pos.x -= player->speed.x * GetFrameTime();
    else if (IsKeyDown(KEY_D)) player->pos.x += player->speed.x * GetFrameTime();
    if      (IsKeyDown(KEY_W)) player->pos.y -= player->speed.y * GetFrameTime();
    else if (IsKeyDown(KEY_S)) player->pos.y += player->speed.y * GetFrameTime();

    // update the camera based on the player's (possibly moved) position
    camera->target = player->pos;

    // handle rotation input
    if      (IsKeyDown(KEY_E)) camera->rotation++;
    else if (IsKeyDown(KEY_Q)) camera->rotation--;

    // constrain camera rotation
    const float maxRotation = 40.0f;
    if      (camera->rotation >  maxRotation) camera->rotation =  maxRotation;
    else if (camera->rotation < -maxRotation) camera->rotation = -maxRotation;

    // handle zoom input
    camera->zoom += GetMouseWheelMove() * 0.05f;

    // constrain camera zoom
    const float minZoom = 0.1f;
    const float maxZoom = 3.0f;
    if      (camera->zoom > maxZoom) camera->zoom = maxZoom;
    else if (camera->zoom < minZoom) camera->zoom = minZoom;

    // handle camera reset (rotation and zoom only)
    if (IsKeyPressed(KEY_R)) {
        camera->rotation = 0.0f;
        camera->zoom = 1.0f;
    }

    // TODO - switch this so the first person cam updates via user input and the overhead cam follows where the 'player' is on the map
    // update first person cam based on movement in overhead cam
//    firstPersonCamera->position = (Vector3) { player->pos.x, 0, player->pos.y };
}

static void UpdateDrawFrame(void) {
    UpdateFrame(&state.player, &state.overheadCamera, &state.firstPersonCamera);

    // draw to overhead texture
    BeginTextureMode(state.renderTextures.overhead);
    {
        ClearBackground(SKYBLUE);

        BeginMode2D(state.overheadCamera);
        {
            // draw a grid centered around 0,0
            rlPushMatrix();
            rlTranslatef(0, 25 * 50, 0);
            rlRotatef(90, 1, 0, 0);
            DrawGrid(100, 50);
            rlPopMatrix();

            // draw the map tiles
            for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
                DrawRectangleRec(state.tiles[i], getMapColor(i));
            }

            // draw the player
            DrawCircleV(state.player.pos, 10.0f, GOLD);
            DrawCircleV(state.player.pos, 8.0f, PURPLE);
        }
        EndMode2D();

        // not sure what this is about
        DrawRectangle(0, 0, GetScreenWidth() / 2, 40, Fade(RAYWHITE, 0.8f));
        DrawText("Overhead", 10, 10, 20, MAROON);
    }
    EndTextureMode();


    BeginTextureMode(state.renderTextures.firstPerson);
    {
        ClearBackground(SKYBLUE);

        BeginMode3D(state.firstPersonCamera);
        {
            // Draw scene: grid of cube trees on a plane to make a "world"
            DrawPlane((Vector3){ 0, 0, 0 }, (Vector2){ 50, 50 }, BEIGE); // Simple world plane

            const int count = 5;
            const float spacing = 4;
            for (float x = -count*spacing; x <= count*spacing; x += spacing) {
                for (float z = -count*spacing; z <= count*spacing; z += spacing) {
                    DrawCube((Vector3) { x, 1.5f, z }, 1, 1, 1, LIME);
                    DrawCube((Vector3) { x, 0.5f, z }, 0.25f, 1, 0.25f, BROWN);
                }
            }

            // Draw a cube at each player's position
            DrawCube((Vector3) { 0, 0, 0 }, 1, 1, 1, RED);
            DrawCube((Vector3) { 50, 0, 50 }, 1, 1, 1, BLUE);
        }
        EndMode3D();

        // not sure what this is about
        DrawRectangle(0, 0, GetScreenWidth() / 2, 40, Fade(RAYWHITE, 0.8f));
        DrawText("FirstPerson", 10, 10, 20, MAROON);
    }
    EndTextureMode();


    BeginDrawing();
    {
        ClearBackground(BLACK);

        DrawTextureRec(state.renderTextures.overhead.texture, state.splitScreenRect, (Vector2) { 0, 0 }, WHITE);
        DrawTextureRec(state.renderTextures.firstPerson.texture, state.splitScreenRect, (Vector2) { GetScreenWidth() / 2, 0 }, WHITE);

        // draw ui
//        {
//            Rectangle guiArea = (Rectangle) {0, 0, 300, 80};
//            DrawRectangleRounded(guiArea, 0.1f, 20, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
//
//            static bool checked = false;
//            GuiCheckBox(
//                    (Rectangle) {10, 40, 20, 20},
//                    checked ? GuiIconText(ICON_OK_TICK, "Checkbox") : GuiIconText(ICON_BOX, "Checkbox"),
//                    &checked
//            );
//            if (checked) {
//                DrawText("Hey, you checked my box!", 10, 10, 20, LIGHTGRAY);
//            } else {
//                DrawText("Look at me, I'm a window!", 10, 10, 20, LIGHTGRAY);
//            }
//        }
    }
    EndDrawing();
}

#include <stdio.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "dark/style_dark.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// TODO - verify that android/web can't support glsl 330
#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION            100
#endif

#include "common.h"

// NOTE - for convenience when primary monitor is otherwise in use
//#define USE_SECONDARY_MONITOR

// ----------------------------------------------------------------------------
// Game state data
// ----------------------------------------------------------------------------

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

static void InitGameData(void);
static void UnloadGameData(void);
static void UpdateDrawFrame(void);

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)

    InitWindow(state.window.width, state.window.height, state.window.title);
    InitGameData();

    GuiLoadStyleDark();

#if defined(USE_SECONDARY_MONITOR)
    // NOTE - this forces fullscreen which is kind of shitty...
    // SetWindowMonitor(1);

    // manually position it in the center of the secondary monitor
    int secondaryMonitorWidth = GetMonitorWidth(1);
    int secondaryMonitorHeight = GetMonitorHeight(1);
    int winPosX = -secondaryMonitorWidth + (secondaryMonitorWidth - state.window.width) / 2;
    int winPosY = (secondaryMonitorHeight - state.window.height) / 2;
    SetWindowPosition(winPosX, winPosY);
#endif

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    UnloadGameData();

    CloseWindow();

    return 0;
}

// ----------------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------------

static void InitGameData() {
    state.player.pos   = (Vector2) { .x = 100, .y = 100 };
    state.player.speed = (Vector2) { .x = 500, .y = 500 };

    state.cameras.overhead = (Camera2D) {
            .offset = (Vector2) {
                    .x = (float) state.window.width / 2.0f,
                    .y = (float) state.window.height / 2.0f
            },
            .target = state.player.pos,
            .rotation = 0,
            .zoom = 1
    };
    state.cameras.firstPerson = (Camera3D) {
            .position = (Vector3) { -3, 3, 0 },
            .target = (Vector3) { 0, 2.25f, 0 },
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

    // load scene data
    state.scene = (struct Scene) {
        .lights = {0},
        .shader =LoadShader(
                TextFormat("data/shaders/glsl%i/lighting.vert", GLSL_VERSION),
                TextFormat("data/shaders/glsl%i/lighting.frag", GLSL_VERSION)),
        .coin = LoadModel("data/models/coin.gltf.glb"),
        .ground = LoadModelFromMesh(GenMeshPlane(50, 50, 50, 50)),
        .treeTrunk = LoadModelFromMesh(GenMeshCube(1, 1, 1)),
        .treeCanopy = LoadModelFromMesh(GenMeshCube(0.25f, 1, 0.25f)),
        .coinRotY = 0.f,
        .coinRotZ = 90.f
    };

    // Get some required shader locations
    state.scene.shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(state.scene.shader, "viewPos");
    // NOTE: "matModel" location name is automatically assigned on shader loading,
    // no need to get the location again if using that uniform name
    //state.scene.shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(state.scene.shader, "matModel");

    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(state.scene.shader, "ambient");
    SetShaderValue(state.scene.shader, ambientLoc, (float[4]) { 0.1f, 0.1f, 0.1f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Assign out lighting shader to models
    state.scene.coin.materials[0].shader = state.scene.shader;
    state.scene.ground.materials[0].shader = state.scene.shader;
    state.scene.treeTrunk.materials[0].shader = state.scene.shader;
    state.scene.treeCanopy.materials[0].shader = state.scene.shader;

    // Create lights
    state.scene.lights[0] = CreateLight(LIGHT_POINT, (Vector3){ -2, 1, -2 }, Vector3Zero(), YELLOW, state.scene.shader);
    state.scene.lights[1] = CreateLight(LIGHT_POINT, (Vector3){  2, 1,  2 }, Vector3Zero(), RED,    state.scene.shader);
    state.scene.lights[2] = CreateLight(LIGHT_POINT, (Vector3){ -2, 1,  2 }, Vector3Zero(), GREEN,  state.scene.shader);
    state.scene.lights[3] = CreateLight(LIGHT_POINT, (Vector3){  2, 1, -2 }, Vector3Zero(), BLUE,   state.scene.shader);
}

static void UnloadGameData() {
    UnloadModel(state.scene.coin);
    UnloadModel(state.scene.ground);
    UnloadModel(state.scene.treeTrunk);
    UnloadModel(state.scene.treeCanopy);

    UnloadShader(state.scene.shader);

    UnloadRenderTexture(state.renderTextures.overhead);
    UnloadRenderTexture(state.renderTextures.firstPerson);
}

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

static void UpdateFrame(struct Scene *scene, struct Player *player, Camera2D *camera, Camera3D *firstPersonCamera) {
    float dt = GetFrameTime();

    // update the first person camera using the raylib built-in camera controls
    UpdateCamera(firstPersonCamera, CAMERA_PERSPECTIVE);
    // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
    float cameraPos[3] = {
            firstPersonCamera->position.x,
            firstPersonCamera->position.y,
            firstPersonCamera->position.z
    };
    SetShaderValue(scene->shader, scene->shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

    // handle movement input
    if      (IsKeyDown(KEY_A)) player->pos.x -= player->speed.x * dt;
    else if (IsKeyDown(KEY_D)) player->pos.x += player->speed.x * dt;
    if      (IsKeyDown(KEY_W)) player->pos.y -= player->speed.y * dt;
    else if (IsKeyDown(KEY_S)) player->pos.y += player->speed.y * dt;

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

    // rotate the coin
    float rotationSpeed = 300.f;
    state.scene.coinRotY += rotationSpeed * dt;
    state.scene.coin.transform = MatrixMultiply(
            MatrixRotateZ(DEG2RAD * state.scene.coinRotZ),
            MatrixRotateY(DEG2RAD * state.scene.coinRotY));
}

static void UpdateDrawFrame(void) {
    UpdateFrame(&state.scene, &state.player, &state.cameras.overhead, &state.cameras.firstPerson);

    // draw to overhead texture
    BeginTextureMode(state.renderTextures.overhead);
    {
        ClearBackground(SKYBLUE);

        BeginMode2D(state.cameras.overhead);
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

        BeginMode3D(state.cameras.firstPerson);
        {
            // Draw scene: grid of cube trees on a plane to make a "world"
            DrawModel(state.scene.ground, (Vector3){ 0, 0, 0 }, 1, BEIGE); // Simple world plane

            const int count = 5;
            const float spacing = 4;
            for (float x = -count*spacing; x <= count*spacing; x += spacing) {
                for (float z = -count*spacing; z <= count*spacing; z += spacing) {
                    DrawModel(state.scene.treeTrunk, (Vector3) { x, 1.5f, z }, 1, LIME);
                    DrawModel(state.scene.treeCanopy, (Vector3) { x, 0.5f, z }, 1, BROWN);
                }
            }

            // Draw a 3d model for testing
            DrawModel(state.scene.coin, (Vector3) { 0, 3.f, 0 }, 1, WHITE);
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

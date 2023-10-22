#ifndef FIDDLE_COMMON_H
#define FIDDLE_COMMON_H

#include <stdint.h>
#include <stdbool.h>

typedef float f32;
typedef double f64;
typedef uint8_t b8;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

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

    struct Cameras {
        Camera2D overhead;
        Camera3D firstPerson;
    } cameras;

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

    struct Scene {
        Light lights[MAX_LIGHTS];
        Shader shader;

        Model coin;
        Model ground;
        Model treeTrunk;
        Model treeCanopy;

        float coinRotY;
        float coinRotZ;
    } scene;
} State;

#endif //FIDDLE_COMMON_H

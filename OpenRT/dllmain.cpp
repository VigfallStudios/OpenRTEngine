#include <cstdio>
#include "EngineAPI.h"
#include <cmath>

EngineAPI* gEngine = nullptr;
float t = 0.0f;

extern "C" {

// Runs at the start of the game.
__declspec(dllexport) void Init(EngineAPI* api) {
    gEngine = api;
    gEngine->Log("Game DLL Init()!");
}

// Runs every frame.
__declspec(dllexport) void Tick(float deltaTime) {
    char buf[128];
    t += deltaTime;
    gEngine->cameraPosition->x = sinf(t) * 5.0f;
    gEngine->cameraPosition->z = cosf(t) * 5.0f;

    sprintf(buf, "Cam Pos: (%.2f, %.2f, %.2f)",
        gEngine->cameraPosition->x,
        gEngine->cameraPosition->y,
        gEngine->cameraPosition->z);
    gEngine->Log(buf);
}

// Runs when game ends.
__declspec(dllexport) void Shutdown() {
    gEngine->Log("Game DLL Shutdown()");
}

}

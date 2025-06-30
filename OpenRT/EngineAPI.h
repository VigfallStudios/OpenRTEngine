#pragma once

struct Vec3 {
    float x, y, z;
};

struct EngineAPI {
    void (*Log)(const char* msg);
    float (*GetTime)();

    Vec3* cameraPosition;
};
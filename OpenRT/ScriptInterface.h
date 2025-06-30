#pragma once

extern "C" {
    __declspec(dllexport) void Init();
    __declspec(dllexport) void Tick(float deltaTime);
    __declspec(dllexport) void Shutdown();
}

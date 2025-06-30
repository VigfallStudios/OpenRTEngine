#define _USE_MATH_DEFINES
#include <iostream>
#include <SDL3/SDL.h>
#include <cmath>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <algorithm>
#ifdef max
#undef max
#endif
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#include "Debug.h"
#include "Editor.h"
#include "Mesh.h"
#include "ScriptInterface.h"
#include "EngineAPI.h"

// Viewport
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

int wWidth = 800;
int wHeight = 600;

std::queue<std::string> commandQueue;
std::mutex commandMutex;
std::atomic<bool> consoleRunning(true);

void ConsoleThread() {
    std::string input;
    while (consoleRunning) {
        std::getline(std::cin, input);
        if (!input.empty()) {
            std::lock_guard<std::mutex> lock(commandMutex);
            commandQueue.push(input);
        }
    }
}

#define BO_DOCK_WIDTH 50
#define BROWSER_HEIGHT 150

Vec3 cam{ 0, 2, 8 };

typedef void(*InitFunc)(EngineAPI*);
typedef void(*TickFunc)(float);
typedef void(*ShutdownFunc)();

HMODULE scriptDLL = nullptr;
InitFunc GameInit = nullptr;
TickFunc GameTick = nullptr;
ShutdownFunc GameShutdown = nullptr;

bool LoadGameDLL(const char* dllPath) {
    scriptDLL = LoadLibraryA(dllPath);
    if (!scriptDLL) return false;

    GameInit = (InitFunc)GetProcAddress(scriptDLL, "Init");
    GameTick = (TickFunc)GetProcAddress(scriptDLL, "Tick");
    GameShutdown = (ShutdownFunc)GetProcAddress(scriptDLL, "Shutdown");

    return GameInit && GameTick && GameShutdown;
}

float EngineTime = 0.0f;
EngineAPI api;

void MyLog(const char* msg) {
    printf("[ENGINE] %s\n", msg);
}

float MyGetTime() {
    return EngineTime;
}

// === MAIN ===
int main(int argc, char* argv[]) {
    Debug.Log("OpenRT starting up...");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Debug.LogError(SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    SDL_Window* window = SDL_CreateWindow("OpenRT 0.5f - Windows Standalone", wWidth, wHeight, SDL_WINDOW_OPENGL);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);

    if (!window) {
        Debug.LogError(SDL_GetError());
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    Uint64 lastTime = SDL_GetTicks();
    int frameCount = 0;
    float fps = 0.0f;

    float deltaTime = 0.0f;
    bool playMode = false;

    // GL Setup
    glViewport(0, 0, 800, 600);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 800.0 / 600.0, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat light_pos[] = { 0.0f, 5.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    float angle = 0.0f;

    Uint64 lastFrameTime = SDL_GetTicks();  // for deltaTime
    Uint64 lastFPSTime = lastFrameTime;     // for FPS

    Mesh cube;
    cube.position = Vec3{ 0, 0, 0 };
    cube.material.r = 1.0f;
    cube.material.g = 0.5f;
    cube.material.b = 0.2f;
    cube.material.wireframe = false;
    cube.material.lit = true;

    if (LoadGameDLL("GameDLL.dll")) {
        api.Log = MyLog;
        api.GetTime = MyGetTime;
        api.cameraPosition = &cam;
        GameInit(&api);
    }

    while (running) {
        // === Handle input ===
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            HandleUIEvents(&event);
        }

        const bool* keys = SDL_GetKeyboardState(NULL);

        // --- Clear and Sky Gradient ---
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === Sky Gradient Pass ===
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, SCR_WIDTH, SCR_HEIGHT, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        glBegin(GL_QUADS);
        glColor3f(0.1f, 0.3f, 0.6f); // Top
        glVertex2f(0, 0);
        glVertex2f(SCR_WIDTH, 0);
        glColor3f(0.9f, 0.6f, 0.3f); // Bottom
        glVertex2f(SCR_WIDTH, SCR_HEIGHT);
        glVertex2f(0, SCR_HEIGHT);
        glEnd();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        // === Scene Render ===
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, (float)SCR_WIDTH / SCR_HEIGHT, 0.1, 100.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(cam.x, cam.y, cam.z, cam.x, cam.y, cam.z - 1, 0, 1, 0);

        if (GameTick) GameTick(deltaTime);

        cube.Update(deltaTime);
        cube.Draw();

        SDL_GL_SwapWindow(window);

        Uint64 currentTime = SDL_GetTicks();
        deltaTime = (currentTime - lastFrameTime) / 1000.0f; // seconds
        lastFrameTime = currentTime;

        // === FPS COUNTER ===
        frameCount++;
        if (currentTime - lastFPSTime >= 1000) {
            fps = frameCount * 1000.0f / (currentTime - lastFPSTime);
            lastFPSTime = currentTime;
            frameCount = 0;

            std::string title = "OpenRT 0.5f - FPS: " + std::to_string((int)fps);
            SDL_SetWindowTitle(window, title.c_str());
        }
    }

    if (GameShutdown) GameShutdown();
    FreeLibrary(scriptDLL);

    SDL_DestroyWindow(window);
    SDL_Quit();

    Debug.Log("OpenRT closed.");

    return 0;
}

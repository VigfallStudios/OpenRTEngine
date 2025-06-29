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

#include "Debug.h"
#include "Scripting.h"
#include "Vector.h"
#include "Commands.h"
#include "Editor.h"

// Viewport
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

int wWidth = 1600;
int wHeight = 900;

float Q_rsqrt(float number) {
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y = number;
    i = *(long*)&y;                 // bit-level hacking
    i = 0x5f3759df - (i >> 1);      // magic constant
    y = *(float*)&i;
    y = y * (threehalfs - (x2 * y * y)); // 1st iteration
    // Optional: 2nd iteration for better accuracy
    // y  = y * (threehalfs - (x2 * y * y)); 
    return y;
}

// Data structures
struct Ray {
    Vec3 origin;
    Vec3 dir;
};

float HitSphere(const Vec3& center, float radius, const Ray& ray) {
    Vec3 oc = ray.origin - center;
    float a = ray.dir.dot(ray.dir);
    float b = 2.0f * oc.dot(ray.dir);
    float c = oc.dot(oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return -1.0f;
    return (-b - std::sqrt(discriminant)) / (2.0f * a);
}

template <typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

inline Uint32 SkyColor(const Vec3& dir) {
    // `t` goes from 0 (bottom) to 1 (top)
    float t = clamp(dir.y * 0.5f + 0.5f, 0.0f, 1.0f);

    // UE4-like gradient: interpolate between bottom and top colors
    Uint8 r = (Uint8)((1.0f - t) * 200 + t * 0);     // 200 -> 0
    Uint8 g = (Uint8)((1.0f - t) * 220 + t * 38);    // 220 -> 38
    Uint8 b = (Uint8)((1.0f - t) * 255 + t * 84);    // 255 -> 84

    return (255u << 24) | (r << 16) | (g << 8) | b;
}

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

template<typename T>
T Max(T a, T b) {
    return (a > b) ? a : b;
}

// === MAIN ===
int main(int argc, char* argv[]) {
    Debug.Log("OpenRT starting up...");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Debug.LogError(SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("OpenRT 0.2f - Windows Standalone", wWidth, wHeight, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);

    if (!window || !renderer || !framebuffer) {
        Debug.LogError(SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ScriptCompiler sCompiler;
    CommandManager cmdManager(sCompiler);
    std::thread console(ConsoleThread);

    bool running = true;
    SDL_Event event;

    Uint64 lastTime = SDL_GetTicks();
    int frameCount = 0;
    float fps = 0.0f;

    float deltaTime = 0.0f;

    // Camera & scene
    Vec3 cam(0, 0, -1);
    Vec3 sphereCenter(0, 0, 1);
    float sphereRadius = 0.5f;
    Vec3 lightDir = Vec3(1, 1, -1).normalize(); // Move above the main loop

    int step = 2;

    std::vector<Vec3> rayDirs(SCR_WIDTH * SCR_HEIGHT);
    float aspect = (float)SCR_WIDTH / SCR_HEIGHT;

    for (int y = 0; y < SCR_HEIGHT; ++y) {
        for (int x = 0; x < SCR_WIDTH; ++x) {
            float u = (float)x / SCR_WIDTH;
            float v = (float)y / SCR_HEIGHT;

            float ndcX = (2.0f * u - 1.0f) * aspect;
            float ndcY = 1.0f - 2.0f * v;

            rayDirs[y * SCR_WIDTH + x] = Vec3(ndcX, ndcY, 1.0f).normalize();
        }
    }

    UIButton myButton = { {400, 50, 100, 100}, "Play" };
    UIButton objectSphere = { {5, MENU_BAR_HEIGHT + 40, 390, 80}, "Sphere" };

    bool playMode = false;

    while (running) {
        // === Handle input ===
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            HandleUIEvents(&event);
        }

        {
            std::lock_guard<std::mutex> lock(commandMutex);
            while (!commandQueue.empty()) {
                std::string cmd = commandQueue.front();
                commandQueue.pop();
                cmdManager.RunString(cmd);
            }
        }

        const bool* keys = SDL_GetKeyboardState(NULL);

        SDL_RenderClear(renderer);

        if (playMode)
        {
            // === Lock framebuffer ===
            void* pixels;
            int pitchBytes;
            if (SDL_LockTexture(framebuffer, nullptr, &pixels, &pitchBytes) < 0) continue;

            Uint32* buffer = (Uint32*)pixels;
            int pitch = pitchBytes / sizeof(Uint32);

            for (int y = 0; y < SCR_HEIGHT; y += step) {
                for (int x = 0; x < SCR_WIDTH; x += step) {
                    Vec3 dir = rayDirs[y * SCR_WIDTH + x];
                    Ray ray{ cam, dir };

                    float t = HitSphere(sphereCenter, sphereRadius, ray);
                    Uint32 color;

                    if (t > 0.0f) {
                        Vec3 hit = ray.origin + ray.dir * t;
                        Vec3 normal = (hit - sphereCenter).normalize();
                        float brightness = Max(0.0f, normal.dot(lightDir));

                        Uint8 r = (Uint8)(255 * brightness);
                        Uint8 g = (Uint8)(50 * brightness);
                        Uint8 b = (Uint8)(50 * brightness);
                        color = (255u << 24) | (r << 16) | (g << 8) | b;
                    }
                    else {
                        color = SkyColor(ray.dir);
                    }

                    for (int dy = 0; dy < step; ++dy) {
                        for (int dx = 0; dx < step; ++dx) {
                            int px = x + dx;
                            int py = y + dy;
                            if (px < SCR_WIDTH && py < SCR_HEIGHT)
                                buffer[py * pitch + px] = color;
                        }
                    }
                }
            }
        }
        else
        {
            void* pixels;
            int pitchBytes;
            if (SDL_LockTexture(framebuffer, nullptr, &pixels, &pitchBytes) < 0) continue;

            Uint32* buffer = (Uint32*)pixels;
            int pitch = pitchBytes / sizeof(Uint32);

            for (int y = 0; y < SCR_HEIGHT; y += step) {
                for (int x = 0; x < SCR_WIDTH; x += step) {
                    for (int dy = 0; dy < step; ++dy) {
                        for (int dx = 0; dx < step; ++dx) {
                            int px = x + dx;
                            int py = y + dy;
                            if (px < SCR_WIDTH && py < SCR_HEIGHT)
                                buffer[py * pitch + px] = SkyColor(Vec3(0, 0, 1));
                        }
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // R, G, B, A
        SDL_RenderClear(renderer);

        SDL_UnlockTexture(framebuffer);
        // Calculate center position to render framebuffer
        float offsetX = (wWidth - SCR_WIDTH) * 0.5f;
        float offsetY = (wHeight - SCR_HEIGHT) * 0.5f;

        SDL_FRect dstRect = { offsetX, offsetY, (float)SCR_WIDTH, (float)SCR_HEIGHT };
        SDL_RenderTexture(renderer, framebuffer, nullptr, &dstRect);

        if (UpdateUIButton(myButton, uiInput)) {
            Debug.Log("Play Mode Toggled");
            playMode = !playMode;
        }

        if (UpdateUIButton(objectSphere, uiInput))
        {
            Debug.Log("Creating sphere");
            MessageBox(NULL, L"Object not created due to ORT_NOT_IMPLEMENTED", L"Could not create GameObject", MB_OK | MB_ICONERROR);
        }

        // Menu bar background
        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);

        SDL_FRect bar = { 0, 0, wWidth, MENU_BAR_HEIGHT };
        SDL_RenderFillRect(renderer, &bar);

        SDL_FRect playBar = { offsetX, MENU_BAR_HEIGHT + 2, SCR_WIDTH, offsetY - 10 };
        SDL_RenderFillRect(renderer, &playBar);
        DrawUIButton(renderer, myButton);

        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_FRect bo = { 0, MENU_BAR_HEIGHT, offsetX - 5, wHeight };
        SDL_RenderFillRect(renderer, &bo);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        DrawText(renderer, "Basic Objects", 5, MENU_BAR_HEIGHT + 10, 3);
        DrawUIButton(renderer, objectSphere);

        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_FRect browser = { offsetX - 4, wHeight - BROWSER_HEIGHT, wWidth, BROWSER_HEIGHT };
        SDL_RenderFillRect(renderer, &browser);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        DrawText(renderer, "Browser", offsetX - 4, (wHeight - BROWSER_HEIGHT) + 10, 2);

        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_FRect bo1 = { wWidth - offsetX - 5, MENU_BAR_HEIGHT, offsetX - 5, wHeight };
        SDL_RenderFillRect(renderer, &bo1);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        DrawText(renderer, "Scene", wWidth - offsetX, MENU_BAR_HEIGHT + 10, 3);

        // Create menu items
        UIMenuItem fileItem = { {10, 0, 50, MENU_BAR_HEIGHT}, "File" };
        UIMenuItem editItem = { {70, 0, 50, MENU_BAR_HEIGHT}, "Edit" };
        UIMenuItem viewItem = { {130, 0, 50, MENU_BAR_HEIGHT}, "View" };

        // Update and draw menu items
        UpdateMenuItem(fileItem, uiInput);
        UpdateMenuItem(editItem, uiInput);
        UpdateMenuItem(viewItem, uiInput);

        DrawMenuItem(renderer, fileItem);
        DrawMenuItem(renderer, editItem);
        DrawMenuItem(renderer, viewItem);

        // Respond to clicks
        if (fileItem.clicked) {
            Debug.Log("File menu clicked!");
        }

        uiInput.mouseClicked = false;

        SDL_RenderPresent(renderer);

        // === FPS COUNTER ===
        frameCount++;
        Uint64 currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= 1000) {
            fps = frameCount * 1000.0f / (currentTime - lastTime);
            lastTime = currentTime;
            frameCount = 0;

            std::string title = "OpenRT - FPS: " + std::to_string((int)fps);
            SDL_SetWindowTitle(window, title.c_str());
        }

        deltaTime = (currentTime - lastTime) / 1000.0f;

        if (playMode)
            sCompiler.Update(&cam, keys); // Script system tick
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    consoleRunning = false;
    console.join(); // Wait for thread to finish

    Debug.Log("OpenRT closed.");

    return 0;
}

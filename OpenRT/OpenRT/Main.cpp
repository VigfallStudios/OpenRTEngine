#define _USE_MATH_DEFINES
#include <iostream>
#include <SDL3/SDL.h>
#include <cmath>
#include <string>

#include "Debug.h"
#include "Scripting.h"

// Viewport
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

int wWidth = 800;
int wHeight = 600;

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
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    Vec3 normalize() const {
        float len = std::sqrt(x * x + y * y + z * z);
        return Vec3(x / len, y / len, z / len);
    }
};

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

inline Uint32 SkyColor(const Vec3& dir) {
    float t = 0.5f * (dir.y + 1.0f);
    Uint8 r = (Uint8)((1 - t) * 40 + t * 120);
    Uint8 g = (Uint8)((1 - t) * 60 + t * 180);
    Uint8 b = (Uint8)((1 - t) * 90 + t * 255);
    return (255u << 24) | (r << 16) | (g << 8) | b;
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
    sCompiler.AddScript("test.ors");

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

    while (running) {
        // === Handle input ===
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        const bool* keys = SDL_GetKeyboardState(NULL);

        SDL_RenderClear(renderer);

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
                    float brightness = std::max(0.0f, normal.dot(lightDir));

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

        SDL_UnlockTexture(framebuffer);
        SDL_RenderTexture(renderer, framebuffer, nullptr, nullptr);

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

        sCompiler.Update(); // Script system tick
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    Debug.Log("OpenRT closed.");

    return 0;
}

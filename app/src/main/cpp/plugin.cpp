// Overlay Lab: biblioteca dinamica carregada pelo proprio aplicativo.
// Nao injeta em processos alheios nem le/escreve memoria de terceiros.
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include "overlay_api.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <imgui.h>
#include <imgui_impl_android.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

namespace {
constexpr const char* TAG = "OverlayLab";
EGLDisplay display = EGL_NO_DISPLAY;
EGLSurface surface = EGL_NO_SURFACE;
EGLContext context = EGL_NO_CONTEXT;
ANativeWindow* currentWindow = nullptr;
bool initialized = false;
bool menuOpen = true;

struct Vec3 { float x, y, z; };
struct Actor { const char* name; Vec3 origin; float health; };
struct Settings {
    bool box = true;
    bool name = true;
    bool health = true;
    bool distance = true;
    bool fov = true;
    bool aim = false;
    bool noRecoil = false;
    float fovRadius = 195.f;
    float smooth = 8.f;
} options;

Vec3 camera{0.f, 0.f, 0.f};
ImVec2 cursor{0.f, 0.f};
const std::array<Actor, 6> actors = {{
    {"Alfa",   {-5.7f, 0.f, 13.f}, 95.f},
    {"Bravo",  {-2.5f, 0.f, 10.f}, 76.f},
    {"Charlie", { 0.8f, 0.f, 14.f}, 43.f},
    {"Delta",  { 3.2f, 0.f, 12.f}, 100.f},
    {"Echo",   { 7.1f, 0.f, 19.f}, 62.f},
    {"Foxtrot",{-8.1f, 0.f, 21.f}, 88.f}
}};

float clamp01(float x) { return std::max(0.f, std::min(1.f, x)); }
float distance2(ImVec2 a, ImVec2 b) {
    const float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Projecao de perspectiva da cena local de testes.
bool worldToScreen(Vec3 pos, ImVec2 size, ImVec2& out) {
    const float z = pos.z - camera.z;
    if (z <= 0.5f) return false;
    const float focal = size.y * 1.1f;
    out.x = size.x * .5f + (pos.x - camera.x) * focal / z;
    out.y = size.y * .59f - (pos.y - camera.y) * focal / z;
    return out.x > -size.x && out.x < size.x * 2.f &&
           out.y > -size.y && out.y < size.y * 2.f;
}

void drawScene(float timeSec, float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 screen = io.DisplaySize;
    if (screen.x <= 0.f || screen.y <= 0.f) return;
    const ImVec2 center(screen.x * .5f, screen.y * .52f);
    if (cursor.x <= 0.f || cursor.y <= 0.f) cursor = center;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    // Grade e linha do horizonte da cena demonstrativa.
    const ImU32 grid = IM_COL32(43, 64, 81, 130);
    for (int i = 0; i < 15; ++i) {
        const float x = screen.x * (i + 1.f) / 16.f;
        draw->AddLine(ImVec2(x, 0), ImVec2(x, screen.y), grid, 1.f);
    }
    for (int i = 0; i < 8; ++i) {
        const float y = screen.y * (i + 1.f) / 9.f;
        draw->AddLine(ImVec2(0, y), ImVec2(screen.x, y), grid, 1.f);
    }
    draw->AddText(ImVec2(22, screen.y - 36), IM_COL32(164, 190, 207, 255),
                  "CENA LOCAL DE TESTE - SEM CONEXAO COM JOGOS");

    if (options.fov) {
        draw->AddCircle(center, options.fovRadius,
                        IM_COL32(110, 185, 255, 215), 96, 1.8f);
    }

    ImVec2 bestAim = center;
    float closest = options.fovRadius;
    bool found = false;
    for (size_t i = 0; i < actors.size(); ++i) {
        const auto& a = actors[i];
        Vec3 feet = a.origin;
        feet.x += std::sin(timeSec * .65f + float(i) * 1.3f) * .42f;
        Vec3 head = feet;
        head.y += 1.72f;
        ImVec2 feet2D, head2D;
        if (!worldToScreen(feet, screen, feet2D) ||
            !worldToScreen(head, screen, head2D)) continue;

        const float h = feet2D.y - head2D.y;
        if (h < 10.f) continue;
        const float w = h * .48f;
        const ImVec2 tl(head2D.x - w * .5f, head2D.y);
        const ImVec2 br(head2D.x + w * .5f, feet2D.y);
        const ImU32 green = IM_COL32(89, 241, 167, 255);
        const ImU32 white = IM_COL32(244, 250, 255, 255);
        if (options.box) draw->AddRect(tl, br, green, 3.f, 0, 2.f);
        if (options.name) {
            const ImVec2 label = ImGui::CalcTextSize(a.name);
            draw->AddText(ImVec2(head2D.x - label.x * .5f, tl.y - 23.f), white, a.name);
        }
        if (options.health) {
            const float ratio = clamp01(a.health / 100.f);
            draw->AddRectFilled(ImVec2(tl.x - 10.f, tl.y),
                                ImVec2(tl.x - 5.f, br.y), IM_COL32(28, 31, 37, 230));
            draw->AddRectFilled(ImVec2(tl.x - 10.f, br.y - h * ratio),
                                ImVec2(tl.x - 5.f, br.y),
                                ratio > .5f ? green : IM_COL32(255, 149, 89, 255));
        }
        if (options.distance) {
            const float dx = feet.x - camera.x;
            const float dz = feet.z - camera.z;
            char label[64];
            std::snprintf(label, sizeof(label), "%.1f m | %.0f HP",
                          std::sqrt(dx * dx + dz * dz), a.health);
            const ImVec2 labelSize = ImGui::CalcTextSize(label);
            draw->AddText(ImVec2(head2D.x - labelSize.x * .5f, br.y + 5.f), white, label);
        }
        const ImVec2 aimPoint(head2D.x, head2D.y + h * .23f);
        const float d = distance2(aimPoint, center);
        if (d < closest) {
            closest = d;
            bestAim = aimPoint;
            found = true;
        }
    }

    // Suavizacao de mira dentro da demonstracao, sem controle de um jogo real.
    const float dt = std::max(0.f, std::min(deltaTime, .1f));
    const ImVec2 target = (options.aim && found) ? bestAim : center;
    const float alpha = 1.f - std::exp(-dt * (22.f / options.smooth));
    cursor.x += (target.x - cursor.x) * alpha;
    cursor.y += (target.y - cursor.y) * alpha;
    const float recoilX = options.noRecoil ? 0.f : std::sin(timeSec * 12.f) * 3.f;
    const float recoilY = options.noRecoil ? 0.f : std::abs(std::sin(timeSec * 8.f)) * 5.f;
    const ImVec2 reticle(cursor.x + recoilX, cursor.y + recoilY);
    const ImU32 reticleColor = options.aim && found ?
        IM_COL32(93, 249, 181, 255) : IM_COL32(250, 244, 232, 245);
    draw->AddLine(ImVec2(reticle.x - 13, reticle.y),
                  ImVec2(reticle.x + 13, reticle.y), reticleColor, 2.f);
    draw->AddLine(ImVec2(reticle.x, reticle.y - 13),
                  ImVec2(reticle.x, reticle.y + 13), reticleColor, 2.f);
    draw->AddCircle(reticle, 3.f, reticleColor, 16, 1.4f);
}

void drawUi() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 135.f, 28.f),
                            ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(.35f);
    ImGui::Begin("Botao", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button(menuOpen ? "FECHAR" : "MENU", ImVec2(100, 44)))
        menuOpen = !menuOpen;
    ImGui::End();

    if (!menuOpen) return;
    ImGui::SetNextWindowPos(ImVec2(28.f, 34.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(365.f, 0.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Overlay Lab | Android NDK", &menuOpen,
                 ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextUnformatted("Painel de testes - dados simulados");
    ImGui::Separator();
    ImGui::TextUnformatted("Visualizacao ESP");
    ImGui::Checkbox("Caixas", &options.box);
    ImGui::Checkbox("Nomes", &options.name);
    ImGui::Checkbox("Vida", &options.health);
    ImGui::Checkbox("Distancia", &options.distance);
    ImGui::Separator();
    ImGui::Checkbox("Circulo FOV", &options.fov);
    ImGui::SliderFloat("Raio FOV", &options.fovRadius, 60.f, 480.f, "%.0f px");
    ImGui::Checkbox("Mira suave (simulada)", &options.aim);
    ImGui::SliderFloat("Smoothing", &options.smooth, 1.f, 20.f, "%.1f");
    ImGui::Checkbox("Sem recuo (simulado)", &options.noRecoil);
    ImGui::Separator();
    if (ImGui::Button("Teleportar camera virtual +2m")) {
        camera.x = std::min(camera.x + 2.f, 10.f);
        camera.z = std::min(camera.z + 1.f, 8.f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Restaurar")) {
        camera = {0.f, 0.f, 0.f};
        cursor = {0.f, 0.f};
    }
    ImGui::Text("Camera: x=%.1f, z=%.1f", camera.x, camera.z);
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::End();
}

void cleanup() {
    if (initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext();
        initialized = false;
    }
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
    currentWindow = nullptr;
}

bool setup(ANativeWindow* window) {
    if (initialized || !window) return initialized;
    currentWindow = window;
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "eglInitialize falhou");
        cleanup();
        return false;
    }
    const EGLint attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_NONE
    };
    EGLConfig config = nullptr;
    EGLint count = 0;
    if (!eglChooseConfig(display, attrs, &config, 1, &count) || count < 1) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "EGL ES3 indisponivel");
        cleanup();
        return false;
    }
    EGLint format = 0;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(window, 0, 0, format);
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
    surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE ||
        !eglMakeCurrent(display, surface, surface, context)) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "EGL context/surface falhou");
        cleanup();
        return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(1.45f);
    ImGui::GetIO().FontGlobalScale = 1.5f;
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplAndroid_Init(window);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    initialized = true;
    return true;
}

void render() {
    if (!initialized) return;
    static auto previous = std::chrono::steady_clock::now();
    static auto started = previous;
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - previous).count();
    const float t = std::chrono::duration<float>(now - started).count();
    previous = now;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    drawScene(t, dt);
    drawUi();
    ImGui::Render();
    const ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(.055f, .085f, .12f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(display, surface);
}

} // namespace

// API publica do plugin. A funcao dlopen() so e usada pelo app que contem este .so.
extern "C" uint32_t overlay_api_version() { return OVERLAY_API_VERSION; }
extern "C" int overlay_init(ANativeWindow* window) { return setup(window) ? 1 : 0; }
extern "C" void overlay_render() { render(); }
extern "C" int overlay_input(AInputEvent* event) {
    return initialized ? ImGui_ImplAndroid_HandleInputEvent(event) : 0;
}
extern "C" void overlay_shutdown() { cleanup(); }

// NativeActivity host: carrega liboverlay_plugin.so no SEU proprio processo.
#include <android/log.h>
#include <android_native_app_glue.h>
#include <dlfcn.h>
#include <stdint.h>
#include "overlay_api.h"

namespace {
constexpr const char* TAG = "OverlayHost";

struct Plugin {
    void* handle = nullptr;
    overlay_api_version_proc version = nullptr;
    overlay_init_proc init = nullptr;
    overlay_render_proc render = nullptr;
    overlay_input_proc input = nullptr;
    overlay_shutdown_proc shutdown = nullptr;
    bool running = false;
} plugin;

void unload() {
    if (plugin.running && plugin.shutdown) plugin.shutdown();
    plugin.running = false;
    if (plugin.handle) dlclose(plugin.handle);
    plugin = {};
}

bool load(ANativeWindow* window) {
    if (!window) return false;
    if (!plugin.handle) {
        plugin.handle = dlopen("liboverlay_plugin.so", RTLD_NOW | RTLD_LOCAL);
        if (!plugin.handle) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "dlopen falhou: %s", dlerror());
            return false;
        }
        plugin.version = reinterpret_cast<overlay_api_version_proc>(dlsym(plugin.handle, "overlay_api_version"));
        plugin.init = reinterpret_cast<overlay_init_proc>(dlsym(plugin.handle, "overlay_init"));
        plugin.render = reinterpret_cast<overlay_render_proc>(dlsym(plugin.handle, "overlay_render"));
        plugin.input = reinterpret_cast<overlay_input_proc>(dlsym(plugin.handle, "overlay_input"));
        plugin.shutdown = reinterpret_cast<overlay_shutdown_proc>(dlsym(plugin.handle, "overlay_shutdown"));
        if (!plugin.version || !plugin.init || !plugin.render ||
            !plugin.input || !plugin.shutdown || plugin.version() != OVERLAY_API_VERSION) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Plugin: simbolos ausentes ou versao incompatível");
            unload();
            return false;
        }
    }
    plugin.running = plugin.init(window) != 0;
    if (!plugin.running) __android_log_print(ANDROID_LOG_ERROR, TAG, "overlay_init falhou");
    return plugin.running;
}

void handleCommand(android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            load(app->window);
            break;
        case APP_CMD_TERM_WINDOW:
            if (plugin.running) {
                plugin.shutdown();
                plugin.running = false;
            }
            break;
        default:
            break;
    }
}

int32_t handleInput(android_app*, AInputEvent* event) {
    return plugin.running ? plugin.input(event) : 0;
}
} // namespace

void android_main(android_app* app) {
    app_dummy();
    app->onAppCmd = handleCommand;
    app->onInputEvent = handleInput;

    for (;;) {
        int events = 0;
        android_poll_source* source = nullptr;
        const int timeout = plugin.running ? 0 : -1;
        while (ALooper_pollOnce(timeout, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                unload();
                return;
            }
        }
        if (plugin.running) plugin.render();
    }
}

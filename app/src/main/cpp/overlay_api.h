#pragma once

#include <stdint.h>
#include <android/native_window.h>
#include <android/input.h>

#define OVERLAY_API_VERSION 1u

// ABI estavel para carregar um plugin dentro do app proprietario.
// Chamar todas as funcoes na mesma thread que possui o contexto EGL.
#ifdef __cplusplus
extern "C" {
#endif
uint32_t overlay_api_version(void);
int overlay_init(ANativeWindow* window);
void overlay_render(void);
int overlay_input(AInputEvent* event);
void overlay_shutdown(void);
#ifdef __cplusplus
}
#endif

typedef uint32_t (*overlay_api_version_proc)(void);
typedef int (*overlay_init_proc)(ANativeWindow*);
typedef void (*overlay_render_proc)(void);
typedef int (*overlay_input_proc)(AInputEvent*);
typedef void (*overlay_shutdown_proc)(void);

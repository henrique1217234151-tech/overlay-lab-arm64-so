# Overlay Lab dlopen (Android ARM64)

Projeto **proprio** em C++/Android NDK que produz duas bibliotecas compartilhadas:

- `liboverlay_host.so`: entrada da NativeActivity; abre a biblioteca de visualizacao usando `dlopen` e localiza as funcoes usando `dlsym`.
- `liboverlay_plugin.so`: cria contexto EGL/OpenGL ES 3, roda Dear ImGui e exibe uma **cena simulada** de ESP (caixas, HP, distancia, nomes), FOV, smoothing, recuo e movimento da camera virtual.

Nao opera sobre nenhum outro processo, nao acessa jogos instalados, nao altera memoria externa e nao utiliza IL2CPP, offsets ou injetores.

## Requisitos

- Windows 10/11 com Android Studio, Android SDK 35, **NDK (Side by side)**, **CMake 3.22.1**, Ninja, JDK 17 e internet para obter ImGui.
- Android ARM64 (Android 8 ou mais novo), com OpenGL ES 3.0.
- Git disponivel no `PATH` para o setup automatizado de ImGui.

## Compilacao pelo Windows

1. Extraia o ZIP.
2. Instale no Android Studio: **Settings > SDK Manager > SDK Tools > NDK (Side by side) + CMake**.
3. Dê dois cliques em `compilar.bat`.
4. Ao terminar, procure em `saida/` os arquivos `liboverlay_host.so` e `liboverlay_plugin.so`.

Se o Android Studio estiver instalado no local padrao, o script encontra o NDK e o CMake. Ele baixa Dear ImGui **v1.91.9** via Git durante a primeira compilacao. Precisa de acesso a internet nessa etapa.

## Instalar o aplicativo de teste

O script `compilar.bat` gera **somente as duas bibliotecas**, nao um APK. Para gerar um APK:

1. Abra a pasta do projeto no Android Studio.
2. Execute `scripts/setup-deps.ps1`, se ainda nao executou `compilar.bat`.
3. Deixe o Android Studio sincronizar o Gradle (AGP 8.7.3, Gradle 8.9 e JDK 17). Se pedir Gradle Wrapper e nao houver wrapper, gere com `gradle wrapper --gradle-version 8.9` ou configure o Gradle instalado.
4. Selecione **Build > Build APK(s)** ou clique em **Run** com seu Android conectado.
5. O APK debug aparecera em `app/build/outputs/apk/debug/app-debug.apk`, e o Gradle empacotara as duas bibliotecas.

## Como funciona o carregamento

```text
NativeActivity -> liboverlay_host.so -> dlopen("liboverlay_plugin.so")
                                  -> dlsym("overlay_api_version", ...)
                                  -> overlay_init(ANativeWindow*)
                                  -> overlay_input(AInputEvent*)
                                  -> overlay_render()
                                  -> overlay_shutdown()
```

`overlay_api.h` define a ABI. A biblioteca de visualizacao possui seu proprio contexto EGL, e **todas as chamadas** sao feitas no thread principal nativo da NativeActivity. Ao perder a janela, o host invoca `overlay_shutdown`; no retorno da janela, invoca `overlay_init` novamente.

## Diagnosticos

- `Dear ImGui ausente`: execute `scripts/setup-deps.ps1`; Git e internet sao necessarios.
- `dlopen falhou`: verifique se `liboverlay_plugin.so` consta no APK dentro de `lib/arm64-v8a/`.
- `EGL ES3 indisponivel`: use Android ARM64 com suporte a OpenGL ES 3.0.
- `Android NDK nao encontrado`: instale o NDK em SDK Manager.
- `SDK location not found`: abra o projeto no Android Studio ou configure `ANDROID_HOME`.
- Logcat para depuracao: `adb logcat -s OverlayHost OverlayLab`.

## Principais arquivos

- `app/src/main/cpp/host.cpp`: `dlopen`/`dlsym`, ciclo de vida e eventos.
- `app/src/main/cpp/plugin.cpp`: renderizacao e menu ImGui.
- `app/src/main/cpp/overlay_api.h`: funcoes C exportadas.
- `app/src/main/cpp/CMakeLists.txt`: targets `overlay_host` e `overlay_plugin`.

**Nota:** Este projeto e um exemplo de plugin dinamico **dentro do seu proprio aplicativo**. Nao existe suporte para anexar, injetar ou carregar essas bibliotecas em Free Fire ou em aplicativos de terceiros.

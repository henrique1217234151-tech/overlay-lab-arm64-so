# Compila as duas bibliotecas arm64-v8a usando NDK/CMake do Android Studio.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
function Fail([string]$message) { throw $message }

$sdkCandidates = @($env:ANDROID_HOME, $env:ANDROID_SDK_ROOT, "$env:LOCALAPPDATA\Android\Sdk") |
    Where-Object { $_ -and (Test-Path (Join-Path $_ 'ndk')) } | Select-Object -Unique
if (-not $sdkCandidates) { Fail 'Instale Android Studio, SDK, NDK (Side by side) e CMake em SDK Manager.' }
$sdk = @($sdkCandidates)[0]
$ndk = Get-ChildItem (Join-Path $sdk 'ndk') -Directory |
    Where-Object { Test-Path (Join-Path $_.FullName 'build\cmake\android.toolchain.cmake') } |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $ndk) { Fail 'Android NDK nao encontrado' }
$toolchain = Join-Path $ndk.FullName 'build\cmake\android.toolchain.cmake'

function FindTool([string]$name, [string]$subpath) {
    $found = Get-Command $name -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    $cmakeBase = Join-Path $sdk 'cmake'
    if (Test-Path $cmakeBase) {
        $matches = Get-ChildItem $cmakeBase -Directory | Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName $subpath } | Where-Object { Test-Path $_ }
        if ($matches) { return @($matches)[0] }
    }
    return $null
}
$cmake = FindTool 'cmake.exe' 'bin\cmake.exe'
$ninja = FindTool 'ninja.exe' 'bin\ninja.exe'
if (-not $cmake -or -not $ninja) { Fail 'CMake/Ninja nao encontrados. Instale CMake pelo SDK Manager.' }

$imgui = Join-Path $root 'third_party\imgui'
if (-not (Test-Path (Join-Path $imgui 'imgui.cpp'))) {
    Write-Host 'Baixando Dear ImGui v1.91.9...' -ForegroundColor Cyan
    & (Join-Path $root 'scripts\setup-deps.ps1')
    if ($LASTEXITCODE -ne 0) { Fail 'Erro no download do ImGui; instale Git e confira sua internet.' }
}
if (-not (Test-Path (Join-Path $imgui 'backends\imgui_impl_android.cpp'))) { Fail 'Dear ImGui incompleto' }

$source = Join-Path $root 'app\src\main\cpp'
$build = Join-Path $root 'build-arm64'
Write-Host 'Compilando para Android ARM64...' -ForegroundColor Cyan
& $cmake -S $source -B $build -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$toolchain" `
    '-DANDROID_ABI=arm64-v8a' '-DANDROID_PLATFORM=android-26' `
    '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_MAKE_PROGRAM=$ninja"
if ($LASTEXITCODE -ne 0) { Fail 'Erro de configuracao CMake' }
& $cmake --build $build --parallel 4
if ($LASTEXITCODE -ne 0) { Fail 'Erro de compilacao NDK' }
$dest = Join-Path $root 'saida'
New-Item -ItemType Directory -Force -Path $dest | Out-Null
foreach ($lib in @('liboverlay_plugin.so','liboverlay_host.so')) {
    $from = Join-Path $build $lib
    if (-not (Test-Path $from)) { Fail "Biblioteca ausente: $from" }
    Copy-Item $from (Join-Path $dest $lib) -Force
    Write-Host "Gerado: $dest\$lib" -ForegroundColor Green
}
Write-Host 'Estas bibliotecas sao usadas exclusivamente no app de demonstracao.'

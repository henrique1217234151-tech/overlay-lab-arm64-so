$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$dest = Join-Path $root 'third_party/imgui'
if (Test-Path (Join-Path $dest 'imgui.cpp')) {
    Write-Host 'Dear ImGui ja encontrado.'
    exit 0
}
New-Item -ItemType Directory -Force -Path (Join-Path $root 'third_party') | Out-Null
git clone --branch v1.91.9 --depth 1 https://github.com/ocornut/imgui.git $dest
if ($LASTEXITCODE -ne 0) { throw 'Falha ao clonar Dear ImGui' }

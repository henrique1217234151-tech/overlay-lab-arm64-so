#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p third_party
if [ ! -f third_party/imgui/imgui.cpp ]; then
  git clone --branch v1.91.9 --depth 1 https://github.com/ocornut/imgui.git third_party/imgui
fi

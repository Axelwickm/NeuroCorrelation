#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_TAG="neurocorrelation-web-build:latest"

docker build \
  -f "${ROOT_DIR}/web/docker/Dockerfile" \
  -t "${IMAGE_TAG}" \
  "${ROOT_DIR}" >/dev/null

docker run --rm \
  --user "$(id -u):$(id -g)" \
  -v "${ROOT_DIR}:/src" \
  -w /src \
  "${IMAGE_TAG}" \
  bash -lc '
    set -euo pipefail
    mkdir -p /tmp/build /tmp/vendor web/package/dist
    cp -r /usr/include/glm /tmp/vendor/
    emcc \
      -std=c99 \
      -O2 \
      -Itinyexpr \
      -c tinyexpr/tinyexpr.c \
      -o /tmp/build/tinyexpr.o
    em++ \
      -std=c++17 \
      -O2 \
      -Iimgui \
      -Itinyexpr \
      -I/tmp/vendor \
      src/main.cpp \
      src/NeuCor.cpp \
      src/NeuCor_Renderer.cpp \
      src/imgui_impl_glfw_gl3.cpp \
      imgui/imgui.cpp \
      imgui/imgui_draw.cpp \
      /tmp/build/tinyexpr.o \
      -o web/package/dist/neurocorrelation.mjs \
      --use-port=contrib.glfw3 \
      --preload-file resources@/resources \
      -sWASM=1 \
      -sALLOW_MEMORY_GROWTH=1 \
      -sNO_EXIT_RUNTIME=1 \
      -sASSERTIONS=1 \
      -sFORCE_FILESYSTEM=1 \
      -sNO_DISABLE_EXCEPTION_CATCHING=1 \
      -sEXPORT_ES6=1 \
      -sMODULARIZE=1 \
      -sEXPORT_NAME=createNeuroCorrelationModule \
      -sENVIRONMENT=web \
      -sMIN_WEBGL_VERSION=2 \
      -sMAX_WEBGL_VERSION=2 \
      -sEXPORTED_FUNCTIONS=_main,_neurocorrelation_request_shutdown
  '

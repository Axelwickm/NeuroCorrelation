FROM emscripten/emsdk:5.0.4

RUN apt-get update \
    && apt-get install -y --no-install-recommends libglm-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/web

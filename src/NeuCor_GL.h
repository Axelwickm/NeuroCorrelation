#ifndef NEUCOR_GL_H
#define NEUCOR_GL_H

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#ifndef GL_BLEND_SRC
#define GL_BLEND_SRC GL_BLEND_SRC_RGB
#endif
#ifndef GL_BLEND_DST
#define GL_BLEND_DST GL_BLEND_DST_RGB
#endif
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#endif // NEUCOR_GL_H

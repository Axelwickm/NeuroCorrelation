#ifndef NEUCOR_RENDERER_H
#define NEUCOR_RENDERER_H

/*  .h & .cpp includes  */
#include <NeuCor.h>

#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
using namespace glm;

class NeuCor_Renderer
{
    public:
        NeuCor_Renderer(NeuCor* _brain);
        ~NeuCor_Renderer();

        float getDeltaTime();

        void updateView();
        void pollWindow();

        bool realRunspeed; // Makes brain's runSpeed define simulation's speed by ms/s
        typedef void (*CallbackType)();
        void setDestructCallback(CallbackType f);
    protected:
    private:
        void updateCamPos();

        NeuCor* brain;

        GLuint billboard_vertex_buffer;
        GLuint neuron_position_buffer;
        GLuint neuron_potAct_buffer;
        GLuint synapse_PT_buffer;
        GLuint synapse_potential_buffer;

        GLuint neuronProgramID;
        GLuint synapseProgramID;
        GLuint ViewProjMatrixID[2];
        GLuint aspectID[2];

        glm::vec3 camPos;
        glm::vec3 camDir;
        glm::vec3 camUp;
        float camHA,camVA;
        double cursorX, cursorY;

        GLFWwindow* window;
        CallbackType destructCallback;

        int width, height;
        double lastTime;
        float deltaTime;
        void initGLFW();
        void initOpenGL(GLFWwindow* window);
        void loadResources();
};

#endif // NEUCOR_RENDERER_H

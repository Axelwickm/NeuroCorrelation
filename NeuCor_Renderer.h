#ifndef NEUCOR_RENDERER_H
#define NEUCOR_RENDERER_H

/*  .h & .cpp includes  */
#include <NeuCor.h>

#include <string>
#include <vector>

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
        bool realRunspeed; // Makes brain's runSpeed define simulation's speed by ms/s

        enum renderingModes { RENDER_VOLTAGE, RENDER_PLASTICITY, RENDER_ACTIVITY, RENDER_NOSYNAPSES, Count};
        std::vector<std::string> renderingModeNames = {"Voltage", "Plasticity", "Activity", "No synapses"};
        renderingModes renderMode;

        void updateView();
        void pollWindow();

        typedef void (*CallbackType)();
        void setDestructCallback(CallbackType f);

        enum callbackErrand {KEY_ACTION, CHAR, MOUSE_BUTTON, MOUSE_SCROLL, MOUSE_ENTER };
        template<typename ... callbackParameters>
        void inputCallback(callbackErrand errand, callbackParameters ... params);
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
        bool navigationMode;

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

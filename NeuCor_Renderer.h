#ifndef NEUCOR_RENDERER_H
#define NEUCOR_RENDERER_H

/*  .h & .cpp includes  */
#include <NeuCor.h>

#include <string>
#include <vector>
#include <deque>
#include <map>

#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"

#include <glm/glm.hpp>
using namespace glm;

class NeuCor_Renderer
{
    public:
        NeuCor_Renderer(NeuCor* _brain);
        ~NeuCor_Renderer();

        float getDeltaTime();
        bool runBrainOnUpdate; // If the render has the responsibility to run the brain.
        bool realRunspeed;// Makes brain's runSpeed define simulation's speed by ms/s
        bool paused;
        bool selectNeuron(int id, bool windowOpen);
        bool deselectNeuron(int id);

        enum renderingModes { RENDER_VOLTAGE, RENDER_PLASTICITY, RENDER_ACTIVITY, RENDER_NOSYNAPSES, Count};
        std::vector<std::string> renderingModeNames = {"Voltage", "Plasticity", "Activity", "No synapses"};
        renderingModes renderMode;


        void updateView();
        void pollWindow();

        typedef void (*CallbackType)();
        void setDestructCallback(CallbackType f);

        enum callbackErrand {KEY_ACTION, CHAR_ACTION, MOUSE_BUTTON, MOUSE_SCROLL, MOUSE_ENTER };
        template<typename ... callbackParameters>
        void inputCallback(callbackErrand errand, callbackParameters ... params);
    protected:
    private:
        NeuCor* brain;

        void renderInterface();
        enum graphicsModule {MODULE_BRAIN, MODULE_TIME, MODULE_SELECTED_NEURONS, MODULE_STATS, MODULE_CONTROLS, MODULE_count};
        bool moduleInitOpen[5] = {true, true, true, false, false};
        struct module {
            graphicsModule type;
            ImVec2 pos;
            bool windowed;
            bool beingDragged;
            bool snapped;
        };
        std::vector<module> modules;

        void renderModule(module* mod, bool windowed);
        void renderNeuronWindow(int ID, bool* open);
        std::pair<int, ImVec2> newNeuWinPos; // <ID,pos> If ID = NULL it isn't used
        void updateCamPos();
        void resetCursor();
        std::vector<int> selectedNeurons; // Selected neuron ID, smart pointer to bool if neuon window is open
        std::map<int, bool> selectedNeuronsWindows; // Selected neuron ID, smart pointer to bool if neuon window is open

        struct realTimeStats {
            realTimeStats();
            float activityUpdateTimer, weightUpdateTimer;

            unsigned neuronCount, synapseCount;

            struct neuronSnapshot { // For plotting
                int id;
                float time;
                float voltage;
            };
            std::map<int, std::deque<neuronSnapshot> > timeline;
            float maxTimeline;
        };
        realTimeStats logger;

        GLuint billboard_vertex_buffer;
        GLuint neuron_position_buffer;
        GLuint neuron_potAct_buffer;
        GLuint synapse_PT_buffer;
        GLuint synapse_potential_buffer;

        GLuint neuronTexID;
        GLuint neuron_smallTexID;
        GLuint neuronProgramID;
        GLuint synapseProgramID;
        GLuint ViewProjMatrixID[2];
        GLuint aspectID[2];

        glm::vec3 camPos;
        glm::vec3 camDir;
        glm::vec3 camUp;
        float camHA,camVA;
        glm::mat4 vp;
        double cursorX, cursorY;
        bool navigationMode, mouseInWindow, showInterface;

        GLFWwindow* window;
        CallbackType destructCallback;

        int width, height;
        double lastTime;
        float deltaTime;
        float FPS;
        void initGLFW();
        void initOpenGL(GLFWwindow* window);
        void loadResources();
        inline glm::vec3 screenCoordinates(glm::vec3 worldPos, bool nomalizedZ = false);
};

#endif // NEUCOR_RENDERER_H

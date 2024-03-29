#include "NeuCor_Renderer.h"

/*  .h & .cpp includes  */
#include "NeuCor.h"
#include "tinyexpr.h"

#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
using namespace glm;

/*  .cpp includes  */

#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <memory>
#include <stdio.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "picopng/picopng.cpp"
#include "imgui_impl_glfw_gl3.h"
#include "imgui_internal.h"

std::map<GLFWwindow*, NeuCor_Renderer*> windowRegistry;

/* glfw error function helper */
void glfw_ErrorCallback(int error, const char* description){
    fputs(description, stderr);
}
/* glfw key callback function helper */
static void glfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::KEY_ACTION, window, key, scancode, action, mods);
    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
}
void glfw_CharCallback(GLFWwindow* window, unsigned int c){
     windowRegistry.at(window)->inputCallback(NeuCor_Renderer::CHAR_ACTION, window, (int) c, -1, -1, -1);
     ImGui_ImplGlfwGL3_CharCallback(window, c);
}
void glfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::MOUSE_BUTTON, window, button, action, mods, -1);
    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
}
void glfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::MOUSE_SCROLL, window, (int) xoffset, (int) yoffset, -1, -1);
    ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
}
void glfw_CursorCallback(GLFWwindow* window, int entered){
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::MOUSE_ENTER, window, entered, -1, -1, -1);
}
void glfw_FocusCallback(GLFWwindow*window, int focused){
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::MOUSE_ENTER, window, focused, -1, -1, -1);
}

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}


NeuCor_Renderer::NeuCor_Renderer(NeuCor* _brain)
:camPos(5,5,5), camDir(0,0,0), camUp(0,1,0), camHA(0.75), camVA(3.8), lastTime(0), deltaTime(1)
{
    brain = _brain;
    runBrainOnUpdate = true;
    paused = false;
    realRunspeed = false;

    navigationMode = false;
    mouseInWindow = true;
    showInterface = true;

    hoveredInput = -1;

    FPS = 0;

    realTimeStats logger();

    /* Initiates GLFW, OpenGL & ImGui*/
    initGLFW();
    initOpenGL(window);
    ImGui_ImplGlfwGL3_Init(window, false);

    /* Place all graphics modules in a vector (order defines position) */
    for (int i = 0; i<MODULE_count; i++){
        modules.emplace_back();
        modules.back().type = (graphicsModule) i;
        modules.back().windowed = false;
        modules.back().snapped = false;
        modules.back().beingDragged = false;;
    }
    activityExpression = (char*) calloc(256, 1);
    activityExpression[0] = 'a';
    closenessIntensity = 0.8;

    /* Load resources */
    loadResources();
    glfwSetCursorPos(window, width/2.0, height/2.0);
    updateCamPos();
    destructCallback = NULL;

    /* Init matrices */
    camDir = glm::vec3 (
        cos(camVA) * sin(camHA),
        sin(camVA),
        cos(camVA) * cos(camHA)
    );
    glm::vec3 right = glm::vec3(
        -cos(camHA),
        0,
        sin(camHA)
    );
    camUp = glm::cross( right, camDir );

    float aspect = (float) width / (float)height;

    glm::mat4 Projection = glm::perspective(glm::radians(65.0f), (float) aspect, 0.05f, 100.0f);
    glm::mat4 View = glm::lookAt(
        camPos,
        camPos+camDir,
        camUp
    );
    vp = Projection * View;

    cameraRadius = glm::distance(camPos, vec3(0,0,0)); // For orbit camera mode
}

NeuCor_Renderer::realTimeStats::realTimeStats(){
    maxTimeline = 15.0; // ms

    activityUpdateTimer = 0;
    weightUpdateTimer = 0.1; // Offset as to not use CPU at the same time
}

NeuCor_Renderer::~NeuCor_Renderer() {
    /* Destroy window and terminate glfw */
    ImGui_ImplGlfwGL3_Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);

    /* Call the destruct callback function if it has been set */
    if (destructCallback != NULL) destructCallback();
}


void NeuCor_Renderer::initGLFW(){
    /* Init glew */
    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwSetErrorCallback(glfw_ErrorCallback);

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3

    cameraMode = cameraModes::CAMERA_ORBIT;
    renderMode = renderingModes::RENDER_VOLTAGE;


    /* Create window */
    window = glfwCreateWindow(1000, 1000, "Neural Correlation", NULL, NULL);
    if (!window) {
        glfwTerminate();

        exit(EXIT_FAILURE);
    }
    windowRegistry[window] = this; // Adds window to global registry

    glfwGetWindowSize(window, &width, &height);
    glfwMakeContextCurrent(window);
    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
    }
    /* Prep for OpenGl */
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, glfw_KeyCallback);
    glfwSetCharCallback(window, glfw_CharCallback);
    glfwSetMouseButtonCallback(window, glfw_MouseButtonCallback);
    glfwSetScrollCallback(window, glfw_ScrollCallback);
    glfwSetWindowFocusCallback(window, glfw_FocusCallback);

    glfwSetCursorEnterCallback(window, glfw_CursorCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwPollEvents();
    glfwSetCursorPos(window, width/2, height /2);
    cursorX = width/2.0; cursorY = height/2.0;
}
void NeuCor_Renderer::initOpenGL(GLFWwindow* window){
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(GLclampf(0.01), GLclampf(0.01), GLclampf(0.02), GLclampf(1.0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLineWidth(2.5);

    synapseProgramID = LoadShaders( "resources/synapse.shader", "resources/synapse.Fshader" );
    neuronProgramID = LoadShaders( "resources/neuron.shader", "resources/neuron.Fshader" );

    glUseProgram(neuronProgramID);
	ViewProjMatrixID[0] = glGetUniformLocation(neuronProgramID, "VP");
	aspectID[0] = glGetUniformLocation(neuronProgramID, "aspect");

	glUseProgram(synapseProgramID);
	ViewProjMatrixID[1] = glGetUniformLocation(neuronProgramID, "VP");
	aspectID[1] = glGetUniformLocation(neuronProgramID, "aspect");

    static const GLfloat g_vertex_buffer_data[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
    };

    glGenBuffers(1, &billboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);


    glGenBuffers(1, &neuron_position_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, neuron_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, brain->positions.size()* 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    glGenBuffers(1, &neuron_potAct_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, neuron_potAct_buffer);
    glBufferData(GL_ARRAY_BUFFER, brain->positions.size()* 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);


    glGenBuffers(1, &synapse_PT_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, synapse_PT_buffer);
    glBufferData(GL_ARRAY_BUFFER, brain->neurons.size() * 5 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    glGenBuffers(1, &synapse_potential_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, synapse_potential_buffer);
    glBufferData(GL_ARRAY_BUFFER, brain->neurons.size() * 5 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

}

void NeuCor_Renderer::loadResources() {
    const char* filename = "resources/neuron.png";

    //load and decode
    std::vector<unsigned char> buffer, image;
    loadFile(buffer, filename);
    unsigned long w, h;

    int error = decodePNG(image, w, h, &buffer[0], (unsigned long)buffer.size());
    if(error != 0) std::cout << "error: " << error << '\n';


    unsigned char * neuronimgData = &image[0];


    // Create one OpenGL texture
    glGenTextures(1, &neuronTexID);


    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, neuronTexID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, neuronimgData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    filename = "resources/neuron_small.png";

    //load and decode
    buffer, image;
    loadFile(buffer, filename);

    error = decodePNG(image, w, h, &buffer[0], (unsigned long)buffer.size());
    if(error != 0) std::cout << "error: " << error << '\n';


    unsigned char * neuron_smallimgData = &image[0];


    // Create one OpenGL texture
    glGenTextures(1, &neuron_smallTexID);


    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, neuron_smallTexID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, neuron_smallimgData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

}

bool NeuCor_Renderer::selectNeuron(int id, bool windowOpen){
    assert(id < brain->neurons.size());
    if (neuronWindows.find(id) == neuronWindows.end()){
        selectedNeurons.push_back(id);
        logger.timeline.insert(std::pair<int, std::deque<realTimeStats::neuronSnapshot> >(id, std::deque<realTimeStats::neuronSnapshot>()));
        neuronWindow newWindow = {
            .open = windowOpen,
            .usingRelative = true, .relativePosition = ImVec2(15, 15),
            .nextPosition = ImVec2(-1,-1), .nextCollapsed = 0,
            .beingDragged = false
        };

        neuronWindows.insert(std::pair<int, neuronWindow> (id, newWindow));
        if (renderMode == RENDER_CLOSENESS) updateSignalSpread();

        return true;
    }
    else {
        neuronWindows.at(id).open = windowOpen;
        return false;
    }
}

bool NeuCor_Renderer::deselectNeuron(int id){
    assert(id < brain->neurons.size());
    if (neuronWindows.find(id) != neuronWindows.end()){
        for (std::vector<int>::iterator i = selectedNeurons.begin(); i < selectedNeurons.end(); i++){
            if (*i == id){
                selectedNeurons.erase(i);
                break;
            }
        }
        neuronWindows.erase(id);
        logger.timeline.erase(id);
        if (renderMode == RENDER_CLOSENESS) updateSignalSpread();
        return true;
    }
    else {
        return false;
    }
}

void NeuCor_Renderer::updateView(){
    #define PRINT_CONNECTIONS_EVERY_FRAME false

    double currentTime = glfwGetTime();
    deltaTime = float(currentTime - lastTime);
    lastTime = currentTime;
    FPS = (FPS*20.0+1.0/deltaTime)/21.0; // Makes FPS change slower
    float aspect = (float) width / (float)height;

    if (runBrainOnUpdate && realRunspeed && !paused){
        float staticRunSpeed = brain->runSpeed;
        brain->runSpeed = staticRunSpeed*deltaTime;
        brain->run();
        brain->runSpeed = staticRunSpeed;
    }
    else if (runBrainOnUpdate && !paused) brain->run();


    updateCamPos();

    if (renderMode == RENDER_ACTIVITY && evaluated != NULL) activityFunction(-1, true);
    else if (renderMode == RENDER_NOSYNAPSES) logger.synapseCount = 0;

    closenessValues.resize(brain->neurons.size(), 0);
    std::vector<coord3> connections;
    std::vector<float> synPot;
    synPot.reserve(brain->neurons.size()*8.0);
    for (auto &neu : brain->neurons){
        for (auto &syn : neu.outSynapses){
            connections.push_back(brain->getNeuron(syn.pN)->position());
            if (connections.back().x != connections.back().x){ // For debugging
                std::cout<<"NaN coord!\n";
            }
            connections.push_back(brain->getNeuron(syn.tN)->position());
            if (connections.back().x != connections.back().x){ // For debugging
                std::cout<<"NaN coord!\n";
            }
            if (PRINT_CONNECTIONS_EVERY_FRAME) std::cout<<syn.pN<<" "<<connections.at(connections.size()-2).x<<" -> "<<syn.tN<<" "<<connections.back().x<<" | ";


            if (renderMode == RENDER_VOLTAGE){
                synPot.push_back(syn.getPrePot()+0.03);
                synPot.push_back(syn.getPostPot()+0.03);
            }
            else if (renderMode == RENDER_PLASTICITY){
                synPot.push_back(syn.getWeight()/2.0);
                synPot.push_back(syn.getWeight()/2.0);
                if (RENDER_PLASTICITY_onlyActive){
                    synPot.at(synPot.size()-2) *= log(brain->getNeuron(syn.pN)->activity()+1.f);
                    synPot.back()              *= log(brain->getNeuron(syn.tN)->activity()+1.f);
                }

            }
            else if (renderMode == RENDER_ACTIVITY && evaluated == NULL){
                synPot.push_back(log(brain->getNeuron(syn.pN)->activity()+1.f));
                synPot.push_back(log(brain->getNeuron(syn.tN)->activity()+1.f));
            }
            else if (renderMode == RENDER_ACTIVITY){
                synPot.push_back(log(activityFunction(syn.pN)+1.f));
                synPot.push_back(log(activityFunction(syn.tN)+1.f));
            }
            else if (renderMode == RENDER_CLOSENESS){
                synPot.push_back(powf(closenessValues.at(syn.pN), closenessIntensity));
                synPot.push_back(powf(closenessValues.at(syn.tN), closenessIntensity));
            }
            else if (renderMode == RENDER_NOSYNAPSES) logger.synapseCount++;
        }
    }
    if (PRINT_CONNECTIONS_EVERY_FRAME) std::cout<<'\n';

    logger.neuronCount = brain->neurons.size();
    if (renderMode != RENDER_NOSYNAPSES) logger.synapseCount = synPot.size()/2;

    if (renderMode == RENDER_NOSYNAPSES) goto renderNeurons; // Skip rendering synapses

    // Render synapses
    renderSynapses:

    glUseProgram(synapseProgramID);


    glBindBuffer(GL_ARRAY_BUFFER, synapse_PT_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, connections.size() * sizeof(coord3), NULL);
    glBufferData(GL_ARRAY_BUFFER, connections.size() * sizeof(coord3), &connections[0], GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, synapse_potential_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, synPot.size() * sizeof(GLfloat), NULL);
    glBufferData(GL_ARRAY_BUFFER, synPot.size() * sizeof(GLfloat), &synPot[0], GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, synapse_PT_buffer);
    glVertexAttribPointer(
     0, // attribute.
     3, // size
     GL_FLOAT, // type
     GL_FALSE, // normalized?
     0, // stride
     (void*)0 // array buffer offset
    );


    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1,0);
    glBindBuffer(GL_ARRAY_BUFFER, synapse_potential_buffer);
    glVertexAttribPointer(
     1, // attribute.
     1, // size
     GL_FLOAT, // type
     GL_FALSE, // normalized?
     0, // stride
     (void*)0 // array buffer offset
    );

    glUniform1f(aspectID[1], aspect);
    glUniformMatrix4fv(ViewProjMatrixID[1], 1, GL_FALSE, &vp[0][0]);


    //glDrawElements(GL_LINES, connections.size()/2, GL_UNSIGNED_INT, (void*)0); <-- Crashes
    glDrawArrays(GL_LINES, 0, connections.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);


    // Render neurons
    renderNeurons:

    glUseProgram(neuronProgramID);


    glUniform1f(aspectID[0], aspect);
    glUniformMatrix4fv(ViewProjMatrixID[0], 1, GL_FALSE, &vp[0][0]);


    unsigned neuronC = brain->positions.size();

    glBindBuffer(GL_ARRAY_BUFFER, neuron_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, neuronC * 3 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW); // Buffer orphaning
    glBufferSubData(GL_ARRAY_BUFFER, 0, neuronC * 3 * sizeof(GLfloat), &brain->positions[0]);

    glBindBuffer(GL_ARRAY_BUFFER, neuron_potAct_buffer);
    glBufferData(GL_ARRAY_BUFFER, neuronC * 2 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW); // Buffer orphaning
    glBufferSubData(GL_ARRAY_BUFFER, 0, neuronC * 2 * sizeof(GLfloat), &brain->potAct[0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glVertexAttribPointer(
     0, // attribute
     3, // size
     GL_FLOAT, // type
     GL_FALSE, // normalized?
     0, // stride
     (void*)0 // array buffer offset
    );

    // 2nd attribute buffer : positions of particles' centers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, neuron_position_buffer);
    glVertexAttribPointer(
     1, // attribute
     3, // size : x + y + z => 3
     GL_FLOAT, // type
     GL_FALSE, // normalized?
     0, // stride
     (void*)0 // array buffer offset
    );

    // 3rd attribute buffer
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, neuron_potAct_buffer);
    glVertexAttribPointer(
     2, // attribute
     2, // values
     GL_FLOAT, // type
     GL_FALSE, // normalized?
     0, // stride
     (void*)0 // array buffer offset
    );


    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, neuronC);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    // Hide cursor if the cursor is in the window, and navigation mode is on, else show it.
    if (navigationMode && mouseInWindow) {glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); ImGui::CaptureMouseFromApp(false);}
    else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);



    if (selectedNeurons.size() != 0 && !paused){
        float brainTime = brain->getTime();
        for (auto neuID: selectedNeurons) {
            Neuron* neu = brain->getNeuron(neuID);
            std::deque<realTimeStats::neuronSnapshot>* neuTimeline;
            neuTimeline = &logger.timeline.at(neuID);

            neuTimeline->emplace_back(realTimeStats::neuronSnapshot());
            auto neuSnap = &neuTimeline->back();

            neuSnap->id = neuID;
            neuSnap->time = brainTime;
            neuSnap->voltage = neu->potential();
            neuSnap->synapseWeights.reserve(neu->outSynapses.size());
            for (auto &syn: neu->outSynapses){
                neuSnap->synapseWeights.push_back(syn.getWeight());
            }

            while (logger.maxTimeline < brainTime - neuTimeline->begin()->time)
                neuTimeline->pop_front();
        }
    }
    if (showInterface) renderInterface();

    glfwSwapBuffers(window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void NeuCor_Renderer::pollWindow(){
    glfwPollEvents();

    int temp_width, temp_height;
    glfwGetWindowSize(window, &temp_width, &temp_height);
    if (temp_width != width || temp_height != height){
        width = temp_width;
        height = temp_height;
        glViewport(0, 0, width, height);
    }
    if (glfwWindowShouldClose(window)){
        delete this;
    }
}
void NeuCor_Renderer::setDestructCallback(CallbackType callbackF){
    destructCallback = callbackF;
}

void NeuCor_Renderer::updateCamPos(){
    // Slow down time
    if (glfwGetKey(window, GLFW_KEY_PERIOD ) == GLFW_PRESS){
        brain->runSpeed = powf(brain->runSpeed, 0.99);
        //std::cout<<"Run-speed: "<<brain->runSpeed<<'\n';
    }
    // Speed up time
    if (glfwGetKey(window, GLFW_KEY_COMMA ) == GLFW_PRESS){
        brain->runSpeed = powf(brain->runSpeed, 1.01);
        //std::cout<<"Run-speed: "<<brain->runSpeed<<'\n';
    }
    if ((!navigationMode && !(cameraMode == CAMERA_ORBIT_MOMENTUM)) || !mouseInWindow) return;

    float aspect = (float) width / (float)height;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (cursorX != cursorX) {cursorX = xpos; cursorY = ypos;}

    if (cameraMode == CAMERA_MOUSE_LOOK){
        float deltaX = cursorX-xpos;
        float deltaY = cursorY-ypos;

        camHA -= 0.15 * deltaTime * deltaX;
        camVA  += 0.15 * deltaTime * deltaY;

        camDir = glm::vec3 (
            cos(camVA) * sin(camHA),
            sin(camVA),
            cos(camVA) * cos(camHA)
        );
        glm::vec3 right = glm::vec3(
            -cos(camHA),
            0,
            sin(camHA)
        );
        camUp = glm::cross( right, camDir );

        float speedMult = 5;
        // Move forward
        if (glfwGetKey(window, GLFW_KEY_W ) == GLFW_PRESS){
            camPos += camDir * GLfloat(deltaTime * speedMult);
        }
        // Move backward
        if (glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS){
            camPos -= camDir * GLfloat(deltaTime * speedMult);
        }
        // Strafe right
        if (glfwGetKey(window, GLFW_KEY_D ) == GLFW_PRESS){
            camPos += right * GLfloat(deltaTime *  speedMult);
        }
        // Strafe left
        if (glfwGetKey(window, GLFW_KEY_A ) == GLFW_PRESS){
            camPos -= right * GLfloat(deltaTime * speedMult);
        }

        glm::mat4 Projection = glm::perspective(glm::radians(65.0f), (float) aspect, 0.05f, 100.0f);
        glm::mat4 View = glm::lookAt(
            camPos,
            camPos+camDir,
            camUp
        );
        vp = Projection * View;

        cameraRadius = glm::distance(camPos, vec3(0,0,0)); // For orbit camera mode
    }
    else if (cameraMode == CAMERA_ORBIT || cameraMode == CAMERA_ORBIT_MOMENTUM) {
        float deltaX = cursorX-xpos;
        float deltaY = cursorY-ypos;
        static glm::vec2 momentum(0.0, 0.0);

        glm::vec2 cameraPan(0.0, 0.0);
        if (ImGui::IsMouseDragging(0, 0) && navigationMode){
            cameraPan = glm::vec2(-deltaX, deltaY)/50.f;
        }
        else {
            if (cameraMode == CAMERA_ORBIT_MOMENTUM){
                if (navigationMode) momentum += glm::vec2(deltaX, -deltaY) * deltaTime / 100.f;
                camHA += momentum.x*deltaTime;
                camVA += momentum.y*deltaTime;
            }
            else {
                camHA -= 0.15 * deltaTime * deltaX;
                camVA  += 0.15 * deltaTime * deltaY;
                momentum = glm::vec2(0.0, 0.0);
            }
        }
        static glm::vec3 focusPoint(0.0, 0.0, 0.0);

        camPos = focusPoint + glm::vec3(cos(camHA) * cos(camVA), sin(camVA), -sin(camHA) * cos(camVA)) * cameraRadius;
        camUp =  glm::normalize(
            glm::vec3(cos(camHA) * cos(camVA + 1.570796f),
            sin(camVA + 1.570796f),
            -sin(camHA) * cos(camVA + 1.570796f))
        );
        glm::mat4 Projection = glm::perspective(glm::radians(65.0f), (float) aspect, 0.05f, 100.0f);
        glm::mat4 View = glm::lookAt(
            camPos,
            focusPoint,
            camUp
        );
        focusPoint += glm::vec3( glm::vec4(cameraPan, 0.0, 0.0) * View );

        vp = Projection * View;
    }
    cursorX = xpos; cursorY = ypos;
};

void NeuCor_Renderer::updateSignalSpread(){
    std::fill(closenessValues.begin(), closenessValues.end(), INFINITY);
    std::vector<std::size_t> toCheck;
    toCheck.reserve(logger.synapseCount/5);
    for (auto neuID: selectedNeurons){
        closenessValues.at(neuID) = 0.0;
        toCheck.push_back(neuID);
    }
    float maxDegree = 0;
    while (!toCheck.empty()){
        int current = toCheck.back();
        toCheck.pop_back();
        for (auto &outSyn: brain->getNeuron(current)->outSynapses){
            if (0.0 > outSyn.getWeight()) continue;
            float newDegree = closenessValues.at(current) + 1.0/outSyn.getWeight();
            if (newDegree < closenessValues.at(outSyn.tN)){
                toCheck.push_back(outSyn.tN);
                closenessValues.at(outSyn.tN) = newDegree;
            }
        }
    }
    for (auto &neu: closenessValues)
        if (maxDegree < neu)
            maxDegree = neu;
    maxDegree = 6;
    for (auto &neu: closenessValues)
        neu = fmax(0, 1.0 - neu/maxDegree);
}

inline float NeuCor_Renderer::activityFunction(int ID, bool update){
    static std::vector<double*> variableLinks;
    static std::vector<std::vector<float>*> activityLinks;
    static int current = 0; // Has to be updated from Neuron, not stored data
    if (update) {
        variableLinks.resize(variables.size());
        activityLinks.resize(variables.size());
        int i = 0;
        for (auto it = variables.begin(); it != variables.end(); it++){
            if (it->first == currentActivity) current = i;
            variableLinks[i] = it->second.first.get();
            activityLinks[i] = &(it->second.second); // Will be empty if current
            i++;
        }
        return 0;
    }
    else {
        for (int i = 0; i<variableLinks.size(); i++){
            if (i != current) *(variableLinks.at(i)) = activityLinks.at(i)->at(ID);
            else *(variableLinks.at(i)) = brain->getNeuron(ID)->activity(); // Live update
        }
        return te_eval(evaluated);
    }
}

inline glm::vec3 NeuCor_Renderer::screenCoordinates(glm::vec3 worldPos, bool nomalizedZ){
    glm::vec4 posClip = vp * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f );
    glm::vec3 posNDC = glm::vec3(posClip) / posClip.w;
    if (1.0 < posNDC.z) return glm::vec3(NAN, NAN, NAN); // Don't give coordinates if behind camera
    if (nomalizedZ){
        GLfloat dR[2]; // Depth range
        glGetFloatv(GL_DEPTH_RANGE, &dR[0]);
        return glm::vec3(
            posNDC.x*width*0.5+width*0.5,
            -posNDC.y*height*0.5+height*0.5,
            (dR[1]-dR[0])/2.0*posNDC.z + (dR[1]+dR[0])/2.0);
    }
    else {
        return glm::vec3(
            posNDC.x*width*0.5+width*0.5,
            -posNDC.y*height*0.5+height*0.5,
            posNDC.z);
    }
}
inline void NeuCor_Renderer::renderLine(int ID){
    // Render line from window to neuron
    coord3 pos3D = brain->getNeuron(ID)->position();
    glm::vec3 screenGLM = screenCoordinates(glm::vec3(pos3D.x, pos3D.y, pos3D.z));
    ImVec2 screen(screenGLM.x, screenGLM.y);

    // Get closest point from window
    ImVec2 minP = ImGui::GetWindowPos();
    ImVec2 maxP(minP.x+ImGui::GetWindowSize().x, minP.y+ImGui::GetWindowSize().y);

    ImVec2 closest = screen;
    if (closest.x > maxP.x) closest.x = maxP.x;
    else if (closest.x < minP.x) closest.x = minP.x;
    if (closest.y > maxP.y) closest.y = maxP.y;
    else if (closest.y < minP.y) closest.y = minP.y;

    // Draw line. This is done inside window so that other windows can focus over it.
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRectFullScreen();
    draw_list->AddLine(closest, screen, ImColor(0, 90, 173, 180), 2.2f);
    draw_list->PopClipRect();
}

void NeuCor_Renderer::renderInterface(){
    ImGui_ImplGlfwGL3_NewFrame();

    for (int i = 0; i<modules.size(); i++){ // Render modules as windows
        if (modules[i].windowed) renderModule(&modules[i], true);
    }


    {   // Dock
        static bool openDock = true;
        static int lastWidth;
        if (!openDock &&  10 < lastWidth)
            ImGui::SetNextWindowSize(ImVec2(lastWidth-20, 0));
        if (openDock &&  lastWidth < std::min( int(width/2.5), 300))
            ImGui::SetNextWindowSize(ImVec2(lastWidth+20, 0));


        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSizeConstraints(ImVec2(10, height), ImVec2(width/2.5, height));
        ImGui::Begin("", NULL, window_flags);
        lastWidth = ImGui::GetWindowWidth();

        if (40 < ImGui::GetWindowWidth()){
            for (int i = 0; i<modules.size(); i++){ // Render modules in dock
                if (!modules[i].windowed || modules[i].snapped) renderModule(&modules[i], false);
            }
        }

        ImGui::Dummy(ImVec2(0, ImGui::GetContentRegionAvail().y-20));
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x-25, 0));
        ImGui::SameLine();
        ImGui::RadioButton("", openDock);
        if (ImGui::IsItemClicked()){
            openDock = !openDock;
        }
        dockHovered = ImGui::IsWindowHovered();
        ImGui::End();

    }

    // Render inputs firers
    ImGui::SetNextWindowPos( ImVec2(0,0) ); // Naked background window
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("BCKGND", NULL, ImGui::GetIO().DisplaySize, 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus );
    hoveredInput = -1;
    int inputHandlerID = 0;
    for (auto &inFi: brain->inputHandler){
        ImDrawList* draw_list = ImGui::GetWindowDrawList();


        ImColor color;
        if (inFi.enabled) color = ImColor(0.8 - 10.0f/((float) ((float) brain->getTime()-inFi.lastFire)*10.0f+1.0f), 0.5f, 0.5f);
        else color = ImColor(0.1f, 0.1f, 0.1f);

        glm::vec3 screenCoords = screenCoordinates(glm::vec3(inFi.a.x, inFi.a.y, inFi.a.z));
        draw_list->AddCircle(ImVec2(screenCoords.x, screenCoords.y), 10, color);

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (glm::distance(glm::vec2(screenCoords), glm::vec2(xpos, ypos)) < 10){
            ImGui::BeginTooltip();
            ImGui::Text("Input %i: %.0f Hz", inputHandlerID, brain->inputArray[inputHandlerID]);
            ImGui::EndTooltip();
            hoveredInput = inputHandlerID;
        }
        inputHandlerID++;
    }

    // Render voltage detectors
    int voltageDetectorID = 0;
    for (auto &VD: brain->voltageDetectors){
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        float voltage = brain->getDetectorVoltage(voltageDetectorID);

        glm::vec3 screenCoords = screenCoordinates(glm::vec3(VD.a.x, VD.a.y, VD.a.z));
        draw_list->AddCircle(ImVec2(screenCoords.x, screenCoords.y), 5, ImColor(0.9f*(voltage+70.f)/60.f, 0.2f, 0.75f));

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (glm::distance(glm::vec2(screenCoords), glm::vec2(xpos, ypos)) < 10){
            ImGui::BeginTooltip();
            ImGui::Text("Voltage detector %i: %.0f mV", voltageDetectorID, voltage);
            ImGui::EndTooltip();
        }
        voltageDetectorID++;
    }
    ImGui::End();

    // Render neuron windows which are open
    for (auto ID: selectedNeurons){
        if (neuronWindows.at(ID).open) renderNeuronWindow(ID);
    }

    ImGui::Render();
}

 float NeuCor_Renderer::getDeltaTime(){
    return deltaTime;
}

void NeuCor_Renderer::renderNeuronWindow(int ID, neuronWindow* neuWin){
    #define windowInitX 320
    #define windowInitY 450
    Neuron* neu = brain->getNeuron(ID);
    if (!neuWin) neuWin = &neuronWindows.at(ID);
    float neuPot = neu->potential();

    coord3 pos3D = neu->position();
    glm::vec3 screenGLM = screenCoordinates(glm::vec3(pos3D.x, pos3D.y, pos3D.z));
    if (neuWin->nextPosition.x != -1 || neuWin->nextPosition.y != -1) {
        ImGui::SetNextWindowPos(neuWin->nextPosition);
        neuWin->usingRelative = false;
    }
    else if (neuWin->usingRelative && !neuWin->beingDragged) ImGui::SetNextWindowPos(ImVec2(screenGLM.x+neuWin->relativePosition.x, screenGLM.y+neuWin->relativePosition.y));
    else if (neuWin->beingDragged) neuWin->relativePosition = ImVec2(neuWin->currentWindowPos.x - screenGLM.x, neuWin->currentWindowPos.y - screenGLM.y);

    if (neuWin->nextCollapsed != 0) ImGui::SetNextWindowCollapsed(neuWin->nextCollapsed == 1);
    else ImGui::SetNextWindowCollapsed(true, ImGuiSetCond_Appearing);

    neuWin->nextPosition = ImVec2(-1, -1);
    neuWin->nextCollapsed = 0;


    char buffer[100];
    std::sprintf(buffer, "Neuron %i", ID);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImColor(116, 102, 116, (int) floor(50 + neuPot*180.0f)));
    bool collapsed = !ImGui::Begin(buffer, &neuWin->open, ImGuiWindowFlags_NoResize);

    neuWin->currentWindowPos = ImGui::GetWindowPos();
    if (ImGui::IsMouseHoveringWindow() && ImGui::IsMouseDragging()){
        neuWin->beingDragged = true;
    }
    else neuWin->beingDragged = false;

    if (collapsed){
        ImGui::SetWindowSize(buffer, ImVec2(
            fmax(ImGui::GetWindowWidth()-20, 120), 1));
        renderLine(ID);
        ImGui::End(); ImGui::PopStyleColor(1);
        return;
    }
    else {
        ImGui::SetWindowSize(buffer, ImVec2(
            fmin(ImGui::GetWindowWidth()+20, windowInitX),
            fmin(ImGui::GetWindowHeight()+40, windowInitY)));
    }


    ImGui::Text("Current voltage: %.01f mV", neuPot);
    ImGui::SameLine(); ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvailWidth() - 30, 0)); ImGui::SameLine();
    ImGui::RadioButton("", neuWin->usingRelative);
    if (ImGui::IsItemClicked()){
        neuWin->usingRelative = !neuWin->usingRelative;
        neuWin->relativePosition = ImVec2(neuWin->currentWindowPos.x - screenGLM.x, neuWin->currentWindowPos.y - screenGLM.y);
    }
    if (ImGui::IsItemHovered()) {ImGui::BeginTooltip(); ImGui::Text("Follow neuron"); ImGui::EndTooltip();}
    ImGui::Text("Firing frequency: %.1f Hz", neu->activity());
    ImGui::Text("Last fire: %1.f ms ago", brain->getTime() - neu->lastFire);

    ImGui::Separator();
    ImGui::Text("Voltage graph");

    std::deque<realTimeStats::neuronSnapshot>* neuTimeline = &logger.timeline.at(ID);
    float voltageData[neuTimeline->size()];
    for (int i = 0; i < neuTimeline->size(); i++){
        voltageData[i] = neuTimeline->at(i).voltage;
    }
    #define voltageGraphWidth 300
    #define voltageGraphHeight 100

    ImGui::PlotLines("", voltageData, neuTimeline->size(), 0, "", -90.0f, 50.0f, ImVec2(300, 100));
    ImVec2 thresholdLine( ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y + voltageGraphHeight * 0.75 );
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddLine(thresholdLine, ImVec2(thresholdLine.x + voltageGraphWidth, thresholdLine.y), ImColor(50, 50, 50));
    ImGui::Separator();

    float windowWidth = ImGui::GetWindowContentRegionWidth();

    ImGui::BeginChild("In synapses", ImVec2(windowWidth * 0.38f, 0), ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Text("In synapses");
    for (auto &synM: neu->inSynapses){
        Synapse* syn = brain->getSynapse(synM);

        ImGui::PushStyleColor(ImGuiCol_Header, ImColor(116, 102, 116, (int) floor(50 + syn->getPrePot()*180.0f)));
        std::sprintf(buffer,"%i", syn->pN);
        if (ImGui::CollapsingHeader(buffer)){
            if (ImGui::Button("Open")) {
                selectNeuron(syn->pN, true);
                ImVec2 currentWindowPos = ImGui::GetWindowPos();
                neuronWindows.at(syn->pN).nextPosition = ImVec2(currentWindowPos.x-windowInitX-10, currentWindowPos.y-windowInitY/2.0+18);
                neuronWindows.at(syn->pN).nextCollapsed = 2;
                neuWin->usingRelative = false;
            }
            ImGui::SameLine(); ImGui::Text("%i -> %i", syn->pN, syn->tN);
            if (!syn->inhibitory) ImGui::TextColored(ImColor(116, 102, 116),"EXCITATORY");
            else ImGui::TextColored(ImColor(26, 26, 116),"inhibitory");
            ImGui::Text("weight: %.2f", syn->getWeight());

        }
        ImGui::PopStyleColor(1);
    }
    float childHeight = ImGui::GetWindowContentRegionMax().y;
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Neuron Image", ImVec2(fmin(windowWidth * 0.2f, 100.f), 300), 0);
        float neuPotScaled = 0.4 + (neuPot+70.f)/200.f;
        float imageSize = fmin(windowWidth*0.2, 100.0f);
        ImGui::Dummy(ImVec2(imageSize, ImGui::GetWindowContentRegionMax().y/2.0f-imageSize/2.0f)); // Vertically centred
        ImGui::Image((void*) neuron_smallTexID, ImVec2(imageSize, imageSize), ImVec2(0,0), ImVec2(1,1), ImVec4(neuPotScaled, neuPotScaled, neuPotScaled, neuPotScaled));
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Out synapses", ImVec2(windowWidth * 0.38f, 0), ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Text("Out synapses");
    int i = 0;
    for (auto &syn: neu->outSynapses){
        ImGui::PushStyleColor(ImGuiCol_Header, ImColor(116, 102, 116, (int) floor(50 + syn.getPostPot()*180.0f)));
        std::sprintf(buffer,"%i", syn.tN);
        if (ImGui::CollapsingHeader(buffer)){
            ImGui::Text("%i -> %i", syn.pN, syn.tN);
            if (ImGui::Button("Open")){
                selectNeuron(syn.tN, true);
                ImVec2 currentWindowPos = ImGui::GetWindowPos();
                neuronWindows.at(syn.tN).nextPosition = ImVec2(currentWindowPos.x+ImGui::GetWindowWidth()/2.0+78, currentWindowPos.y-windowInitY/2.0+18);
                neuronWindows.at(syn.tN).nextCollapsed = 2;
                neuWin->usingRelative = false;
            }
            if (0.0f < syn.getWeight() ) ImGui::TextColored(ImColor(116, 102, 116),"EXCITATORY");
            else ImGui::TextColored(ImColor(26, 26, 116),"inhibitory");
            ImGui::Text("weight: %.2f", syn.getWeight());
            float weightData[neuTimeline->size()];
            for (int j = 0; j < neuTimeline->size(); j++){
                weightData[j] = neuTimeline->at(j).synapseWeights.at(i);
            }
            ImGui::PlotLines("", weightData, neuTimeline->size(), 0, "", -1.0f, 1.0f, ImVec2(77, 48));
            if (ImGui::IsItemHovered()){
                ImGui::BeginTooltip();
                ImGui::PlotLines("", weightData, neuTimeline->size(), 0, "", -1.0f, 1.0f, ImVec2(500, 200));
                ImGui::Text("weight: %.3f", syn.getWeight());
                ImGui::EndTooltip();
            }

        }
        ImGui::PopStyleColor(1);
        i++;
    }
    ImGui::EndChild();

    renderLine(ID);

    ImGui::End();
    ImGui::PopStyleColor(1);
}

void NeuCor_Renderer::renderModule(module* mod, bool windowed){
    bool openTree = false;
    bool activeTree = false;
    if (mod->beingDragged) {
        ImGui::SetNextWindowPos(ImGui::GetMousePos());
        ImGui::SetNextWindowFocus();
        mod->beingDragged = false;
    }

    // Set module treenodes initial state
    if (!windowed) ImGui::SetNextTreeNodeOpen(moduleInitOpen[(int) mod->type], ImGuiSetCond_Once);

    switch (mod->type){

    case MODULE_BRAIN: {
        if (windowed){
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::Begin("Brain", NULL, window_flags);
        }
        else {openTree = ImGui::CollapsingHeader("Brain"); if (!openTree) break; activeTree = ImGui::IsItemActive();}


        ImGui::Text("Neurons: %i", logger.neuronCount);
        ImGui::SameLine(0, 80); ImGui::Text("Synapses: %i", logger.synapseCount);

        ImGui::Text("Brain runtime: %.001f ms", brain->getTime());
        ImGui::Text("FPS: %i", (int) round(FPS));
        ImGui::NewLine();
        // Camera mode switcher
        ImGui::Text("Camera mode:");
        if(ImGui::Button("<"))
            cameraMode = static_cast<cameraModes>((cameraMode-1+cameraModes::CAMERA_count)%cameraModes::CAMERA_count);
        ImGui::SameLine(); ImGui::Text(cameraModeNames.at(cameraMode).data()); ImGui::SameLine(180,0);
        if (glfwGetKey(window, GLFW_KEY_C ) == GLFW_PRESS){
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(2.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(2.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(2.0f, 0.8f, 0.8f));
        }
        if(ImGui::Button(">"))
            cameraMode = static_cast<cameraModes>((cameraMode+1)%cameraModes::CAMERA_count);
        if (glfwGetKey(window, GLFW_KEY_C ) == GLFW_PRESS) ImGui::PopStyleColor(3);

        // Rendering mode switcher
        ImGui::Text("Rendering mode:");
        ImGui::PushID("render left button");
        if(ImGui::Button("<"))
            renderMode = static_cast<renderingModes>((renderMode-1+renderingModes::RENDER_count)%renderingModes::RENDER_count);
        ImGui::PopID();
        ImGui::SameLine(); ImGui::Text(renderingModeNames.at(renderMode).data()); ImGui::SameLine(180,0);
        if (glfwGetKey(window, GLFW_KEY_M ) == GLFW_PRESS){
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(2.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(2.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(2.0f, 0.8f, 0.8f));
        }
        ImGui::PushID("render right button");
        if(ImGui::Button(">"))
            renderMode = static_cast<renderingModes>((renderMode+1)%renderingModes::RENDER_count);
        ImGui::PopID();
        if (glfwGetKey(window, GLFW_KEY_M ) == GLFW_PRESS) ImGui::PopStyleColor(3);
        if (renderMode == RENDER_PLASTICITY){
            ImGui::Checkbox("Only active", &RENDER_PLASTICITY_onlyActive);
        }
        else if (renderMode == RENDER_CLOSENESS){
            ImGui::DragFloat("Strength factor", &closenessIntensity, 0.01, 0, 20, "%.2f", 2.0);
        }
        else if (renderMode == RENDER_ACTIVITY){
            char* letters[] = {"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"};
            if (currentActivity == ""){
                currentActivity = "a";
                for (int i = 0; variables.find(currentActivity) != variables.end(); i++)
                    currentActivity = letters[i];

                variables.insert(std::pair<char*, std::pair<std::unique_ptr<double>, std::vector<float> > >(currentActivity,
                                                  std::pair<std::unique_ptr<double>, std::vector<float> >(std::unique_ptr<double>(new double), std::vector<float>())));
            }
            if (ImGui::Button(currentActivity, ImVec2(50,50)) && variables.size() != sizeof(letters)/sizeof(char*)){

                if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS){
                    /*ImGui::OpenPopup("Change name");
                    strcpy(changingName, currentActivity);*/
                }
                else {
                    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS) {
                        // Store current activities
                        auto lastA = &variables.at(currentActivity);
                        lastA->second.reserve(brain->neurons.size());
                        for (auto &neu: brain->neurons) lastA->second.push_back(neu.activity());
                        currentActivity = "";
                    }
                    brain->resetActivities();
                }
            }/*
            if (ImGui::BeginPopup("Change name")){
                ImGui::InputText("", changingName, 16);
                ImGui::EndPopup();
                nameChanged = true;
            }*/
            if (ImGui::IsItemHovered()){ImGui::BeginTooltip(); ImGui::Text("Save activity\nCtrl + Click to reset"); ImGui::EndTooltip();}
            ImGui::SameLine();
            ImGui::BeginChild("VariableList", ImVec2(ImGui::GetContentRegionAvailWidth(), 100), true);
            ImGui::Columns(3);
            std::vector<char*> toDelete;
            int i = 1;
            for (auto &var: variables){
                if (var.first == currentActivity) continue;
                ImGui::Text(var.first);
                if (ImGui::IsItemClicked()) toDelete.push_back(var.first);
                if (ImGui::IsItemHovered()){ImGui::BeginTooltip(); ImGui::Text("Press to delete %s", var.first); ImGui::EndTooltip();}
                if (i % 10 == 0) ImGui::NextColumn();
                i++;
            }
            while (toDelete.size() != 0){ variables.erase(toDelete.back()); toDelete.pop_back();}

            ImGui::EndChild();

            ImGui::InputText("Expression", activityExpression, 256);
            static int error;
            if (error != 0) ImGui::TextColored(ImColor::HSV(0.0f, 0.7f, 0.7f), "Arithmetic error at %i", error);
            if (ImGui::Button("Use")){
                te_variable vars[1];
                std::size_t var_i = 0;
                for (auto &var: variables) {
                    te_variable tevar = {var.first, (double*)var.second.first.get()};
                    vars[var_i] = tevar;
                    var_i++;
                }
                te_expr* compiled = te_compile(activityExpression, vars, variables.size(), &error);
                if (compiled) evaluated = compiled; // Uses compiled if it succeeded
                if (*activityExpression == *currentActivity && strlen(activityExpression) == strlen(currentActivity))
                    evaluated = NULL; // Nulls, to use a simpler and more effective rendering method
            }

        }
        ImGui::NewLine();
        ImGui::SliderFloat("Learning rate", &brain->learningRate, 0, 4, "%.3f");

    } break;

    case MODULE_TIME: {
        if (windowed){
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::Begin("Time", NULL, window_flags);
        }
        else {openTree = ImGui::CollapsingHeader("Time"); if (!openTree) break; activeTree = ImGui::IsItemActive();}

        bool loadedPaused = false;
        if (paused && runBrainOnUpdate){
            loadedPaused = true;
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor::HSV(0.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImColor::HSV(0.0f, 0.6f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImColor::HSV(0.0f, 0.7f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor::HSV(0.0f, 0.7f, 0.5f));

            if (ImGui::Button("Run")){
                paused = false;
            }
        }
        else if (runBrainOnUpdate){
            if (ImGui::Button("Pause")){
                paused = true;
            }
        }
        ImGui::SameLine();
        if (!realRunspeed)
            ImGui::SliderFloat("Speed", &brain->runSpeed, 0.0, 1.0, "%.4f ms per run", 2.0f);
        else
            ImGui::SliderFloat("Speed", &brain->runSpeed, 0.0, 10.0, "%.4f ms/s ", 2.0f);
        if (loadedPaused) ImGui::PopStyleColor(4);

        if (runBrainOnUpdate){
            ImGui::Checkbox("Real time", &realRunspeed);
            ImGui::SameLine(); ImGui::TextDisabled("[?]");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(450.0f);
                ImGui::TextUnformatted("Makes simulation speed dependent on real time. Speed is defined by: simulation time / real time (ms/s).");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
        ImGui::Checkbox("Run all", &brain->runAll);
        ImGui::SameLine(); ImGui::TextDisabled("[?]");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(450.0f);
            ImGui::TextUnformatted("Simulates all neurons every frame.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

    } break;

    case (MODULE_STATS): {
        if (windowed) {
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::Begin("Statistics", NULL, window_flags);
        }
        else {openTree = ImGui::CollapsingHeader("Statistics"); if (!openTree) break; activeTree = ImGui::IsItemActive();}

        {
            ImGui::Text("Neuron activity distribution");
            static int a_spans = 25.0;
            static float a_range_min = 0.0, a_range_max = 6.0;
            static float a_updatePeriod = 0.2;
            static bool a_updatesOn = true;
            logger.activityUpdateTimer -= deltaTime;

            static std::vector<float> activityDistribution;
            activityDistribution.resize(a_spans, 0);

            if (logger.activityUpdateTimer <= 0 && a_updatesOn || logger.activityUpdateTimer < -100){
                int above = 0, below = 0;
                for (auto &a: activityDistribution) a = 0;
                for (auto &neu : brain->neurons){
                    int spanIndex = floor(a_spans*((float) neu.activity()-a_range_min)/((float) a_range_max-a_range_min));
                    if (spanIndex < 0){
                        below++;
                        continue;
                    }
                    else if (spanIndex >= activityDistribution.size()){
                        above++;
                        continue;
                    }
                    activityDistribution.at(spanIndex)++;
                }
                logger.activityUpdateTimer = a_updatePeriod;
            }
            float* a_data = &activityDistribution[0];
            ImGui::PlotHistogram("", a_data, activityDistribution.size(), 0, "", FLT_MAX, FLT_MAX, ImVec2(0, 200));

            if (ImGui::IsItemClicked()) ImGui::OpenPopup("Activity distribution settings");
            if (ImGui::BeginPopup("Activity distribution settings")){
                ImGui::DragFloatRange2("range", &a_range_min, &a_range_max, 0.25f, 0.0f, 100.0f, "Min: %.1f", "Max: %.1f");
                ImGui::DragInt("spans", &a_spans, 1.f, 1, 100, "%.0f");
                ImGui::Separator();
                if (!a_updatesOn) logger.activityUpdateTimer = a_updatePeriod;
                if (ImGui::Button("Update")){
                    logger.activityUpdateTimer = -100;
                }
                ImGui::Checkbox("", &a_updatesOn);
                ImGui::SameLine(); ImGui::DragFloat("Period", &a_updatePeriod, 0.2f, 0.05f, 60.0f, "%.1f s");
                if (0.5 < a_updatePeriod) ImGui::ProgressBar(logger.activityUpdateTimer/a_updatePeriod, ImVec2(0,0), "");
                if (ImGui::Button("Reset")){a_spans = 2.0, a_range_min = 0.0, a_range_max = 50.0; a_updatePeriod = 5.0; a_updatesOn = true;}
                ImGui::EndPopup();
            }
        }

        {
            ImGui::Text("Synapse weight distribution");
            static int w_spans = 20;
            static float w_range_min = -1.0, w_range_max = 1.0;
            static float w_updatePeriod = 0.2;
            static bool w_updatesOn = true;
            logger.weightUpdateTimer -= deltaTime;

            static std::vector<float> weightDistribution;
            weightDistribution.resize(w_spans, 0);

            if (logger.weightUpdateTimer <= 0 && w_updatesOn || logger.weightUpdateTimer < -100){
                int above = 0, below = 0;
                for (auto &w: weightDistribution) w = 0;
                for (auto &neu : brain->neurons){
                    for (auto &syn : neu.outSynapses){
                        int spanIndex = floor(w_spans*((float) syn.getWeight()-w_range_min)/((float) w_range_max-w_range_min));
                        if (spanIndex < 0){
                            below++;
                            continue;
                        }
                        else if (spanIndex >= weightDistribution.size()){
                            above++;
                            continue;
                        }
                        weightDistribution.at(spanIndex)++;
                    }
                }
                logger.weightUpdateTimer = w_updatePeriod;
            }
            float* w_data = &weightDistribution[0];
            ImGui::PlotHistogram("", w_data, weightDistribution.size(), 0, "", FLT_MAX, FLT_MAX, ImVec2(0, 200));

            if (ImGui::IsItemClicked()) ImGui::OpenPopup("Weight distribution settings");
            if (ImGui::BeginPopup("Weight distribution settings")){
                ImGui::DragFloatRange2("range", &w_range_min, &w_range_max, 0.25f, -10.0f, 10.0f, "Min: %.1f", "Max: %.1f");
                ImGui::DragInt("spans", &w_spans, 1.f, 1, 100, "%.0f");
                ImGui::Separator();
                if (!w_updatesOn) logger.weightUpdateTimer = w_updatePeriod;
                if (ImGui::Button("Update")){
                    logger.weightUpdateTimer = -100;
                }
                ImGui::Checkbox("", &w_updatesOn);
                ImGui::SameLine(); ImGui::DragFloat("Period", &w_updatePeriod, 0.2f, 0.05f, 60.0f, "%.1f s");
                if (0.5 < w_updatePeriod) ImGui::ProgressBar(logger.weightUpdateTimer/w_updatePeriod, ImVec2(0,0), "");
                if (ImGui::Button("Reset")){w_spans = 0.2, w_range_min = -3.0, w_range_max = 3.0; w_updatePeriod = 5.0; w_updatesOn = true;}
                ImGui::EndPopup();
            }
        }

        {
            ImGui::Text("Raster plot");
            float rasterPlotWidth = ImGui::GetContentRegionAvailWidth(), rasterPlotHeight = sqrt(brain->neurons.size())*15.f;
            static float rasterPlotTime = 20.0; // ms

            ImVec2 nextPos = ImVec2(ImGui::GetWindowPos().x + 8, ImGui::GetItemRectMax().y + 5);
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            drawList->AddRectFilled(nextPos, ImVec2(nextPos.x+rasterPlotWidth+10, nextPos.y+rasterPlotHeight+10), ImColor(0.2f, 0.2f, 0.2f));
            float brainTime = brain->getTime();
            int neuronCount = brain->neurons.size();
            static std::deque<std::vector<int> > firePlot;
            int firePlotSize = rasterPlotTime/(brain->runSpeed);

            if (!paused) {
                firePlot.emplace_back();
                firePlot.back().reserve(neuronCount/5);
            }
            for (auto &neu: brain->neurons) {
                if (brainTime - neu.lastFire < brain->runSpeed){
                    firePlot.back().push_back(neu.getID());
                }
            }
            firePlot.back().shrink_to_fit();
            for (int i = 0; i < (int) firePlot.size()-firePlotSize; i++)
                firePlot.pop_front();
            float i = 0;
            for (auto &frame: firePlot){
                for (auto &fire: frame){
                    ImVec2 spikePos = ImVec2(
                            nextPos.x+5+rasterPlotWidth*((float) i/firePlot.size()),
                            nextPos.y+5+rasterPlotHeight*fire/neuronCount);
                    drawList->AddRectFilled(spikePos, ImVec2(spikePos.x+1.5, spikePos.y+1.5), ImColor(0.9f, 0.6f, 0.9f));
                }
                i++;
            }


            ImGui::Dummy(ImVec2(rasterPlotWidth, rasterPlotHeight+50));
            if (ImGui::IsItemClicked()) ImGui::OpenPopup("Raster plot settings");
            if (ImGui::BeginPopup("Raster plot settings")){
                    ImGui::DragFloat("Time range", &rasterPlotTime, 0.02, 0.1, 100, "%.2f", 2);
                    ImGui::EndPopup();
            }
        }

        break;
    }

    case (MODULE_SELECTED_NEURONS): {
        if (windowed) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(250, 0), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::SetNextWindowContentWidth(250);
            ImGui::Begin("Selected neurons");
        }
        else {openTree = ImGui::CollapsingHeader("Selected neurons"); if (!openTree) break; activeTree = ImGui::IsItemActive();}

        ImGui::PushStyleColor(ImGuiCol_Header, ImColor(0, 90, 173));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImColor::HSV(1.57f, 1.0f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImColor::HSV(1.57f, 0.6f, 0.68f));
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        int cellsPerRow = (int) std::max(floor(ImGui::GetWindowWidth()/50.0)-1 , 1.0);
        for (int i = 0; i<selectedNeurons.size(); i++){
            ImGui::PushID(i);

            char buffer[100];
            std::sprintf(buffer, "Neuron\n%i", selectedNeurons.at(i));
            ImGui::Selectable(buffer, &neuronWindows.at(selectedNeurons.at(i)).open, 0, ImVec2(50,50));
            bool deselected = ImGui::IsItemClicked(1);

            float currentPot;
            if (renderMode == RENDER_ACTIVITY)
                currentPot = log(brain->getNeuron(selectedNeurons.at(i))->activity()+1.0);
            else
                currentPot = (brain->getNeuron(selectedNeurons.at(i))->potential()+70.0)/110.0;
            drawList->AddRectFilled(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y+35.0f),
                                    ImGui::GetItemRectMax(), ImColor::HSV(1.52f, .75f, currentPot));

            if ((i+1)%cellsPerRow != 0 && i != selectedNeurons.size()-1)
                ImGui::SameLine();
            ImGui::PopID();

            if (deselected) deselectNeuron(selectedNeurons.at(i));
        }
        ImGui::PopStyleColor(3);

        ImGui::Separator();

        if (ImGui::Button("Deselect all")){
            while (selectedNeurons.size() != 0) deselectNeuron(selectedNeurons.at(0));
        }
        ImGui::SameLine();
        if (ImGui::Button("Open all")){
            for (auto ID: selectedNeurons) neuronWindows.at(ID).open = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close all")){
            for (auto ID: selectedNeurons) neuronWindows.at(ID).open = false;
        }
        static char selectNeuronBuf[64];
        ImGui::InputText("ID", selectNeuronBuf, 64, ImGuiInputTextFlags_CharsDecimal);
        bool shouldSelect = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) && ImGui::IsItemActive();
        ImGui::SameLine();
        if (shouldSelect || ImGui::Button("Select") ){
            int selected = 0; int offset = 0;
            bool noSelected = true;
            for (int i = 0; i<64; i++) if (selectNeuronBuf[i] != 0) {offset = i;}
            for (int i = 63; 0 <= i; i--){
                if (selectNeuronBuf[i] == 0) continue;
                noSelected = false;
                selected += (selectNeuronBuf[i]-48)*(float) powf(10, offset-i);;
            }
            if (brain->neurons.size() <= selected) std::cout<<"Neuron "<<selected<<" does not exist.\n";
            else if (!noSelected){
                selectNeuron(selected, false);
                memset(selectNeuronBuf, 0, 64);
            }
        }
        break;
    }

    case (MODULE_CONTROLS): {
        if (windowed) ImGui::Begin("Controls");
        else {openTree = ImGui::CollapsingHeader("Controls"); if (!openTree) break; activeTree = ImGui::IsItemActive();}

        std::string controlsText =
            "Right click  -  Enter navigation mode\n"
            "Right click + Ctrl  -  Hide interface\n"
            "Left click  -  Toggle selection of neuron\n"
            "Space - Pause/Resume\n"
            ".  -  Speed up time\n"
            ",  -  Slow down time\n"
            "C  -  Go to next camera mode\n"
            "M  -  Go to next rendering mode\n"
            "N - Clear activities\n"
            "Esc - Exit program\n"
            "\n"
            "Camera mode:\n"
            "Mouse look\n"
            "WASD  -  move camera\n"
            "\n"
            "Orbit + Orbit momentum\n"
            "Left click + drag - pan camera\n"
            "Scroll - Move closer/further away\n"
        ;

        ImGui::Text(controlsText.c_str());
        break;
    }

    default: {
        if (windowed) ImGui::CollapsingHeader("Undefined");
        else {openTree = ImGui::CollapsingHeader("Undefined"); if (!openTree) break; activeTree = ImGui::IsItemActive();}
        ImGui::Text("Module isn't defined in NeuCor_Renderer::renderModule().");
        break;
    }

    }
    static graphicsModule justUndocked = MODULE_count;
    if (justUndocked == mod->type && windowed){
        // Keep dragging window by utilizing ImGui internals
        ImGuiContext& g = *GImGui;
        ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
        ImGui::FocusWindow(currentWindow);
        g.MovedWindow = currentWindow;
        g.MovedWindowMoveId = currentWindow->MoveId;
        ImGui::SetActiveID(g.MovedWindowMoveId, currentWindow);

        justUndocked = MODULE_count;
    }
    if (windowed){
        if (mod->snapped && ImGui::IsMouseReleased(0)) {mod->snapped = false; mod->windowed = false;}
        mod->snapped = ImGui::IsMouseHoveringWindow() && ImGui::IsMouseDragging() && ImGui::GetMousePos().x < 80;
        ImGui::End();
    }
    else if (openTree) {
        if (activeTree && ImGui::GetMousePos().x > 80 && ImGui::IsMouseDragging()){
            mod->windowed = true;
            mod->beingDragged = true;
            justUndocked = mod->type;

        }
    }
}

void NeuCor_Renderer::resetCursor(){
    cursorX = NAN; cursorY = NAN;
    glfwSetCursorPos(window, width/2.0, height/2.0);
}

template<typename ... callbackParameters>
void NeuCor_Renderer::inputCallback(callbackErrand errand, callbackParameters ... params){
    std::tuple<callbackParameters...> TTparams(params... );

    switch (errand){

    case (KEY_ACTION):
        if (std::get<1>(TTparams) == GLFW_KEY_ESCAPE && std::get<3>(TTparams) == GLFW_PRESS) glfwSetWindowShouldClose(std::get<0>(TTparams), GL_TRUE); // Close window on escape-key press
        if (std::get<1>(TTparams) == GLFW_KEY_SPACE && std::get<3>(TTparams) == GLFW_PRESS) paused = !paused; // Pause time
        if (std::get<1>(TTparams) == GLFW_KEY_N && std::get<3>(TTparams) == GLFW_PRESS) brain->resetActivities(); // Reset all activity start times
        break;

    case (CHAR_ACTION):
        if (std::get<1>(TTparams) == 99 || std::get<1>(TTparams) == 67){ // Iterate to next camera mode on C-key press
            cameraMode = static_cast<cameraModes>(cameraMode+1);
            if (cameraMode == cameraModes::CAMERA_count) cameraMode = static_cast<cameraModes>(cameraMode-(int) cameraModes::CAMERA_count);
            std::cout<<"Camera mode: "<<cameraModeNames.at(cameraMode)<<'\n';
        }
        else if (std::get<1>(TTparams) == 109 || std::get<1>(TTparams) == 77){ // Iterate to next rendering mode on M-key press
            renderMode = static_cast<renderingModes>(renderMode+1);
            if (renderMode == renderingModes::RENDER_count) renderMode = static_cast<renderingModes>(renderMode-(int) renderingModes::RENDER_count);
            std::cout<<"Rendering mode: "<<renderingModeNames.at(renderMode)<<'\n';
        }
        break;

    case (MOUSE_BUTTON):
        if (std::get<1>(TTparams) == GLFW_MOUSE_BUTTON_LEFT && std::get<2>(TTparams) == GLFW_PRESS && !navigationMode && !ImGui::IsMouseHoveringAnyWindow()){ // Neuron selection
            // Toggle input firer
            if (hoveredInput != -1){
                brain->inputHandler.at(hoveredInput).enabled = ! brain->inputHandler.at(hoveredInput).enabled;
            }
            // Or select neuron
            else {
                #define minDistance 10.0

                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                glm::vec2 cursorPos (xpos, ypos);

                float closestDistance = INFINITY;
                std::size_t ID;
                for (auto &neu: brain->neurons){
                    coord3 neuronPos = neu.position();
                    glm::vec3 screenPos = screenCoordinates(glm::vec3(neuronPos.x, neuronPos.y, neuronPos.z));
                    float flatDist = glm::distance(cursorPos, glm::vec2(screenPos));
                    if (flatDist < closestDistance){
                        closestDistance = flatDist;
                        ID = neu.getID();
                    }
                }
                if (closestDistance < minDistance){
                    if (!selectNeuron(ID, true)) deselectNeuron(ID);   // Tries to select, if false the neuron is already selected and is then deselected.
                }
            }
        }
        if (std::get<1>(TTparams) == GLFW_MOUSE_BUTTON_RIGHT && std::get<2>(TTparams) == GLFW_PRESS && !dockHovered) { // Switch to navigation mode
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {navigationMode = true; showInterface = !showInterface;}
            else navigationMode = !navigationMode;

            if (navigationMode) resetCursor();
            else showInterface = true;
        }
        break;

    case (MOUSE_SCROLL):
        if (cameraMode == CAMERA_ORBIT || cameraMode == CAMERA_ORBIT_MOMENTUM){
            cameraRadius -= std::get<2>(TTparams)/4.0f;
        }
        break;

    case (MOUSE_ENTER):
        mouseInWindow = std::get<1>(TTparams) && glfwGetWindowAttrib(window, GLFW_FOCUSED);
        if (mouseInWindow && navigationMode) resetCursor();
        break;
    }
}

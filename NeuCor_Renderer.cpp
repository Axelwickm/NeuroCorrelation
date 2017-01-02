#include "NeuCor_Renderer.h"

/*  .h & .cpp includes  */
#include <NeuCor.h>

//#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
using namespace glm;

/*  .cpp includes  */

#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <picopng/picopng.cpp>
#include <imgui_impl_glfw_gl3.h>
#include <imgui/imgui_internal.h>

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
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::MOUSE_SCROLL, window, (int) xoffset*1000, (int) yoffset*1000, -1, -1);
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


    /* Load resources */
    loadResources();

    destructCallback = NULL;
    newNeuWinPos.first = NULL;
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

    glClearColor(GLclampf(0.06), GLclampf(0.06), GLclampf(0.07), GLclampf(1.0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLineWidth(2.5);

    synapseProgramID = LoadShaders( "synapse.shader", "synapse.Fshader" );
    neuronProgramID = LoadShaders( "neuron.shader", "neuron.Fshader" );

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
    const char* filename = "neuron.png";

    //load and decode
    std::vector<unsigned char> buffer, image;
    loadFile(buffer, filename);
    unsigned long w, h;

    int error = decodePNG(image, w, h, &buffer[0], (unsigned long)buffer.size());
    if(error != 0) std::cout << "error: " << error << std::endl;


    unsigned char * neuronimgData = &image[0];


    // Create one OpenGL texture
    glGenTextures(1, &neuronTexID);


    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, neuronTexID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, neuronimgData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    filename = "neuron_small.png";

    //load and decode
    buffer, image;
    loadFile(buffer, filename);

    error = decodePNG(image, w, h, &buffer[0], (unsigned long)buffer.size());
    if(error != 0) std::cout << "error: " << error << std::endl;


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
    if (selectedNeuronsWindows.find(id) == selectedNeuronsWindows.end()){
        selectedNeurons.push_back(id);
        selectedNeuronsWindows.insert(std::pair<int, bool> (id, windowOpen));
        logger.timeline.insert(std::pair<int, std::deque<realTimeStats::neuronSnapshot> >(id, std::deque<realTimeStats::neuronSnapshot>()));

        return true;
    }
    else {
        selectedNeuronsWindows.at(id) = windowOpen;
        return false;
    }
}

bool NeuCor_Renderer::deselectNeuron(int id){
    assert(id < brain->neurons.size());
    if (selectedNeuronsWindows.find(id) != selectedNeuronsWindows.end()){
        for (std::vector<int>::iterator i = selectedNeurons.begin(); i < selectedNeurons.end(); i++){
            if (*i == id){
                selectedNeurons.erase(i);
                break;
            }
        }
        selectedNeuronsWindows.erase(id);
        logger.timeline.erase(id);
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

    if (runBrainOnUpdate && realRunspeed && !paused){
        float staticRunSpeed = brain->runSpeed;
        brain->runSpeed = staticRunSpeed*deltaTime;
        brain->run();
        brain->runSpeed = staticRunSpeed;
    }
    else if (runBrainOnUpdate && !paused) brain->run();


    updateCamPos();

    float aspect = (float) width / (float)height;
    glm::mat4 Projection = glm::perspective(glm::radians(65.0f), (float) aspect, 0.05f, 100.0f);
    glm::mat4 View = glm::lookAt(
        camPos,
        camPos+camDir,
        camUp
    );
    vp = Projection * View;


    if (renderMode == RENDER_NOSYNAPSES) logger.synapseCount = 0;

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
            }
            else if (renderMode == RENDER_ACTIVITY){
                synPot.push_back(log(brain->getNeuron(syn.pN)->activity()+1.f));
                synPot.push_back(log(brain->getNeuron(syn.tN)->activity()+1.f));
            }
            else if (renderMode == RENDER_NOSYNAPSES) logger.synapseCount++;
        }
    }
    if (PRINT_CONNECTIONS_EVERY_FRAME) std::cout<<std::endl;

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
    if (navigationMode && mouseInWindow) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
        //std::cout<<"Run-speed: "<<brain->runSpeed<<std::endl;
    }
    // Speed up time
    if (glfwGetKey(window, GLFW_KEY_COMMA ) == GLFW_PRESS){
        brain->runSpeed = powf(brain->runSpeed, 1.01);
        //std::cout<<"Run-speed: "<<brain->runSpeed<<std::endl;
    }
    static bool firstTime = true;
    if ((!navigationMode || !mouseInWindow) && !firstTime) return;
    firstTime = false;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (cursorX != cursorX) {cursorX = xpos; cursorY = ypos;}

    float deltaX = cursorX-xpos;
    float deltaY = cursorY-ypos;


    camHA -= 0.15 * deltaTime * deltaX;
    camVA  += 0.15 * deltaTime * deltaY;

    cursorX = xpos; cursorY = ypos;

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
}

inline glm::vec3 NeuCor_Renderer::screenCoordinates(glm::vec3 worldPos, bool nomalizedZ){
    glm::vec4 posClip = vp * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f );
    glm::vec3 posNDC = glm::vec3(posClip) / posClip.w;
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

void NeuCor_Renderer::renderInterface(){
    ImGui_ImplGlfwGL3_NewFrame();

    for (int i = 0; i<modules.size(); i++){ // Render modules as windows
        if (modules[i].windowed) renderModule(&modules[i], true);
        //std::cout<<modules[i].windowed<<"  "<<modules[i].snapped<<std::endl;
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
        ImGui::End();

    }

    // Render neuron windows which are open
    for (auto ID: selectedNeurons){
        bool* open = &selectedNeuronsWindows.at(ID);
        if (*open) renderNeuronWindow(ID, open);
    }

    ImGui::Render();
}

 float NeuCor_Renderer::getDeltaTime(){
    return deltaTime;
}

void NeuCor_Renderer::renderNeuronWindow(int ID, bool *open){
    #define windowInitX 320
    #define windowInitY 450
    Neuron* neu = brain->getNeuron(ID);
    float neuPot = neu->potential();

    if (newNeuWinPos.first == ID){
        ImGui::SetNextWindowPos(newNeuWinPos.second);
        newNeuWinPos.first = NULL;
    }

    char buffer[100];
    std::sprintf(buffer, "Neuron %i", ID);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImColor(116, 102, 116, (int) floor(50 + neuPot*180.0f)));
    ImGui::SetNextWindowCollapsed(true, ImGuiSetCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(windowInitX, windowInitY), ImGuiSetCond_Appearing);
    ImGui::Begin(buffer, open, 0);

    ImGui::Text("Current voltage: %.01f mV", neuPot);
    ImGui::Text("Firing frequency: %.1f Hz", neu->activity());
    ImGui::Text("Last fire: %1.f ms ago", brain->getTime() - neu->lastFire);

    ImGui::Separator();
    ImGui::Text("Voltage graph");

    std::deque<realTimeStats::neuronSnapshot>* neuTimeline = &logger.timeline.at(ID);
    float voltageData[neuTimeline->size()];
    for (int i = 0; i < neuTimeline->size(); i++){
        voltageData[i] = neuTimeline->at(i).voltage;
    }
    ImGui::PlotLines("", voltageData, neuTimeline->size(), 0, "", -90.0f, 50.0f, ImVec2(300, 100));

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
                newNeuWinPos.first = syn->pN;
                newNeuWinPos.second = ImVec2(currentWindowPos.x-windowInitX-10, currentWindowPos.y-windowInitY/2.0+18);
            }
            ImGui::SameLine(); ImGui::Text("%i -> %i", syn->pN, syn->tN);
            if (0.0f < syn->getWeight() ) ImGui::TextColored(ImColor(116, 102, 116),"EXCITATORY");
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
    for (auto &syn: neu->outSynapses){
        ImGui::PushStyleColor(ImGuiCol_Header, ImColor(116, 102, 116, (int) floor(50 + syn.getPostPot()*180.0f)));
        std::sprintf(buffer,"%i", syn.tN);
        if (ImGui::CollapsingHeader(buffer)){
            ImGui::Text("%i -> %i", syn.pN, syn.tN);
            if (ImGui::Button("Open")){
                selectNeuron(syn.tN, true);
                ImVec2 currentWindowPos = ImGui::GetWindowPos();
                newNeuWinPos.first = syn.tN;
                newNeuWinPos.second = ImVec2(currentWindowPos.x+ImGui::GetWindowWidth()/2.0+78, currentWindowPos.y-windowInitY/2.0+18);
            }
            if (0.0f < syn.getWeight() ) ImGui::TextColored(ImColor(116, 102, 116),"EXCITATORY");
            else ImGui::TextColored(ImColor(26, 26, 116),"inhibitory");
            ImGui::Text("weight: %.2f", syn.getWeight());

        }
        ImGui::PopStyleColor(1);
    }
    ImGui::EndChild();


    // Render line from window to neuron
    coord3 pos3D = neu->position();
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

        if(ImGui::Button("<"))
            renderMode = static_cast<renderingModes>((renderMode-1+renderingModes::Count)%renderingModes::Count);
        ImGui::SameLine(); ImGui::Text(renderingModeNames.at(renderMode).data()); ImGui::SameLine(150,0);
        if (glfwGetKey(window, GLFW_KEY_M ) == GLFW_PRESS){
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(2.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(2.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(2.0f, 0.8f, 0.8f));
        }
        if(ImGui::Button(">"))
            renderMode = static_cast<renderingModes>((renderMode+1)%renderingModes::Count);
        if (glfwGetKey(window, GLFW_KEY_M ) == GLFW_PRESS) ImGui::PopStyleColor(3);

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

    } break;

    case (MODULE_STATS): {
        if (windowed) {
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::Begin("Statistics", NULL, window_flags);
        }
        else {openTree = ImGui::CollapsingHeader("Statistics"); if (!openTree) break; activeTree = ImGui::IsItemActive();}
        ImGui::Text("Neuron activity distribution");

            // Activity dist.
        static float a_span = 2.0, a_range_min = 0.0, a_range_max = 50.0;
        static float a_updatePeriod = 5.0;
        static bool a_updatesOn = true;
        logger.activityUpdateTimer -= deltaTime;

        static std::vector<float> activityDistribution;
        activityDistribution.resize(ceil(float(a_range_max-a_range_min)/a_span), 0);

        if (logger.activityUpdateTimer <= 0 && a_updatesOn || logger.activityUpdateTimer < -100){
            int above = 0, below = 0;
            for (auto &neu : brain->neurons){
                int spanIndex = ceil((neu.activity()-a_range_min)/a_span);
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
            ImGui::DragFloat("span", &a_span, 1.0f, 1.0f, 10.0f, "%f");
            ImGui::Separator();
            if (!a_updatesOn) logger.activityUpdateTimer = a_updatePeriod;
            if (ImGui::Button("Update")){
                logger.activityUpdateTimer = -100;
            }
            ImGui::Checkbox("", &a_updatesOn);
            ImGui::SameLine(); ImGui::DragFloat("Period", &a_updatePeriod, 0.2f, 0.05f, 60.0f, "%.1f s");
            if (0.5 < a_updatePeriod) ImGui::ProgressBar(logger.activityUpdateTimer/a_updatePeriod, ImVec2(0,0), "");
            if (ImGui::Button("Reset")){a_span = 2.0, a_range_min = 0.0, a_range_max = 50.0; a_updatePeriod = 5.0; a_updatesOn = true;}
            ImGui::EndPopup();
        }



        // Weight distribution
        static float w_span = 0.2, w_range_min = -3.0, w_range_max = 3.0;
        static float w_updatePeriod = 5.0;
        static bool w_updatesOn = true;
        logger.weightUpdateTimer -= deltaTime;

        static std::vector<float> weightDistribution;
        weightDistribution.resize(ceil(float(w_range_max-w_range_min)/w_span), 0);

        if (logger.weightUpdateTimer <= 0 && w_updatesOn || logger.weightUpdateTimer < -100){
            int above = 0, below = 0;
            for (auto &neu : brain->neurons){
                for (auto &syn : neu.outSynapses){
                    int spanIndex = ceil((syn.getWeight()-w_range_min)/w_span);
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
            ImGui::DragFloat("span", &w_span, 0.05f, 0.01f, 1.0f, "%f");
            ImGui::Separator();
            if (!w_updatesOn) logger.weightUpdateTimer = w_updatePeriod;
            if (ImGui::Button("Update")){
                logger.weightUpdateTimer = -100;
            }
            ImGui::Checkbox("", &w_updatesOn);
            ImGui::SameLine(); ImGui::DragFloat("Period", &w_updatePeriod, 0.2f, 0.05f, 60.0f, "%.1f s");
            if (0.5 < w_updatePeriod) ImGui::ProgressBar(logger.weightUpdateTimer/w_updatePeriod, ImVec2(0,0), "");
            if (ImGui::Button("Reset")){w_span = 0.2, w_range_min = -3.0, w_range_max = 3.0; w_updatePeriod = 5.0; w_updatesOn = true;}
            ImGui::EndPopup();
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
            ImGui::Selectable(buffer, &selectedNeuronsWindows.at(selectedNeurons.at(i)), 0, ImVec2(50,50));

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
        }
        ImGui::PopStyleColor(3);

        ImGui::Separator();

        if (ImGui::Button("Deselect all")){
            while (selectedNeurons.size() != 0) deselectNeuron(selectedNeurons.at(0));
        }
        ImGui::SameLine();
        if (ImGui::Button("Open all")){
            for (auto ID: selectedNeurons) selectedNeuronsWindows.at(ID) = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close all")){
            for (auto ID: selectedNeurons) selectedNeuronsWindows.at(ID) = false;
        }
        break;
    }

    case (MODULE_CONTROLS): {
        if (windowed) ImGui::Begin("Controls");
        else {openTree = ImGui::CollapsingHeader("Controls"); if (!openTree) break; activeTree = ImGui::IsItemActive();}
        ImGui::Text("Module isn't defined in NeuCor_Renderer::renderModule().");
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
        if (std::get<1>(TTparams) == GLFW_KEY_P && std::get<3>(TTparams) == GLFW_PRESS) paused = !paused; // Pause time
        if (std::get<1>(TTparams) == GLFW_KEY_SPACE && std::get<3>(TTparams) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {navigationMode = true; showInterface = !showInterface;}
            else navigationMode = !navigationMode;

            if (navigationMode) resetCursor();
            else showInterface = true;
        }
        if (std::get<1>(TTparams) == GLFW_KEY_N && std::get<3>(TTparams) == GLFW_PRESS){ // Reset all activity start times
            brain->resetActivities();
        }
        break;

    case (CHAR_ACTION):
        if (std::get<1>(TTparams) == 109 || std::get<1>(TTparams) == 77){ // Iterate to next rendering mode on M-key press
            renderMode = static_cast<renderingModes>(renderMode+1);
            if (renderMode == renderingModes::Count) renderMode = static_cast<renderingModes>(renderMode-(int) renderingModes::Count);
            std::cout<<"Rendering mode: "<<renderingModeNames.at(renderMode)<<std::endl;
        }
        break;

    case (MOUSE_BUTTON):
        if (std::get<1>(TTparams) == GLFW_MOUSE_BUTTON_LEFT && std::get<2>(TTparams) == GLFW_PRESS && !navigationMode && !ImGui::IsMouseHoveringAnyWindow()){ // Neuron selection
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
        break;


    case (MOUSE_ENTER):
        mouseInWindow = std::get<1>(TTparams) && glfwGetWindowAttrib(window, GLFW_FOCUSED);
        if (mouseInWindow && navigationMode) resetCursor();
        break;
    }
}

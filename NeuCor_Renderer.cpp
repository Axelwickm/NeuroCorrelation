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

#include <boost/any.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <picopng.cpp>

std::map<GLFWwindow*, NeuCor_Renderer*> windowRegistry;

/* glfw error function helper */
void glfw_ErrorCallback(int error, const char* description){
    fputs(description, stderr);
}
/* glfw key callback function helper */
static void glfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::KEY_ACTION, window, key, scancode, action, mods);
}
void cursor_enter_callback(GLFWwindow* window, int entered){
    windowRegistry.at(window)->inputCallback(NeuCor_Renderer::MOUSE_ENTER, window, entered, -1, -1, -1);
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
    realRunspeed = false;
    /* Initiates GLFW & OpenGL*/
    initGLFW();
    initOpenGL(window);

    /* Load resources */
    loadResources();

    destructCallback = NULL;
}

NeuCor_Renderer::~NeuCor_Renderer() {
    /* Destroy window and terminate glfw */
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

    glfwSetCursorEnterCallback(window, cursor_enter_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwPollEvents();
    glfwSetCursorPos(window, width/2, height /2);
    cursorX = width/2.0;
    cursorY = height/2.0;
    cursorOnScreen = true;
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


    unsigned char * imgData = &image[0];


    // Create one OpenGL texture
    GLuint neuronTexID;
    glGenTextures(1, &neuronTexID);


    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, neuronTexID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

}
void NeuCor_Renderer::updateView(){
     #define PRINT_CONNECTIONS_EVERY_FRAME false

    double currentTime = glfwGetTime();
    deltaTime = float(lastTime - currentTime);
    lastTime = currentTime;

    // Run brain if runRate != -1
    if (realRunspeed){
        float staticRunSpeed = brain->runSpeed;
        brain->runSpeed = fabs(deltaTime*staticRunSpeed*1000.0);
        brain->run();
        brain->runSpeed = staticRunSpeed;
    }


    updateCamPos();

    float aspect = (float) width / (float)height;
    glm::mat4 Projection = glm::perspective(glm::radians(65.0f), (float) aspect, 0.05f, 100.0f);
    glm::mat4 View = glm::lookAt(
        camPos,
        camPos+camDir,
        camUp
    );
    glm::mat4 vp = Projection * View;


    /// POSSIBLY TODO
    // On left mouse button hold
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS and false){
        glm::vec4 viewport(0, 0, width, height);
        glm::vec3 nearCoord = glm::unProject(glm::vec3(width/2.0, height/2.0, 0.0), View, Projection, viewport);
        glm::vec3 farCoord = glm::unProject(glm::vec3(width/2.0, height/2.0, 1000.0), View, Projection, viewport);
        //std::cout<<nearCoord.x<<" "<<nearCoord.y<<" "<<nearCoord.z<<std::endl;

        float shortestDist = INFINITY;
        unsigned selectedNeu = NAN;
        for (auto &neu: brain->neurons){
            float dist = fabs((farCoord.y - nearCoord.y)*neu.position().x - (farCoord.x - nearCoord.x)*neu.position().y + farCoord.x*nearCoord.y - farCoord.y*nearCoord.x);
            std::cout<<sqrt(powf((double) farCoord.y - nearCoord.y, 2.0) - powf((double) farCoord.y - nearCoord.y, 2.0))<<std::endl;
            dist /= sqrt(powf(farCoord.y - nearCoord.y, 2.0) - powf(farCoord.y - nearCoord.y, 2.0));
            if (dist < shortestDist) shortestDist = dist;
        }
    }


    // Render synapses
    glUseProgram(synapseProgramID);

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
                synPot.push_back(syn.getPostPot());
                synPot.push_back(syn.getPrePot());
            }
            else if (renderMode == RENDER_PLASTICITY){
                synPot.push_back(syn.strength);
                synPot.push_back(syn.strength);
            }
        }
    }
    if (PRINT_CONNECTIONS_EVERY_FRAME) std::cout<<std::endl;

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

    glfwSwapBuffers(window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
 float NeuCor_Renderer::getDeltaTime(){
    return deltaTime;
 }
void NeuCor_Renderer::pollWindow(){
    glfwPollEvents();
    if (cursorOnScreen) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

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
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (cursorOnScreen) {
        camHA += 0.15 * deltaTime * float(cursorX-xpos);
        camVA  -= 0.15 * deltaTime * float(cursorY-ypos);
    }
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
        camPos -= camDir * GLfloat(deltaTime * 1 * speedMult);
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS){
        camPos += camDir * GLfloat(deltaTime * 1 * speedMult);
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_D ) == GLFW_PRESS){
        camPos -= right * GLfloat(deltaTime * 1 * speedMult);
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_A ) == GLFW_PRESS){
        camPos += right * GLfloat(deltaTime * 1 * speedMult);
    }

    if (glfwGetKey(window, GLFW_KEY_PERIOD ) == GLFW_PRESS){
        brain->runSpeed = powf(brain->runSpeed, 0.99);
        std::cout<<"Run-speed: "<<brain->runSpeed<<std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_COMMA ) == GLFW_PRESS){
        brain->runSpeed = powf(brain->runSpeed, 1.01);
        std::cout<<"Run-speed: "<<brain->runSpeed<<std::endl;
    }
}
template<typename ... callbackParameters>
void NeuCor_Renderer::inputCallback(callbackErrand errand, callbackParameters ... params){
    std::tuple<callbackParameters...> TTparams(params... );

    switch (errand){

    case (KEY_ACTION):
        if (std::get<1>(TTparams) == GLFW_KEY_ESCAPE && std::get<3>(TTparams) == GLFW_PRESS) glfwSetWindowShouldClose(std::get<0>(TTparams), GL_TRUE);
        if (std::get<1>(TTparams) == GLFW_KEY_M && std::get<3>(TTparams) == GLFW_PRESS){
            renderMode = static_cast<renderingModes>(renderMode+1);
            if (renderMode == renderingModes::Count) renderMode = static_cast<renderingModes>(renderMode-(int) renderingModes::Count);
        }
        break;

    case (MOUSE_ENTER):
        if (std::get<1>(TTparams) == 1) cursorOnScreen = true;
        else cursorOnScreen = false;
        break;
    }
};

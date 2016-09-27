#version 330 core

layout(location = 0) in vec3 squareVertices;
layout(location = 1) in vec3 xyz; // Position of the centre of the neuron
layout(location = 2) in vec2 potAct;

uniform mat4 VP;
uniform float aspect;

out vec2 UV;
out float opacity;

void main(){
	vec3 particleCenter_wordspace = xyz;

    //Don't render if neuron position is NaN (which happens when it is deleted.)
	if (isnan(xyz.x)){
        return;
	}

    vec3 vertexPosition_worldspace = particleCenter_wordspace;
    float particleSize = min(1.0, potAct[1])/20.0;


	gl_Position = VP * vec4(vertexPosition_worldspace, 1.0f);
	gl_Position /= pow(gl_Position.w, 0.2)*0.1 + 1.0;
	gl_Position.xy += squareVertices.xy * vec2(1.0, aspect) * particleSize;


    UV = squareVertices.xy * vec2(1, -1) + 0.5;
    opacity = potAct[0];
}

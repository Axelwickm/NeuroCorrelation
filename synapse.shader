#version 330 core

layout(location = 0) in vec3 xyz;
layout(location = 1) in float pot;

uniform mat4 VP;
uniform float aspect;

out float opacity;

void main(){
    //Don't render if neuron position is NaN (which happens when it is deleted.)
	if (isnan(xyz.x)){
        return;
	}

	gl_Position = VP * vec4(xyz, 1.0);

    opacity = pot;
}

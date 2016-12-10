#version 330 core

in float opacity;

out vec4 color;

void main(){
  color = vec4(0.3, 0.2, 0.3, opacity);
}

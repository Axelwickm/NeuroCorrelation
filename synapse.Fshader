#version 330 core

in float opacity;

out vec4 color;

void main(){
  color = vec4(0.3, 0.2, 0.3, 0.1+opacity);
  //color *= vec4(opacity, opacity, opacity, 1.0);
}

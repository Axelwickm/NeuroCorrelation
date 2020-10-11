#version 330 core

in float opacity;

out vec4 color;

void main(){
  if (0.0 < opacity)
    color = vec4(0.45, 0.3, 0.45, opacity);

  else
    color = vec4(0.05, 0.05, 0.3, -opacity);

}

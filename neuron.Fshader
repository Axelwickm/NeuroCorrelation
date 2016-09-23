#version 330 core

uniform sampler2D myTextureSampler;
in vec2 UV;
in float opacity;

out vec4 color;

void main(){
  color = texture( myTextureSampler, UV ).rgba;

  color *= opacity;
}

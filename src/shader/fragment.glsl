#version 330 core

in vec3 vertColor;
in vec2 vertTex;

out vec4 color;
out vec4 FragColor;

uniform sampler2D Texture;

void main(){
    FragColor = mix(texture(Texture, vertTex),vec4(vertColor,1.0),0.5);
}
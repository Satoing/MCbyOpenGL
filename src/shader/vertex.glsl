#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texture;
layout (location = 2) in vec3 normal;

out vec3 vertColor;
out vec2 vertTex;

//统一变量需要传入的参数：
//投影矩阵、视图矩阵、变换矩阵
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
//光源位置、光源颜色、物体颜色、摄像机位置
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 objectColor;

void main(){
    vertTex=texture;
    gl_Position=projection*view*model*vec4(pos,1.0f);
    vec3 position=vec3(model*vec4(pos,1.0));
    mat3 normalMatrix=mat3(transpose(inverse(model)));
    vec3 Normal=normalize(normalMatrix*normal);

    //环境光部分
    float ambientStrength = 0.1f;
    vec3 ambient=ambientStrength*lightColor*objectColor;

    //漫反射光部分
    vec3 lightDir=normalize(lightPos-position);
    float diffFactor=max(dot(lightDir,Normal),0);
    vec3 diffuse=diffFactor*lightColor;

    //镜面反射光部分
    float specularStrength=0.3f;
    vec3 reflectDir=normalize(reflect(-lightDir,Normal));
    vec3 viewDir=normalize(viewPos-position);
    float specFactor=pow(max(dot(reflectDir,viewDir),0.0),32);
    vec3 specular=specularStrength*specFactor*lightColor;

    //上面三种光效叠加
    vertColor=(ambient+diffuse+specular)*objectColor;
}
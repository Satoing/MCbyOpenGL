/**仿照Minecraft实现的一个简单三维场景编辑器。
 -------------------------------------------
|Please read readme.md or readme.pdf first.|
-------------------------------------------
1. 按下鼠标左键可以在当前摄像机位置处放置方块，但用于向下取整的原因，方块位置可能会有一些偏移。
2. 按下鼠标右键可以从存档中读取之前场景中方块的数据，并在当前场景中应用。
3. 按下esc键退出程序，并将当前场景中的所有方块数据存入save.dat中。
4. 代码的编码为UTF-8编码，所以可能出现乱码的情况。建议改一下编码，因为给出了详细的注释，不然我注释也白写了……
**/

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>
#include <tool/shader.h>
#include <tool/camera.h>

#include "vertices.h"

std::string Shader::dirName;
using namespace std;

//定义全局变量
//-----------
//setting
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

//camera
Camera camera(glm::vec3(0.0f, 2.0f, 3.0f));//camera对象
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float theta=45.0f;

//timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPos(0,5,0);
bool isFullScreen=false;
bool flag=true;//切换准备放置的方块纹理

//定义一个方块的属性
struct Cube{
public:
    //放置的方块所在的位置，向下取整
    int posX;
    int posY;
    int posZ;
    int tex;//纹理的id
    Cube(int x,int y,int z,int t):posX(x),posY(y),posZ(z),tex(t){}//构造函数
};

//放置的方块，每放置一个方块，就向数组append
vector<Cube> cubes;

//函数声明
//-------
void reshape_callback(GLFWwindow *window, int width, int height);//防止图形发生形变
void mouse_callback(GLFWwindow* window, double xpos, double ypos);//处理鼠标的移动
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);//处理鼠标滚轮的移动
void processInput(GLFWwindow *window);//由于需要获得连续的键盘输入，所以不能用键盘回调函数
void pushToVector();//用于处理将Cube对象放入数组的函数
void button_callback(GLFWwindow *window, int button, int action, int mods);

int main(int argc,char *argv[]){
    //一系列初始化
    //-----------
    Shader::dirName = argv[1];
    glfwInit();

    GLFWmonitor *pMonitor=isFullScreen?glfwGetPrimaryMonitor():nullptr;
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Camera",pMonitor,NULL);
    if(window==NULL){
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    //进入窗口后隐藏鼠标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);

    //定义OpenGL全局配置
    //-----------------
    glClearColor(0.5,0.5,0.5,1.0);
    glEnable(GL_DEPTH_TEST);

    //注册回调函数
    //-----------
    glfwSetFramebufferSizeCallback(window,reshape_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, button_callback);


    //创建和编译着色器
    //---------------
    Shader shader("./shader/vertex.glsl", "./shader/fragment.glsl");//地面和放置的方块共用
    Shader shaderL("./shader/light.vs", "./shader/light.fs");//光源的着色器

    //顶点数据、缓冲、属性
    //------------------
    //地面和放置物体的缓冲对象
    GLuint vao,vbo;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER,vbo);

    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);//位置
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,8*sizeof(GLfloat),(void*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);//纹理坐标
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,8*sizeof(GLfloat),(void*)(5*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);//法向量

    glBindVertexArray(0);

    //光源的缓冲对象
    GLuint vaoL;
    glGenVertexArrays(1,&vaoL);
    glBindVertexArray(vaoL);

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    
    glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(3);//位置

    glBindVertexArray(0);

    //创建和加载纹理
    //------------
    GLuint tex1,tex2,tex3;

    //texture1
    glGenTextures(1,&tex1);
    glBindTexture(GL_TEXTURE_2D,tex1);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    stbi_set_flip_vertically_on_load(true);
    int width,height,nrChannels;
    unsigned char *image=stbi_load("./static/texture/mc/diamond_ore.png",&width,&height,&nrChannels,0);
    if(image){
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
    }
    stbi_image_free(image);

    //texture2
    glGenTextures(1,&tex2);
    glBindTexture(GL_TEXTURE_2D,tex2);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    stbi_set_flip_vertically_on_load(true);
    image=stbi_load("./static/texture/mc/wool.png",&width,&height,&nrChannels,0);
    if(image){
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,image);
    }
    stbi_image_free(image);

    //texture3
    glGenTextures(1,&tex3);
    glBindTexture(GL_TEXTURE_2D,tex3);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    stbi_set_flip_vertically_on_load(true);
    image=stbi_load("./static/texture/mc/planks_jungle.png",&width,&height,&nrChannels,0);
    if(image){
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
    }
    stbi_image_free(image);
    
    //渲染循环
    //-------
    while(!glfwWindowShouldClose(window)){
        //input
        //-----
        processInput(window);
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame-lastFrame;
        lastFrame = currentFrame;
        lightPos.x=1.2*cos(glfwGetTime());

        //render
        //------
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        //绘制光源
        //=======
        //激活着色器
        shaderL.use();
        glBindVertexArray(vaoL);

        glm::mat4 model=glm::mat4(1);
        model=glm::translate(model,lightPos);
        model=glm::scale(model,glm::vec3(0.2,0.2,0.2));

        //视图和投影变换矩阵
        glm::mat4 view = camera.GetViewMatrix();//直接从camera类中获取视图矩阵
        glm::mat4 projection = glm::perspective(glm::radians(theta),(float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 100.0f);

        //将数据传递给统一变量
        shaderL.setMat4("model",model);
        shaderL.setMat4("projection",projection);
        shaderL.setMat4("view",view);

        glDrawArrays(GL_QUADS,0,24);
        glBindVertexArray(0);
        
        //绘制立方体
        //=========
        shader.use();
        
        //将数据传递给统一变量
		shader.setMat4("view",view);
        shader.setMat4("projection",projection);

        shader.setVec3("lightPos",glm::vec3(lightPos.x,lightPos.y,lightPos.z));
        shader.setVec3("lightColor",glm::vec3(1,1,1));
        shader.setVec3("viewPos",glm::vec3(camera.Position.x,camera.Position.y,camera.Position.z));
        shader.setVec3("objectColor",glm::vec3(1.0,1.0,1.0));

        glBindVertexArray(vao);
        
        //利用几何变换创建地面
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindTexture(GL_TEXTURE_2D,tex1);
        
        glm::mat4 initial=glm::translate(glm::mat4(1),glm::vec3(-9.5,-0.5,-9.5));
        for(int i=0;i<20;i++)
            for(int j=0;j<20;j++){
                glm::mat4 model=initial;
                model=glm::translate(model,glm::vec3(j,0,i));
                shader.setMat4("model",model);
                glDrawArrays(GL_QUADS,0,24);
            }

        //绘制vector中记录的物体
        //====================
        initial=glm::translate(glm::mat4(1),glm::vec3(0.5,0.5,0.5));
        for(auto cube:cubes){
            glBindTexture(GL_TEXTURE_2D,(cube.tex==2)?tex2:tex3);//实现纹理的选取
            glm::mat4 model=initial;
            model=glm::translate(model,glm::vec3(cube.posX,cube.posY,cube.posZ));
            shader.setMat4("model",model);
            glDrawArrays(GL_QUADS,0,24);
        }
        
        //swap buffers and poll IO events
        //-------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    //清除资源并退出
    //--------
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1, &vaoL);

    glfwTerminate();
    return 0;
}

void reshape_callback(GLFWwindow *window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window){
    //按下esc键，退出程序并将当前场景的数据保存到文件save.dat中
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        ofstream ofs("save.dat");
        if(!ofs.is_open())
            cout<<"fail to open"<<'\n';
        for(auto cube:cubes){//退出时将当前的cubes中的数据写入文件
            ofs<<cube.posX<<' '<<cube.posY<<' '<<cube.posZ<<' '<<cube.tex<<'\n';
            cout<<cube.posX<<' '<<cube.posY<<' '<<cube.posZ<<' '<<cube.tex<<'\n';
        }
        ofs.close();

        glfwSetWindowShouldClose(window, true);
    }
    //控制摄像机的移动
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    //切换方块的纹理
    if(glfwGetKey(window,GLFW_KEY_1)==GLFW_PRESS) flag=true;
    if(glfwGetKey(window,GLFW_KEY_2)==GLFW_PRESS) flag=false;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn){
    float xpos = static_cast<float>(xposIn);//损失精度的强制类型转换
    float ypos = static_cast<float>(yposIn);//这样编译器不会警告

    //防止第一次进入时出现窗口抖动的情况
    if (firstMouse){
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    //计算offest并更新lastXY
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    //camera类处理鼠标移动的方法，传入offset即可
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    //camera类处理滚轮移动的方法
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
    theta=camera.Zoom;
}

void button_callback(GLFWwindow *window, int button, int action, int mods){
    if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS){
        cout<<"call for one time."<<'\n';
        pushToVector();
    }
    //按下鼠标右键，从文件中读取上次场景的数据
    if(button==GLFW_MOUSE_BUTTON_RIGHT&&action==GLFW_PRESS){
        ifstream ifs("save.dat");
        char buf[8];
        while(ifs.getline(buf,sizeof(buf))){
            //要减去48，为什么我也不知道，调试的时候测出来的，始终差48……
            //我悟了，是因为char的原因。数字的ASCII码和其实际值相差48
            cubes.emplace_back(buf[0]-48,buf[2]-48,buf[4]-48,buf[6]-48);
            //cout<<buf[0]<<' '<<buf[2]<<' '<<buf[4]<<' '<<buf[6]<<'\n';
        }
        ifs.close();
    }
}

void pushToVector(){
    //glm::vec3 front=camera.Front;
    glm::vec3 position=camera.Position;

    //初始的检测位置
    int x=position.x,y=position.y,z=position.z;//,count=3;
    cubes.emplace_back(x,y,z,flag?2:3);//在摄像机的位置处放置方块

    /**下面的代码有BUG。本来准备沿视线的射线方向进行碰撞检测的，但是方块只能堆一层
    找了半天没找出问题，所以就换成简单一点的：直接在摄像机的位置处放置方块**/

    // int x_temp=x,y_temp=y,z_temp=z;
    // float k1=front.x/front.y,k2=front.z/front.y;
    // bool record=false;
    
    // while(y>0&&count>0){
    //     for(auto cube:cubes){
    //         if((x==cube.posX)&&(y==cube.posY)&&(z==cube.posZ)){
    //             cubes.emplace_back(x_temp,y_temp,z_temp,flag?1:0);
    //             record=true;
    //             cout<<"OK"<<'\n';
    //         }
    //     }
    //     x_temp=x,y_temp=y,z_temp=z;
    //     count--;y--;
    //     if(record==true) break;
    //     x=position.x+k1*(y-position.y);
    //     z=position.z+k2*(y-position.y);
    // }
    // if(y<=0) cubes.emplace_back(x,y,z,flag?1:0);
    // cout<<cubes.size()<<'\n';
}
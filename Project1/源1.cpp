#include <glad/glad.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include"shaderr.cpp"
#include <stb_image.h>
#include"myimage.cpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include"camera.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include"model.cpp"

float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
float deltaTime = 0.0f; // ��ǰ֡����һ֡��ʱ���
float lastFrame = 0.0f; // ��һ֡��ʱ��
float lastX = 400, lastY = 300, xoffset = 0, yoffset = 0;
bool mouseFirstSample = true;   // first callback only stores the position (no jump on startup)
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (mouseFirstSample)
    {
        lastX = xpos;
        lastY = ypos;
        mouseFirstSample = false;
        return;
    }
    xoffset = xpos - lastX;
    yoffset = lastY - ypos; // ע���������෴�ģ���Ϊy�����Ǵӵײ����������������
    lastX = xpos;
    lastY = ypos;
}
float fov = 45.0;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (fov >= 1.0f && fov <= 45.0f)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 45.0f)
        fov = 45.0f;
}

int main()
{
    unsigned int width = 800, height = 600;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // IMPORTANT: build the model only after the GL context exists (GLAD loaded).
    // At global scope this runs during static initialization, before assimp's own
    // logger is constructed and before any GL function pointer is available -> crash.
    Model ourModel("cube.obj");

    Shader shaderProgram("vertex", "fragmenttex");
    /* glm::mat4 trans = glm::mat4(1.0f);
     trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
     trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
     unsigned int transformLoc = glGetUniformLocation(shaderProgram.ID, "transform");
     glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));*/
    
    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute: byte offset must be 3 * sizeof(float) = 12, and it MUST be enabled
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    

    
    glEnable(GL_DEPTH_TEST);

    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    camera.setMouseSensitivity(0.05);


    Shader light("vertex","lightfrag");
    light.use();
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::vec3 coral(1.0f, 0.5f, 0.31f);
    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    // ֻ��Ҫ��VBO�����ٴ�����VBO�����ݣ���Ϊ���ӵ�VBO�������Ѿ���������ȷ�������嶥������
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // ���õ�������Ķ������ԣ������ǵĵ���˵����ֻ��λ�����ݣ�
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    bool firstMouse = true;



    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        camera.setspeed(2 * deltaTime);
        camera.ProcessMouseMovement(xoffset, yoffset);
        xoffset = 0, yoffset = 0;
        camera.processInput(window);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 proj = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        // ע�⣬���ǽ�����������Ҫ�����ƶ������ķ������ƶ���

        
        
        shaderProgram.use();
        shaderProgram.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
        shaderProgram.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        shaderProgram.setMat4("model", model);
        shaderProgram.setMat4("projection", proj);
        shaderProgram.setMat4("view", view);
        shaderProgram.setVec3("lightPos", lightPos);
        glm::vec3 lightColor;
        lightColor.x = sin(glfwGetTime() * 2.0f);
        lightColor.y = sin(glfwGetTime() * 0.7f);
        lightColor.z = sin(glfwGetTime() * 1.3f);

        glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f); // 降低影响
        glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f); // 很低的影响

        shaderProgram.setVec3("light.ambient", ambientColor);
        shaderProgram.setVec3("light.diffuse", diffuseColor);
        shaderProgram.setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
        shaderProgram.setVec3("material.diffuse", 1.0f, 0.5f, 0.31f);
        shaderProgram.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
        shaderProgram.setFloat("material.shininess", 32.0f);
        // viewPos MUST be uploaded while shaderProgram is bound:
        // glUniform* always acts on the currently active program.
        shaderProgram.setVec3("viewPos", camera.Position);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // draw the model: shaderProgram is still the active program here
        glm::mat4 modelMat = glm::mat4(1.0f);
        shaderProgram.setMat4("model", modelMat);
        ourModel.Draw(shaderProgram);
        
        /*light.use();
        light.setMat4("projection", proj);
        light.setMat4("view", view);
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        light.setMat4("model", model);
       
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
         */

        processInput(window);
        glfwSwapBuffers(window);
        glfwPollEvents();

    }




    glfwTerminate();
    return 0;
}

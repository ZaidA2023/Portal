#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include "camera.cpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <array>
#include "shader.h"

float deltaTime = 0.0f;
float lastFrame = 0.0f;
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

Camera camera(glm::vec3(0.0f, -3.7f, 1.0f), 
              glm::vec3(0.0f, 1.0f, 0.0f), 
              -90.0f, 0.0f);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void updateCurve(std::vector<glm::vec3> points, bool& update);
void printVec3(const glm::vec3& v);
void renderScene(Shader& shader, unsigned int VAO_curve, unsigned int VAO_points, unsigned int VAO_cube, 
                 bool isWireframe);
unsigned int loadTexture(const char* path);

std::vector<float> positions;
std::vector<float> controlPointVertices;
struct CurvePointData {
    glm::vec3 position;
    glm::vec3 tangent;
};
std::vector<CurvePointData> curvePoints; 
       
std::pair<std::vector<float>, std::vector<unsigned int>> createWalls() {
    float size = 2.0f;
    float texRepeat = 4.0f; // Controls how many times the texture repeats
    
    std::vector<float> vertices = {
        // Format: x, y, z, tex_x, tex_y
        
        // Bottom
        -size, -size*2, -size,   0.0f, 0.0f,              
        size, -size*2, -size,    texRepeat, 0.0f,     
        size, -size*2, size,     texRepeat, texRepeat, 
        -size, -size*2, size,    0.0f, texRepeat,       
        
        // Top 
        -size, 0.0f, -size,    0.0f, 0.0f,            
        size, 0.0f, -size,     texRepeat, 0.0f,         
        size, 0.0f, size,      texRepeat, texRepeat,   
        -size, 0.0f, size,     0.0f, texRepeat,       
        
        // Front
        -size, -size*2, size,    0.0f, 0.0f,           
        size, -size*2, size,     texRepeat, 0.0f,      
        size, 0.0f, size,      texRepeat, texRepeat,    
        -size, 0.0f, size,     0.0f, texRepeat,         
        
        // Back 
        -size, -size*2, -size,   texRepeat, 0.0f,        
        size, -size*2, -size,    0.0f, 0.0f,         
        size, 0.0f, -size,     0.0f, texRepeat,      
        -size, 0.0f, -size,    texRepeat, texRepeat,  
        
        // Left 
        -size, -size*2, -size,   0.0f, 0.0f,            
        -size, -size*2, size,    texRepeat, 0.0f,         
        -size, 0.0f, size,     texRepeat, texRepeat,    
        -size, 0.0f, -size,    0.0f, texRepeat,         
        
        // Right 
        size, -size*2, -size,    texRepeat, 0.0f,        
        size, -size*2, size,     0.0f, 0.0f,            
        size, 0.0f, size,      0.0f, texRepeat,        
        size, 0.0f, -size,     texRepeat, texRepeat     
    };
    
    std::vector<unsigned int> indices = {
        // Bottom
        0, 1, 2,   0, 2, 3,
        // Top
        4, 5, 6,   4, 6, 7,
        // Front
        8, 9, 10,  8, 10, 11,
        // Back
        12, 13, 14, 12, 14, 15,
        // Left
        16, 17, 18, 16, 18, 19,
        // Right
        20, 21, 22, 20, 22, 23
    };
    
    return {vertices, indices};
}

void printVec3(const glm::vec3& v) {
    std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
}

void renderScene(Shader& shader, unsigned int VAO_curve, unsigned int VAO_points, unsigned int VAO_cube, 
                bool isWireframe) {
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), 
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                                0.1f, 100.0f);
    
    shader.setMat4("view", viewMatrix);
    shader.setMat4("projection", projectionMatrix);
    
    if (isWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Draw the cube
    glBindVertexArray(VAO_cube);
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    shader.setMat4("model", modelMatrix);
    
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Textured Cube", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Set up callbacks
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    
    Shader ourShader("/Users/azaid/Documents/Graphics/ogl/src/vertex.glsl", 
                     "/Users/azaid/Documents/Graphics/ogl/src/fragment.glsl");
    glDisable(GL_CULL_FACE);

    // Create and setup cube vertex data
    auto [cubeVertices, cubeIndices] = createWalls();
    
    // Create VAOs and VBOs
    unsigned int VAO_cube, VBO_cube, EBO_cube;
    glGenVertexArrays(1, &VAO_cube);
    glGenBuffers(1, &VBO_cube);
    glGenBuffers(1, &EBO_cube);
    
    // Bind VAO first
    glBindVertexArray(VAO_cube);
    
    // Bind and set VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO_cube);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(float), cubeVertices.data(), GL_STATIC_DRAW);
    
    // Bind and set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_cube);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), cubeIndices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Unbind VBO to prevent accidental modification
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Don't unbind EBO while VAO is active as the bound EBO is stored in the VAO
    
    // Unbind VAO
    glBindVertexArray(0);
    
    // Load and create a texture
    unsigned int texture1;
    texture1 = loadTexture("/Users/azaid/Documents/Graphics/ogl/src/walltexture.jpgf8e1ee5c-aa56-40f9-986a-a583fbad830bLarge.jpg");  // Replace with your texture path
    
    // Tell the shader to use texture unit 0
    ourShader.use();
    ourShader.setInt("texture1", 0);
    
    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        
        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        
        // Pass transformations to the shader
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        
        glBindVertexArray(VAO_cube);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        
        // IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Delete
    glDeleteVertexArrays(1, &VAO_cube);
    glDeleteBuffers(1, &VBO_cube);
    glDeleteBuffers(1, &EBO_cube);
    glDeleteTextures(1, &texture1);

    glfwTerminate();
    return 0;
}

unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    static bool cursorLocked = false;
    static bool tabWasPressed = false;
    
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabWasPressed) {
            cursorLocked = !cursorLocked;
            if (cursorLocked) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            tabWasPressed = true;
        }
    } else {
        tabWasPressed = false;
    }
    
    if (cursorLocked) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
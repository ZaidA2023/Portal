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
float width = 4.0f;
float height = 2.0f;
std::vector<float> interPos;
std::vector<unsigned int> interInd;

glm::vec3 firstPort(0,0,0);
glm::vec3 secondPort(0,0,0);

Camera camera(glm::vec3(0.0f, -height + 0.3f, 1.0f), 
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
bool checkIntersection(glm::vec3& rayOrigin, glm::vec3& rayVector, std::vector<glm::vec3>& face, glm::vec3& point);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

std::vector<float> positions;
       
std::pair<std::vector<float>, std::vector<unsigned int>> createWalls() {
    float texCount = 4.0f; // Controls how many times the texture repeats
    float modifier = 4.0f;
    
    
    std::vector<float> vertices = {
        // Format: x, y, z, tex_x, tex_y
        
        // Bottom
        -width, -height, -width,   0.0f, 0.0f,              
        width, -height, -width,    texCount, 0.0f,     
        width, -height, width,     texCount, texCount, 
        -width, -height, width,    0.0f, texCount,       
        
        // Top 
        -width, 0.0f, -width,    0.0f, 0.0f,            
        width, 0.0f, -width,     texCount, 0.0f,         
        width, 0.0f, width,      texCount, texCount,   
        -width, 0.0f, width,     0.0f, texCount,       
        
        // Front
        -width, -height, width,    0.0f, 0.0f,           
        width, -height, width,     texCount, 0.0f,      
        width, 0.0f, width,      texCount, texCount/modifier,    
        -width, 0.0f, width,     0.0f, texCount/modifier,         
        
        // Back 
        -width, -height, -width,   texCount, 0.0f,        
        width, -height, -width,    0.0f, 0.0f,         
        width, 0.0f, -width,     0.0f, texCount/modifier,      
        -width, 0.0f, -width,    texCount, texCount/modifier,  
        
        // Left 
        -width, -height, -width,   0.0f, 0.0f,            
        -width, -height, width,    texCount, 0.0f,         
        -width, 0.0f, width,     texCount, texCount/modifier,    
        -width, 0.0f, -width,    0.0f, texCount/modifier,         
        
        // Right 
        width, -height, -width,    texCount, 0.0f,        
        width, -height, width,     0.0f, 0.0f,            
        width, 0.0f, width,      0.0f, texCount/modifier,        
        width, 0.0f, -width,     texCount, texCount/modifier     
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

std::pair<std::vector<float>, std::vector<unsigned int>> createCrosshair() {
    float size = 0.03f; // Size of the crosshair

    std::vector<float> vertices = {
        -size, -size, 0.0f,
         size, -size, 0.0f,
         size,  size, 0.0f,
        -size,  size, 0.0f
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        0, 2, 3
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
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    
    Shader ourShader("/Users/azaid/Documents/Graphics/ogl/src/vertex.glsl", 
                     "/Users/azaid/Documents/Graphics/ogl/src/fragment.glsl");
                     
    Shader crosshairShader("/Users/azaid/Documents/Graphics/ogl/src/crossVert.glsl",
                          "/Users/azaid/Documents/Graphics/ogl/src/crossFrag.glsl");
                          
    glDisable(GL_CULL_FACE);

    // Create and setup cube vertex data
    auto [cubeVertices, cubeIndices] = createWalls();
    interInd = cubeIndices;
    interPos = cubeVertices;
    
    // Create and setup crosshair vertex data
    auto [crosshairVertices, crosshairIndices] = createCrosshair();
    
    // Create VAOs and VBOs for cube
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
    
    // Unbind VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Create VAO and VBO for crosshair
    unsigned int VAO_crosshair, VBO_crosshair, EBO_crosshair;
    glGenVertexArrays(1, &VAO_crosshair);
    glGenBuffers(1, &VBO_crosshair);
    glGenBuffers(1, &EBO_crosshair);
    
    // Bind crosshair VAO
    glBindVertexArray(VAO_crosshair);
    
    // Bind and set crosshair VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO_crosshair);
    glBufferData(GL_ARRAY_BUFFER, crosshairVertices.size() * sizeof(float), crosshairVertices.data(), GL_STATIC_DRAW);
    
    // Bind and set crosshair EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_crosshair);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, crosshairIndices.size() * sizeof(unsigned int), crosshairIndices.data(), GL_STATIC_DRAW);
    
    // Position attribute for crosshair
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind VBO and VAO for crosshair
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Load and create a texture
    unsigned int texture1;
    texture1 = loadTexture("/Users/azaid/Documents/Graphics/ogl/src/walltexture.jpg");  // Replace with your texture path
    
    // Tell the shader to use texture unit 0
    ourShader.use();
    ourShader.setInt("texture1", 0);
    
    // Set crosshair color and size in the shader
    crosshairShader.use();
    crosshairShader.setVec4("crosshairColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
    crosshairShader.setFloat("innerRadius", 0.3f); 
    crosshairShader.setFloat("outerRadius", 0.5f); 
    
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
        
        // Disable depth test so the crosshair always appears on top
        glDisable(GL_DEPTH_TEST);
        
        crosshairShader.use();
        glm::vec3 crosshairPosition = camera.Position + camera.Front * 0.5f;
        glm::mat4 crossModel = glm::mat4(1.0f);
        crossModel = glm::translate(model, crosshairPosition);
        crosshairShader.setMat4("model", model);
        crosshairShader.setMat4("view", camera.GetViewMatrix());
        crosshairShader.setMat4("projection", glm::perspective(glm::radians(45.0f), 
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f));

        glBindVertexArray(VAO_crosshair);
        glDrawElements(GL_TRIANGLES, crosshairIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        
        // IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Delete
    glDeleteVertexArrays(1, &VAO_cube);
    glDeleteBuffers(1, &VBO_cube);
    glDeleteBuffers(1, &EBO_cube);
    glDeleteVertexArrays(1, &VAO_crosshair);
    glDeleteBuffers(1, &VBO_crosshair);
    glDeleteBuffers(1, &EBO_crosshair);
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  if(glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) return;
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    for(int i = 0; i < interInd.size(); i = i+3) {
      int step = 5;
      unsigned int index1 = interInd[i] * step; //0
      unsigned int index2 = interInd[i+1] * step; //1
      unsigned int index3 = interInd[i+2] * step; //2
      //0,1,2 -> (0,1,2),(5,6,7),(10,11,12)
      std::vector<glm::vec3> face = {
        glm::vec3(interPos[index1], interPos[index1 + 1], interPos[index1 + 2]),
        glm::vec3(interPos[index2], interPos[index2 + 1], interPos[index2 + 2]),
        glm::vec3(interPos[index3], interPos[index3 + 1], interPos[index3 + 2])
      };

      if(firstPort != glm::vec3(0,0,0) && secondPort == glm::vec3(0,0,0)) {
        if(checkIntersection(camera.Position, camera.Front, face, secondPort)) {
          std::cout<< " Second one!" <<std::endl;
          printVec3(secondPort);
        }
      } else {
        if(checkIntersection(camera.Position, camera.Front, face, firstPort)) {
          std::cout<< " First one!" <<std::endl;
          printVec3(firstPort);
        }
      }

    }
  }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


bool checkIntersection(glm::vec3& rayOrigin, glm::vec3& rayVector, std::vector<glm::vec3>& face, glm::vec3& point) {
    const float EPSILON = 0.0000001;
    glm::vec3 vertex0 = face[0];
    glm::vec3 vertex1 = face[1];
    glm::vec3 vertex2 = face[2];
    glm::vec3 edge1, edge2, h, s, q;
    float a, f, u, v;
    edge1 = vertex1 - vertex0;
    edge2 = vertex2 - vertex0;
    h = glm::cross(rayVector, edge2);
    a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;
    f = 1 / a;
    s = rayOrigin - vertex0;
    u = f * glm::dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    q = glm::cross(s, edge1);
    v = f * glm::dot(rayVector, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    float t = f * glm::dot(edge2, q);
    if (t > EPSILON) {
        glm::vec3 plus = glm::normalize(rayVector) * (t * glm::length(rayVector));
        point = rayOrigin + plus;
        return true;
    } else
        return false;
}
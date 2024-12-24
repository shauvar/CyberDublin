#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib> // For rand()
#include <ctime>   // For seeding rand()
#include <sstream>
#include <fstream>

// Global variables
int windowWidth = 1080;
int windowHeight = 800;
glm::mat4 projection;
glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

glm::vec3 cameraPos(0.0f, 5.0f, 10.0f);
float cameraSpeed = 0.05f;                             // Reduced speed
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f); // Slight downward angle
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

const int gridSizeX = 6; // Number of buildings in X direction
const int gridSizeZ = 6; // Number of buildings in Z direction
float buildingHeights[gridSizeX][gridSizeZ];

float xOffset[gridSizeX]; // Track X offset for each column

GLuint skyboxVAO, skyboxVBO;
GLuint skyboxShaderProgram;
GLuint skyboxTexture;


// Utility function to read a shader file
std::string readFile(const char *filePath)
{
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to compile shaders
GLuint compileShader(const char *vertexPath, const char *fragmentPath)
{
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    GLuint vertex, fragment;
    GLint success;
    GLchar infoLog[512];

    // Compile vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Compile fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Link shaders into a program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return shaderProgram;
}

// Cube vertices with normals
float cubeVertices[] = {
    // positions          // texture coords // normals
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,

    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f,

    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};

    float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,

    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f
};


// Load a texture from file
GLuint loadTexture(const char *path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    std::cout << "Attempting to load texture from: " << path << std::endl;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        std::cout << "Successfully loaded texture with:" << std::endl;
        std::cout << "Width: " << width << std::endl;
        std::cout << "Height: " << height << std::endl;
        std::cout << "Channels: " << nrChannels << std::endl;
        
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;  // Add this line
        return 0;
    }
    stbi_image_free(data);
    return textureID;
}
// Setup OpenGL buffers
void setupOpenGL(GLuint &VAO, GLuint &VBO)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void setupSkybox() {
    // Generate and bind VAO and VBO for skybox
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Load skybox shader
    skyboxShaderProgram = compileShader("../shaders/skybox_vertex_shader.glsl", 
                                      "../shaders/skybox_fragment_shader.glsl");

    // Change the path to match the building texture path format
    skyboxTexture = loadTexture("../assets/sky.jpg");  // Changed from "./assets/sky.jpg"
    
    if (skyboxTexture == 0) {
        std::cerr << "ERROR: Could not load skybox texture. Check if ../assets/sky.jpg exists." << std::endl;
        // Create a default blue texture as fallback
        unsigned char defaultColor[] = {100, 149, 237, 255}; // Cornflower blue
        glGenTextures(1, &skyboxTexture);
        glBindTexture(GL_TEXTURE_2D, skyboxTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}


void renderSkybox(const glm::mat4& view, const glm::mat4& projection) {
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderProgram);
    
    // Remove translation from view matrix
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Set the texture uniform
    glUniform1i(glGetUniformLocation(skyboxShaderProgram, "skyboxTexture"), 0);
    
    // Bind skybox texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyboxTexture);
    
    // Render skybox quad
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 24); // Updated vertex count
    glBindVertexArray(0);
    
    glDepthFunc(GL_LESS);
}

// Callback to handle window resizing
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, windowWidth, windowHeight);

    float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 50.0f);
}

void processInput(GLFWwindow *window)
{
    float moveSpeed = 0.010f; // Adjust this value to control movement speed

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cameraPos.z -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cameraPos.z += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cameraPos.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cameraPos.x += moveSpeed;

    // Add escape key to close window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create GLFW window
    GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "CyberDublin", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set the framebuffer size callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Compile shaders
    GLuint shaderProgram = compileShader("../shaders/vertex_shader.glsl", "../shaders/fragment_shader.glsl");

    // Set up buffers
    GLuint VAO, VBO;
    setupOpenGL(VAO, VBO);
    setupSkybox();

    // Load texture
    GLuint texture1 = loadTexture("../assets/building.jpg");

    // Set the initial projection matrix
    float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 50.0f);

    glm::mat4 view = glm::lookAt(cameraPos,
                                 glm::vec3(0.0f, 0.0f, 0.0f),  // Look at center
                                 glm::vec3(0.0f, 1.0f, 0.0f)); // Up vector

    // Seed the random number generator and initialize building heights
    srand(static_cast<unsigned>(time(0)));
    for (int i = 0; i < gridSizeX; ++i)
    {
        for (int j = 0; j < gridSizeZ; ++j)
        {
            // Increase height range to 2.0 - 8.0 for much taller buildings
            buildingHeights[i][j] = 2.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (8.0f - 2.0f)));
        }
    }

    float zOffset[gridSizeZ]; // Track the Z offset for each row of buildings

    for (int i = 0; i < gridSizeX; ++i)
    {
        xOffset[i] = -5.0f + i * 2.0f; // Initial X positions
    }
    // Initialize zOffsets for each row
    for (int j = 0; j < gridSizeZ; ++j)
    {
        zOffset[j] = -5.0f + j * 2.0f; // Initial positions
    }

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update view matrix with the new camera position
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        renderSkybox(view, projection);
        // Use shader program
        glUseProgram(shaderProgram);

        // Pass transformation matrices to the shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

        // Bind the texture
        glBindTexture(GL_TEXTURE_2D, texture1);
        glBindVertexArray(VAO);

        

        // Modified building rendering loop:
        for (int i = 0; i < gridSizeX; ++i)
        {
            for (int j = 0; j < gridSizeZ; ++j)
            {
                // Calculate the base positions relative to the camera
                float baseX = xOffset[i];
                float baseZ = zOffset[j];

                // Reposition buildings based on the camera position
                while (cameraPos.x - baseX > gridSizeX)
                {
                    xOffset[i] += gridSizeX * 2.0f;
                    baseX = xOffset[i];
                    buildingHeights[i][j] = 2.0f + static_cast<float>(rand()) /
                                                       (static_cast<float>(RAND_MAX / (8.0f - 2.0f)));
                }

                while (cameraPos.x - baseX < -gridSizeX)
                {
                    xOffset[i] -= gridSizeX * 2.0f;
                    baseX = xOffset[i];
                    buildingHeights[i][j] = 2.0f + static_cast<float>(rand()) /
                                                       (static_cast<float>(RAND_MAX / (8.0f - 2.0f)));
                }

                while (cameraPos.z - baseZ > gridSizeZ)
                {
                    zOffset[j] += gridSizeZ * 2.0f;
                    baseZ = zOffset[j];
                    buildingHeights[i][j] = 2.0f + static_cast<float>(rand()) /
                                                       (static_cast<float>(RAND_MAX / (8.0f - 2.0f)));
                }

                while (cameraPos.z - baseZ < -gridSizeZ)
                {
                    zOffset[j] -= gridSizeZ * 2.0f;
                    baseZ = zOffset[j];
                    buildingHeights[i][j] = 2.0f + static_cast<float>(rand()) /
                                                       (static_cast<float>(RAND_MAX / (8.0f - 2.0f)));
                }

                // Model transformation
                glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(baseX, buildingHeights[i][j] / 2.0f, baseZ));
                model = glm::scale(model, glm::vec3(1.0f, buildingHeights[i][j], 1.0f));

                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}
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
#include <string>
#include <chrono>
#include <thread>

struct InstanceData
{
    glm::vec3 position;
    glm::vec3 scale;
};

struct Car {
    glm::vec3 position;
    float speed;
    glm::vec3 color;
    bool movingForward;
    bool brakingLights;  // New: for brake lights effect
    float headlightIntensity; // New: for headlight glow
};


struct XWing {
    glm::vec3 position;
    glm::vec3 color;
};

// Global variables
int windowWidth = 1080;
int windowHeight = 800;
glm::mat4 projection;
glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

XWing xwing;
GLuint xwingVAO, xwingVBO;



const int NUM_STREETS = 15;  // Number of vertical streets
const int CARS_PER_STREET = 4;  // Number of cars per street
const float STREET_SPACING = 4.0f;  // Space between streets

glm::vec3 cameraPos(0.0f, 5.0f, 10.0f);
float cameraSpeed = 0.05f;                             // Reduced speed
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f); // Slight downward angle
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

const int gridSizeX = 60; // Number of buildings in X direction
const int gridSizeZ = 60; // Number of buildings in Z direction
float buildingHeights[gridSizeX][gridSizeZ];

float xOffset[gridSizeX]; // Track X offset for each column

GLuint skyboxVAO, skyboxVBO;
GLuint skyboxShaderProgram;
GLuint skyboxTexture;

const int MAX_INSTANCES = gridSizeX * gridSizeZ;

std::vector<InstanceData> instanceData(MAX_INSTANCES);
GLuint instanceVBO;


std::vector<Car> cars;
const int NUM_CARS = 10;
const float ROAD_LENGTH = 60.0f;  // Match  grid size * 2
const float CAR_SPACING = 20.0f;  // Minimum space between cars
GLuint carVAO, carVBO;

// For FPS calculation
double lastTime = 0.0;
int frameCount = 0;
double lastFPSUpdate = 0.0;
double currentFPS = 0.0;

// Camera parameters
float yaw = -90.0f; // Start looking forward (negative z)
float pitch = 0.0f; // Start looking horizontally
bool firstMouse = true;
float lastX = windowWidth / 2.0f;
float lastY = windowHeight / 2.0f;
const float mouseSensitivity = 0.01f;

GLuint roadVAO, roadVBO; // Declare these globally for road rendering
GLuint roadTexture;      // Declare the road texture globally


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
    // Back face
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,

    // Front face
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    // Left face
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,

    // Right face
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,

    // Bottom face
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,

    // Top face
    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f};

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
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl; 
        return 0;
    }
    stbi_image_free(data);
    return textureID;
}


void setupXWing() {
    // Simple spaceship vertices - sleek, arrow-like design
    float shipVertices[] = {
        // Main body - triangular shape
        // Top face
         0.0f,  0.1f, -0.6f,  // Front tip
        -0.2f,  0.1f,  0.3f,  // Left back
         0.2f,  0.1f,  0.3f,  // Right back

        // Bottom face
         0.0f, -0.1f, -0.6f,  // Front tip
        -0.2f, -0.1f,  0.3f,  // Left back
         0.2f, -0.1f,  0.3f,  // Right back

        // Side wings
        // Left wing
        -0.2f,  0.0f, -0.2f,  // Front
        -0.4f,  0.0f,  0.1f,  // Back
        -0.2f,  0.0f,  0.1f,  // Connect to body

        // Right wing
         0.2f,  0.0f, -0.2f,  // Front
         0.4f,  0.0f,  0.1f,  // Back
         0.2f,  0.0f,  0.1f,  // Connect to body

        // Rear stabilizer
         0.0f,  0.2f,  0.0f,  // Top
         0.0f,  0.0f,  0.3f,  // Back
         0.0f, -0.1f,  0.0f   // Bottom
    };

    glGenVertexArrays(1, &xwingVAO);
    glGenBuffers(1, &xwingVBO);

    glBindVertexArray(xwingVAO);
    glBindBuffer(GL_ARRAY_BUFFER, xwingVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shipVertices), shipVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Dark metallic color
    xwing.color = glm::vec3(0.3f, 0.3f, 0.35f);
    xwing.position = cameraPos - glm::vec3(0.0f, 0.5f, 4.0f);
}


// Setup OpenGL buffers
void setupOpenGL(GLuint &VAO, GLuint &VBO)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(VAO);

    // Setup vertex buffer
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

    // Set up instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(InstanceData), NULL, GL_DYNAMIC_DRAW);

    // Instance position attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1); // Tell OpenGL this is an instanced attribute

    // Instance scale attribute
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}



void updateXWing() {
    // Position X-wing directly in front of camera but slightly below
    float distance = 3.0f;  // Distance in front of camera
    
    // Calculate position directly in front of camera using cameraFront
    xwing.position = cameraPos + (cameraFront * distance) + glm::vec3(0.0f, -1.0f, 0.0f);  // -1.0f moves it down slightly
}

void renderXWing(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram);
    glBindVertexArray(xwingVAO);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, xwing.position);
    
    // First rotate 90 degrees around Y to face forward by default
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Then apply camera rotation
    model = glm::rotate(model, glm::radians(yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-pitch), glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Scale
    model = glm::scale(model, glm::vec3(0.7f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Main body
    glUniform3fv(glGetUniformLocation(shaderProgram, "carColor"), 1, glm::value_ptr(xwing.color));
    
    // Draw main body triangles
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Draw wings
    glDrawArrays(GL_TRIANGLES, 6, 6);
    
    // Draw stabilizer
    glDrawArrays(GL_TRIANGLES, 12, 3);

    glBindVertexArray(0);
}
void initializeCars() {
    cars.clear();
    
    // Create cars for each street
    for(int street = 0; street < NUM_STREETS; street++) {
        float streetX = -30.0f + (street * STREET_SPACING);  // X position of this street
        
        // Add cars to this street
        for(int i = 0; i < CARS_PER_STREET; i++) {
            Car car;
            // Distribute cars along the street length
            float startZ = -ROAD_LENGTH/2 + (i * (ROAD_LENGTH/CARS_PER_STREET));
            
            car.position = glm::vec3(streetX, 0.3f, startZ);
            car.speed = 0.02f + static_cast<float>(rand()) / RAND_MAX * 0.02f;  // Slower speed
            // Alternate direction based on street number
            car.movingForward = (street % 2 == 0);
            
            // Alternate colors for visual variety
            if (street % 3 == 0) {
                car.color = glm::vec3(0.8f, 0.2f, 0.2f);  // Red
            } else if (street % 3 == 1) {
                car.color = glm::vec3(0.2f, 0.2f, 0.8f);  // Blue
            } else {
                car.color = glm::vec3(0.8f, 0.8f, 0.8f);  // Silver
            }
            
            cars.push_back(car);
        }
    }
}


void setupCars() {
    // Car vertices (more realistic car shape)
    float carVertices[] = {
        // Main body - slightly curved top
        // Front face
        -0.3f, -0.1f, -0.6f,  // Bottom left
         0.3f, -0.1f, -0.6f,  // Bottom right
         0.3f,  0.2f, -0.6f,  // Top right
         0.3f,  0.2f, -0.6f,  // Top right
        -0.3f,  0.2f, -0.6f,  // Top left
        -0.3f, -0.1f, -0.6f,  // Bottom left

        // Back face
        -0.3f, -0.1f,  0.6f,
         0.3f, -0.1f,  0.6f,
         0.3f,  0.15f,  0.6f,  // Slightly lower for aerodynamic look
         0.3f,  0.15f,  0.6f,
        -0.3f,  0.15f,  0.6f,
        -0.3f, -0.1f,  0.6f,

        // Windshield (angled)
        -0.25f,  0.2f, -0.2f,
         0.25f,  0.2f, -0.2f,
         0.25f,  0.25f, -0.4f,
         0.25f,  0.25f, -0.4f,
        -0.25f,  0.25f, -0.4f,
        -0.25f,  0.2f, -0.2f,

        // Hood
        -0.25f,  0.15f, -0.6f,
         0.25f,  0.15f, -0.6f,
         0.25f,  0.15f, -0.4f,
         0.25f,  0.15f, -0.4f,
        -0.25f,  0.15f, -0.4f,
        -0.25f,  0.15f, -0.6f,

        // Roof
        -0.25f,  0.25f, -0.4f,
         0.25f,  0.25f, -0.4f,
         0.25f,  0.25f,  0.1f,
         0.25f,  0.25f,  0.1f,
        -0.25f,  0.25f,  0.1f,
        -0.25f,  0.25f, -0.4f,

        // Left side
        -0.3f,  0.2f,  0.6f,
        -0.3f,  0.2f, -0.6f,
        -0.3f, -0.1f, -0.6f,
        -0.3f, -0.1f, -0.6f,
        -0.3f, -0.1f,  0.6f,
        -0.3f,  0.2f,  0.6f,

        // Right side
         0.3f,  0.2f,  0.6f,
         0.3f,  0.2f, -0.6f,
         0.3f, -0.1f, -0.6f,
         0.3f, -0.1f, -0.6f,
         0.3f, -0.1f,  0.6f,
         0.3f,  0.2f,  0.6f,

        // Bottom
        -0.3f, -0.1f, -0.6f,
         0.3f, -0.1f, -0.6f,
         0.3f, -0.1f,  0.6f,
         0.3f, -0.1f,  0.6f,
        -0.3f, -0.1f,  0.6f,
        -0.3f, -0.1f, -0.6f,

        // Wheels (simplified as black boxes)
        // Front left wheel
        -0.35f, -0.1f, -0.4f,
        -0.35f,  0.0f, -0.4f,
        -0.35f,  0.0f, -0.2f,
        -0.35f, -0.1f, -0.2f,

        // Front right wheel
         0.35f, -0.1f, -0.4f,
         0.35f,  0.0f, -0.4f,
         0.35f,  0.0f, -0.2f,
         0.35f, -0.1f, -0.2f,

        // Back left wheel
        -0.35f, -0.1f,  0.4f,
        -0.35f,  0.0f,  0.4f,
        -0.35f,  0.0f,  0.2f,
        -0.35f, -0.1f,  0.2f,

        // Back right wheel
         0.35f, -0.1f,  0.4f,
         0.35f,  0.0f,  0.4f,
         0.35f,  0.0f,  0.2f,
         0.35f, -0.1f,  0.2f,
    };

    glGenVertexArrays(1, &carVAO);
    glGenBuffers(1, &carVBO);

    glBindVertexArray(carVAO);
    glBindBuffer(GL_ARRAY_BUFFER, carVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(carVertices), carVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void updateCars() {
    for(auto& car : cars) {
        float moveAmount = car.movingForward ? car.speed : -car.speed;
        car.position.z += moveAmount;

        // Wrap around when reaching the ends
        if(car.position.z > ROAD_LENGTH/2) {
            car.position.z = -ROAD_LENGTH/2;
        } else if(car.position.z < -ROAD_LENGTH/2) {
            car.position.z = ROAD_LENGTH/2;
        }
        
        // Check for nearby cars in same lane
        for(const auto& otherCar : cars) {
            if(&otherCar != &car && abs(car.position.x - otherCar.position.x) < 0.1f) { // Same lane
                float distance = abs(car.position.z - otherCar.position.z);
                if(distance < 2.0f) { // Too close
                    car.speed *= 0.95f; // Slow down
                    car.brakingLights = true;
                } else {
                    car.speed = 0.02f + static_cast<float>(rand()) / RAND_MAX * 0.02f; // Resume normal speed
                    car.brakingLights = false;
                }
            }
        }
    }
}


// Function to render cars
void renderCars(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram);
    glBindVertexArray(carVAO);

    for(const auto& car : cars) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, car.position);
        
        // Rotate car based on direction
        if (!car.movingForward) {
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "carColor"), 1, glm::value_ptr(car.color));
        glUniform1i(glGetUniformLocation(shaderProgram, "brakingLights"), car.brakingLights);
        glUniform1f(glGetUniformLocation(shaderProgram, "headlightIntensity"), car.headlightIntensity);
        glUniform1i(glGetUniformLocation(shaderProgram, "movingForward"), car.movingForward);

        // Draw main car body
        glDrawArrays(GL_TRIANGLES, 0, 48);  // Draw main body vertices

        // Draw wheels in black
        glm::vec3 wheelColor(0.1f, 0.1f, 0.1f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "carColor"), 1, glm::value_ptr(wheelColor));
        glDrawArrays(GL_TRIANGLE_FAN, 48, 16);  // Draw wheel vertices
    }

    glBindVertexArray(0);
}

void updateFPS(GLFWwindow *window)
{
    // Get current time
    double currentTime = glfwGetTime();
    frameCount++;

    // Update FPS every 0.5 seconds
    if (currentTime - lastFPSUpdate >= 0.5)
    {
        // Calculate FPS
        currentFPS = double(frameCount) / (currentTime - lastFPSUpdate);
        frameCount = 0;
        lastFPSUpdate = currentTime;

        // Update window title with FPS
        std::string title = "CyberDublin | FPS: " + std::to_string(static_cast<int>(currentFPS));
        glfwSetWindowTitle(window, title.c_str());
    }
}


void setupRoad() {
    std::vector<float> roadVertices;

    // Create road vertices for each street
    for (int i = 0; i < NUM_STREETS; i++) {
        float xPos = -30.0f + (i * STREET_SPACING);

        // Define vertices with texture coordinates
        float streetVerts[] = {
            // Positions          // Texture Coords
            xPos - 0.5f, 0.0f, -30.0f,  0.0f, 0.0f,  // Bottom-left
            xPos + 0.5f, 0.0f, -30.0f,  1.0f, 0.0f,  // Bottom-right
            xPos + 0.5f, 0.0f,  30.0f,  1.0f, 30.0f, // Top-right
            
            xPos - 0.5f, 0.0f, -30.0f,  0.0f, 0.0f,  // Bottom-left
            xPos + 0.5f, 0.0f,  30.0f,  1.0f, 30.0f, // Top-right
            xPos - 0.5f, 0.0f,  30.0f,  0.0f, 30.0f  // Top-left
        };

        roadVertices.insert(roadVertices.end(), std::begin(streetVerts), std::end(streetVerts));
    }

    // Create and bind VAO and VBO for roads
    glGenVertexArrays(1, &roadVAO);
    glGenBuffers(1, &roadVBO);

    glBindVertexArray(roadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
    glBufferData(GL_ARRAY_BUFFER, roadVertices.size() * sizeof(float), roadVertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Load road texture
    roadTexture = loadTexture("../assets/road.jpg");

    glBindVertexArray(0);
}


void renderRoad(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram);

    // Set transformation matrices
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Bind road texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, roadTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "roadTexture"), 0);

    // Render the road
    glBindVertexArray(roadVAO);
    glDrawArrays(GL_TRIANGLES, 0, NUM_STREETS * 6);
    glBindVertexArray(0);
}

void setupSkybox()
{
    // Generate and bind VAO and VBO for skybox
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    // Load skybox shader
    skyboxShaderProgram = compileShader("../shaders/skybox_vertex_shader.glsl",
                                        "../shaders/skybox_fragment_shader.glsl");

    // Change the path to match the building texture path format
    skyboxTexture = loadTexture("../assets/sky.jpg"); // Changed from "./assets/sky.jpg"

    if (skyboxTexture == 0)
    {
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


void renderSkybox(const glm::mat4 &view, const glm::mat4 &projection)
{
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
    glDrawArrays(GL_TRIANGLES, 0, 36); // Updated vertex count
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Update camera front vector
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
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
    float moveSpeed = 0.01f;
    float strafeSpeed = 0.01f;

    // Forward/Backward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += moveSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= moveSpeed * cameraFront;

    // Left/Right strafe
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * strafeSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * strafeSpeed;

    // Up/Down movement
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraUp * moveSpeed;

    // Escape to close
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
    // Mouse capture and callback
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
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
    roadTexture = loadTexture("../assets/road.jpg");
    GLuint carShaderProgram = compileShader("../shaders/car_vertex_shader.glsl", 
                                          "../shaders/car_fragment_shader.glsl");

    setupRoad();
    setupCars();
    setupXWing();

    initializeCars();



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
        updateXWing();
        
        updateCars();

        renderSkybox(view, projection);
        // Use shader program
        glUseProgram(shaderProgram);

        renderRoad(shaderProgram, view, projection);

        // Pass transformation matrices to the shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

        // Bind the texture
        glBindTexture(GL_TEXTURE_2D, texture1);
        glBindVertexArray(VAO);

        int instanceCount = 0;

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

                // Update instance data
                instanceData[instanceCount].position = glm::vec3(baseX, buildingHeights[i][j] / 2.0f, baseZ);
                instanceData[instanceCount].scale = glm::vec3(1.0f, buildingHeights[i][j], 1.0f);
                instanceCount++;
            }
        }

        // Update instance buffer
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(InstanceData), &instanceData[0]);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, instanceCount);

        renderCars(carShaderProgram, view, projection);
        renderXWing(carShaderProgram, view, projection);

        updateFPS(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteVertexArrays(1, &roadVAO);
    glDeleteBuffers(1, &roadVBO);

    glfwTerminate();
    return 0;
}

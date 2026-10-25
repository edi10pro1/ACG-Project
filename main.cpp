#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmath> // Necesar pentru sin()

// Include bibliotecile adaptate în funcție de sistemul de operare
#ifdef __APPLE__
    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtc/type_ptr.hpp>
#else
    // Include GLEW (Windows)
    #include "dependente\glew\glew.h"

    // Include GLFW (Windows)
    #include "dependente\glfw\glfw3.h"

    // Include GLM (Windows)
    #include "dependente\glm\glm.hpp"
    #include "dependente\glm\gtc\matrix_transform.hpp"
    #include "dependente\glm\gtc\type_ptr.hpp"
#endif

#include "shader.hpp"

// Variables
GLFWwindow* window;
const int width = 1024, height = 1024;

// Window resizing callback
void window_callback(GLFWwindow* window, int new_width, int new_height)
{
    if (new_width <= 0 || new_height <= 0)
    {
        return;
    }

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = new_width;
    int viewportHeight = new_height;

    if (new_width > new_height)
    {
        viewportWidth = new_height;
        viewportX = (new_width - viewportWidth) / 2;
    }
    else if (new_height > new_width)
    {
        viewportHeight = new_width;
        viewportY = (new_height - viewportHeight) / 2;
    }

    glViewport(
        viewportX,
        viewportY,
        viewportWidth,
        viewportHeight);
}

int main(void)
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Setări obligatorii pe macOS pentru OpenGL Core Profile
    #ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #endif

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(width, height, "Dragon", NULL, NULL);

    if (window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window.");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true;

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        return -1;
    }

    // Set viewport
    glViewport(0, 0, width, height);

    // Dark blue background (cerul)
    glClearColor(0.0f, 0.0f, 0.4f, 1.0f);

    // Create and compile shaders
    GLuint programID = LoadShaders(
        "SimpleVertexShader.vertexshader",
        "SimpleFragmentShader.fragmentshader"
    );

    // Ground vertices (coborât la -0.9f pentru un zoom out mai mare)
    GLfloat groundVertices[] = {
        -1.0f, -0.9f, 0.0f,  
         1.0f, -0.9f, 0.0f, 
         1.0f, -1.0f, 0.0f,  
        -1.0f, -1.0f, 0.0f   
    };

    GLuint groundIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint groundVBO, groundVAO, groundIBO;

    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    glGenBuffers(1, &groundIBO);

    glBindVertexArray(groundVAO);

    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Dragon vertices
    GLfloat vertices[] = {
        // Body
        -0.2f, -0.05f, 0.0f,  // 0
         0.0f, -0.05f, 0.0f,  // 1
         0.6f, -0.05f, 0.0f,  // 2
         0.6f,  0.1f,  0.0f,  // 3
         0.0f,  0.1f,  0.0f,  // 4

         // Head
          0.6f, -0.1f,  0.0f,  // 5
          0.9f,  0.0f,  0.0f,  // 6
          0.6f,  0.2f,  0.0f,  // 7
          0.55f, 0.1f,  0.0f,  // 8
          0.6f,  0.15f, 0.0f,  // 9
          0.5f,  0.2f,  0.0f,  // 10
          0.7f, -0.1f,  0.0f,  // 11
          0.85f,-0.1f,  0.0f,  // 12
          0.85f,-0.05f, 0.0f,  // 13
          0.7f, -0.05f, 0.0f,  // 14

          // Legs
           0.0f, -0.15f, 0.0f,  // 15
           0.1f, -0.15f, 0.0f,  // 16
           0.1f, -0.05f, 0.0f,  // 17
           0.3f, -0.15f, 0.0f,  // 18
           0.4f, -0.15f, 0.0f,  // 19
           0.4f, -0.05f, 0.0f,  // 20
           0.3f, -0.05f, 0.0f,  // 21

           // Tail
           -0.12f, 0.0f,  0.0f,  // 22
           -0.2f,  0.0f,  0.0f,  // 23
           -0.4f, -0.05f, 0.0f,  // 24
           -0.4f,  0.0f,  0.0f,  // 25
           -0.6f, -0.2f,  0.0f,  // 26
           -0.6f, -0.15f, 0.0f,  // 27
           -0.8f, -0.2f,  0.0f,  // 28
           -0.8f, -0.15f, 0.0f,  // 29
           -1.0f, -0.2f,  0.0f,  // 30
           -0.9f, -0.175f,0.0f,  // 31
           -0.9f,  0.0f,  0.0f,  // 32

           // Wings
           -0.1f,  0.025f,0.0f,  // 33
           -0.1f,  0.2f,  0.0f,  // 34
            0.1f,  0.3f,  0.0f,  // 35
            0.0f,  0.4f,  0.0f,  // 36
            0.3f,  0.1f,  0.0f,  // 37
            0.3f,  0.4f,  0.0f,  // 38
            0.4f,  0.1f,  0.0f,  // 39
            0.3f,  0.2f,  0.0f,  // 40
            0.0f,  0.7f,  0.0f,  // 41
           -0.3f,  0.7f,  0.0f,  // 42
           -0.1f,  0.5f,  0.0f,  // 43
           -0.3f,  0.5f,  0.0f,  // 44
           -0.5f,  0.7f,  0.0f,  // 45
           -0.2f,  0.7f,  0.0f,  // 46
           -0.5f,  1.0f,  0.0f,  // 47
           -0.8f,  1.0f,  0.0f,  // 48
           -0.5f,  0.8f,  0.0f   // 49
    };

    // Salvăm o copie a vârfurilor originale pentru animația capului/cozii
    GLfloat initialVertices[sizeof(vertices) / sizeof(GLfloat)];
    for(int i = 0; i < sizeof(vertices) / sizeof(GLfloat); i++) {
        initialVertices[i] = vertices[i];
    }

    // Dragon indices
    GLuint indices[] = {
        // Body (9)
        1, 2, 3,
        3, 4, 1,
        0, 1, 4,

        // Head (15)
        5, 6, 7,
        8, 3, 9,
        8, 9, 10,
        11, 12, 13,
        13, 14, 11,

        // Leg 1 (6)
        15, 16, 17,
        17, 1, 15,

        // Leg 2 (6)
        18, 19, 20,
        20, 21, 18,

        // Tail (27)
        22, 23, 0,
        24, 0, 23,
        23, 25, 24,
        25, 27, 26,
        26, 24, 25,
        28, 26, 27,
        27, 29, 28,
        30, 28, 29,
        31, 29, 32,

        // Triunghiuri extra în corpul principal (3)
        33, 4, 34,

        // Aripi animate (30)
        4, 37, 35,
        37, 38, 36,
        37, 39, 40,
        36, 38, 41,
        36, 41, 42,
        44, 36, 43,
        44, 43, 42,
        45, 44, 42,
        45, 46, 47,
        47, 48, 49
    };

    // VAO, VBO, IBO pentru dragon
    GLuint vbo, vao, ibo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Window resize callback
    glfwSetFramebufferSizeCallback(window, window_callback);

    // Poziția și nivelul minim actualizat pentru noul sol la -0.9f (scală 0.25f)
    float dragonX = 0.0f;
    float dragonY = -0.85f;         
    const float minGroundY = -0.85f; 
    const float flightSpeed = 1.0f;  
    const float fallSpeed = 0.5f;
    const float glideSpeed = 1.2f;

    // Unghiul dragonului
    float dragonAngle = 0.0f;

    // Controlul aripilor
    float wingFlapSpeed = 5.0f;
    float wingPhase = 0.0f; 

    float lastTime = (float)glfwGetTime();

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Close window with ESC
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // Verificăm starea tastelor A, D și SPACE
        bool isAPressed = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
        bool isDPressed = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
        bool isSpacePressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

        // Control direcție (A / D)
        if (isAPressed)
        {
            dragonAngle = glm::radians(180.0f); // Stânga
        }
        if (isDPressed)
        {
            dragonAngle = glm::radians(0.0f);   // Dreapta
        }

        // Control zbor și viteză aripi
        float targetWingFlapSpeed = 5.0f; 

        if (isSpacePressed)
        {
            dragonY += flightSpeed * deltaTime;
            targetWingFlapSpeed = 20.0f; 
        }
        
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        {
            dragonY -= flightSpeed * deltaTime;
            if (dragonY < minGroundY) {
                dragonY = minGroundY; 
            }
        }
        
        if (dragonY > minGroundY)
        {
            dragonY -= fallSpeed * deltaTime;
        }
        else 
        {
            dragonY = minGroundY;
        }

        // Logica de deplasare orizontală
        bool shouldMoveHorizontal = false;
        if (dragonY > minGroundY)
        {
            if (!isSpacePressed)
            {
                shouldMoveHorizontal = true; 
            }
            else
            {
                if (isAPressed || isDPressed)
                {
                    shouldMoveHorizontal = true; 
                }
            }
        }

        if (shouldMoveHorizontal)
        {
            float moveDirection = cos(dragonAngle); 
            dragonX += moveDirection * glideSpeed * deltaTime;

            if (dragonX > 1.5f) dragonX = -1.5f;
            if (dragonX < -1.5f) dragonX = 1.5f;
        }

        // Tranziție lină viteză aripi
        float smoothingFactor = 6.0f;
        wingFlapSpeed += (targetWingFlapSpeed - wingFlapSpeed) * smoothingFactor * deltaTime;
        wingPhase += wingFlapSpeed * deltaTime; 

        // ==========================================
        // ANIMAȚIE CAP ȘI COADĂ
        // ==========================================
        for(int i = 0; i < sizeof(vertices) / sizeof(GLfloat); i++) {
            vertices[i] = initialVertices[i];
        }

        // Cap
        for (int i = 5; i <= 14; i++) {
            float origY = initialVertices[i * 3 + 1];
            float origX = initialVertices[i * 3];
            if (origX > 0.6f) {
                vertices[i * 3 + 1] = origY + sin(currentTime * 4.0f) * 0.03f;
            }
        }

        // Amplitudine coadă
        float tailAmplitude = 0.01f; 
        if (dragonY > minGroundY) {
            tailAmplitude = 0.04f; 
        }
        if (isSpacePressed) {
            tailAmplitude = 0.07f; 
        }

        // Coadă
        for (int i = 22; i <= 32; i++) {
            float origX = initialVertices[i * 3];     
            float origY = initialVertices[i * 3 + 1];
            float tailFactor = abs(origX + 0.12f); 
            vertices[i * 3 + 1] = origY + sin(currentTime * 7.0f + origX * 4.0f) * tailAmplitude * tailFactor;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        
        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(programID);

        unsigned int transformLoc = glGetUniformLocation(programID, "transform");
        unsigned int colorLoc = glGetUniformLocation(programID, "color");

        // ==========================================
        // 1. DESENARE SOL
        // ==========================================
        glBindVertexArray(groundVAO);

        glm::vec4 groundColor(0.1f, 0.6f, 0.1f, 1.0f);
        glUniform4fv(colorLoc, 1, glm::value_ptr(groundColor));

        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(groundModel));

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ==========================================
        // 2. DESENARE CORP DRAGON (Zoom out mărit la 0.25f)
        // ==========================================
        glBindVertexArray(vao);

        glm::vec4 dragonColor(0.5f, 0.0f, 0.5f, 1.0f);
        glUniform4fv(colorLoc, 1, glm::value_ptr(dragonColor));

        glm::mat4 dragonModel = glm::translate(glm::mat4(1.0f), glm::vec3(dragonX, dragonY, 0.0f));
        dragonModel = glm::rotate(dragonModel, dragonAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        dragonModel = glm::scale(dragonModel, glm::vec3(0.25f, 0.25f, 1.0f)); // Zoom out mai mare (dragon mai mic)

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(dragonModel));

        glDrawElements(GL_TRIANGLES, 66, GL_UNSIGNED_INT, (void*)0);

        // ==========================================
        // 3. DESENARE ARIPI ANIMATE
        // ==========================================
        float flapAngle = sin(wingPhase) * 0.70f - 5.0f; 

        glm::mat4 wingsModel = dragonModel; 
        wingsModel = glm::rotate(wingsModel, flapAngle, glm::vec3(1.0f, 0.0f, 0.0f));

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(wingsModel));
        glDrawElements(GL_TRIANGLES, 30, GL_UNSIGNED_INT, (void*)(66 * sizeof(GLuint)));

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteVertexArrays(1, &vao);
    
    glDeleteBuffers(1, &groundVBO);
    glDeleteBuffers(1, &groundIBO);
    glDeleteVertexArrays(1, &groundVAO);

    glDeleteProgram(programID);

    glfwTerminate();

    return 0;
}
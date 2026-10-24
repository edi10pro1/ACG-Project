
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Include GLEW
#include "dependente\glew\glew.h"

// Include GLFW
#include "dependente\glfw\glfw3.h"

// Include GLM
#include "dependente\glm\glm.hpp"
#include "dependente\glm\gtc\matrix_transform.hpp"
#include "dependente\glm\gtc\type_ptr.hpp"

#include "shader.hpp"

// Variables
GLFWwindow* window;
const int width = 1024, height = 1024;

/*
// Variables for multiple squares and movement
const int positionCount = 10;
int currentPosition = 0;
glm::vec3 squareOffset(0.0f, 0.0f, 0.0f);
float zoomScale = 1.0f;
*/

/*
// Handling cursor position
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    std::cout << "The mouse cursor is: "
              << xpos << " " << ypos << std::endl;
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        std::cout << "Left mouse button pressed" << std::endl;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        std::cout << "Right mouse button pressed" << std::endl;
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        currentPosition = (currentPosition + 1) % positionCount;

        std::cout << "Current position: "
                  << currentPosition << std::endl;
    }
}
*/

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

/*
// Scroll callback for zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    zoomScale += (float)yoffset * 0.1f;

    if (zoomScale < 0.2f)
        zoomScale = 0.2f;

    if (zoomScale > 3.0f)
        zoomScale = 3.0f;
}
*/

int main(void)
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

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

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 1.0f);

    // Create and compile shaders
    GLuint programID = LoadShaders(
        "SimpleVertexShader.vertexshader",
        "SimpleFragmentShader.fragmentshader"
    );

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

    // Dragon indices
    GLuint indices[] = {
        // Body
        1, 2, 3,
        3, 4, 1,
        0, 1, 4,

        // Head
        5, 6, 7,
        8, 3, 9,
        8, 9, 10,
        11, 12, 13,
        13, 14, 11,

        // Leg 1
        15, 16, 17,
        17, 1, 15,

        // Leg 2
        18, 19, 20,
        20, 21, 18,

        // Tail
        22, 23, 0,
        24, 0, 23,
        23, 25, 24,
        25, 27, 26,
        26, 24, 25,
        28, 26, 27,
        27, 29, 28,
        30, 28, 29,
        31, 29, 32,

        // Extra triangle for wings
        33, 4, 34,

        // Wings
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

    // VAO, VBO, IBO
    GLuint vbo, vao, ibo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // Bind IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    // Set attribute pointers
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    /*
    // Original square positions
    glm::vec3 positions[] = {
        glm::vec3(0.0f,  0.0f,  0),
        glm::vec3(0.2f,  0.5f, 0),
        glm::vec3(-0.15f, -0.22f, 0),
        glm::vec3(-0.38f, -0.2f, 0),
        glm::vec3(0.24f, -0.4f, 0),
        glm::vec3(-0.17f,  0.3f, 0),
        glm::vec3(0.23f, -0.2f, 0),
        glm::vec3(0.15f,  0.2f, 0),
        glm::vec3(0.15f,  0.7f, 0),
        glm::vec3(-0.13f,  0.1f, 0)
    };
    */

    // Window resize callback
    glfwSetFramebufferSizeCallback(window, window_callback);

    /*
    // Original callbacks
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    const float rotationSpeed = 90.0f;
    const float movementSpeed = 0.75f;
    float lastTime = (float)glfwGetTime();
    */

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Close window with ESC
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        /*
        // Original movement and rotation controls
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            squareOffset.y += movementSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            squareOffset.y -= movementSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            squareOffset.x -= movementSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            squareOffset.x += movementSpeed * deltaTime;

        bool leftButtonPressed =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        bool rightButtonPressed =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        float rotationDirection = 0.0f;

        if (leftButtonPressed)
            rotationDirection += 1.0f;

        if (rightButtonPressed)
            rotationDirection -= 1.0f;

        if (rotationDirection != 0.0f)
        {
            float rotationAngle =
                rotationSpeed * deltaTime * rotationDirection;

            trans = glm::rotate(
                trans,
                rotationAngle,
                glm::vec3(0.0f, 0.0f, 1.0f));
        }
        */

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader
        glUseProgram(programID);

        // Bind VAO
        glBindVertexArray(vao);

        // Get uniform locations
        unsigned int transformLoc =
            glGetUniformLocation(programID, "transform");

        unsigned int transformLoc2 =
            glGetUniformLocation(programID, "color");

        // Dragon color
        glm::vec4 color =
            glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);

        glUniform4fv(
            transformLoc2,
            1,
            glm::value_ptr(color));

        // Scale dragon to fit inside window
        glm::mat4 model =
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(0.8f, 0.8f, 1.0f));

        glUniformMatrix4fv(
            transformLoc,
            1,
            GL_FALSE,
            glm::value_ptr(model));

        /*
        // Original multiple-square rendering
        glm::mat4 groupTransform =
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(
                    zoomScale,
                    zoomScale,
                    1.0f));

        for (int i = 0; i < positionCount; i++)
        {
            int positionIndex = i;

            if (i == 0)
                positionIndex = currentPosition;

            glm::vec3 position = positions[positionIndex];

            if (i == 0)
                position += squareOffset;

            glm::mat4 localTransform =
                glm::translate(
                    glm::mat4(1.0f),
                    position) * trans;

            glm::mat4 model =
                groupTransform * localTransform;

            glUniformMatrix4fv(
                transformLoc,
                1,
                GL_FALSE,
                glm::value_ptr(model));

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        */

        // Draw the dragon
        glDrawElements(
            GL_TRIANGLES,
            sizeof(indices) / sizeof(indices[0]),
            GL_UNSIGNED_INT,
            0
        );

        // Swap buffers after drawing
        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(programID);

    glfwTerminate();

    return 0;
}

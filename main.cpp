#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>
#include <fstream>

#ifdef __APPLE__
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#else
#define GLM_FORCE_RADIANS
#include "dependente\glew\glew.h"
#include "dependente\glfw\glfw3.h"
#include "dependente\glm\glm.hpp"
#include "dependente\glm\gtc\matrix_transform.hpp"
#include "dependente\glm\gtc\type_ptr.hpp"
#endif

#include "shader.hpp"

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 1024;

const float PI = 3.14159265359f;
const float GROUND_Y = -0.9f;

const int OBSTACLES_PER_ROUND = 20;
const int MAX_PLAYER_FIREBALLS = 3;
const int MAX_BOSS_FIREBALLS = 5;

GLFWwindow* window = nullptr;

float worldHalfWidth = 1.0f;
float worldHalfHeight = 1.0f;

glm::mat4 projection(1.0f);

std::mt19937 rng(std::random_device{}());

float randomFloat(float low, float high)
{
    std::uniform_real_distribution<float> dist(low, high);
    return dist(rng);
}

// WINDOW AND PROJECTION

void updateProjection(int w, int h)
{
    if (w <= 0 || h <= 0)
        return;

    glViewport(0, 0, w, h);

    float aspect = (float)w / (float)h;

    worldHalfWidth = std::max(1.0f, aspect);
    worldHalfHeight = std::max(1.0f, 1.0f / aspect);

    projection = glm::ortho(
        -worldHalfWidth, worldHalfWidth,
        -worldHalfHeight, worldHalfHeight,
        -1.0f, 1.0f
    );
}

// SCORE LOADING AND SAVING

int loadHighScore() {
    int highScore = 0;
    std::ifstream file("highscore.txt");
    
    if (file.is_open()) {
        file >> highScore; 
        file.close();
    } else {
       // daca fisierul nu exista creaza unul
        std::ofstream outFile("highscore.txt");
        outFile << 0;
        outFile.close();
    }
    
    return highScore;
}

void window_callback(GLFWwindow*, int w, int h)
{
    updateProjection(w, h);
}

// MESH (to simplify the code a bit, we will use a struct to store mesh data)

struct Mesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei count = 0;

    void create(
        const GLfloat vertices[],
        size_t vertexCount,
        const GLuint indices[],
        size_t indexCount)
    {
        count = (GLsizei)indexCount;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            vertexCount * sizeof(GLfloat),
            vertices,
            GL_DYNAMIC_DRAW 
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indexCount * sizeof(GLuint),
            indices,
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(GLfloat), (void*)0
        );

        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void updateVertices(const GLfloat vertices[], size_t vertexCount)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(GLfloat), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void destroy()
    {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    }
};

// GAME OBJECTS (to work with them a bit easier, we will use structs)

struct Obstacle
{
    float x = 0.0f;
    float height = 0.0f;
    bool hit = false;
    float fallAngle = 0.0f;
    bool counted = false;
};

struct Fireball
{
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 velocity = glm::vec2(0.0f);
    bool active = false;
};

struct Dragon
{
    float x = -0.35f;
    float y = -0.865f;
    float direction = 1.0f;

    int lives = 5;

    float hitTimer = 0.0f;
    float invulnerableTimer = 0.0f;
    float shootCooldown = 0.0f;

    float wingPhase = 0.0f;
    float wingSpeed = 5.0f;
};

struct Boss
{
    bool active = false;
    int lives = 5;

    float x = 0.6f;
    float y = 0.05f;

    float time = 0.0f;
    float shootTimer = 1.5f;
    float hitFlashTimer = 0.0f;
    float invulnerableTimer = 0.0f;
};

enum class GamePhase
{
    OBSTACLES,
    BOSS,
    GAME_OVER
};

Dragon dragon;
Boss boss;

GamePhase gamePhase = GamePhase::OBSTACLES;

Obstacle obstacles[OBSTACLES_PER_ROUND];
Fireball playerFireballs[MAX_PLAYER_FIREBALLS];
Fireball bossFireballs[MAX_BOSS_FIREBALLS];

int obstacleCount = 0;
int obstaclesPassed = 0;
int bossesDefeated = 0;
int score = 0;

bool highScore = false;

// OPENGL OBJECTS

Mesh dragonBodyMesh;
Mesh dragonWingMesh;

Mesh bossBodyMesh;
Mesh bossWingMesh;

Mesh quadMesh;
Mesh fireMesh;

GLuint programID;
GLint transformLoc;
GLint colorLoc;

// PLAYER DRAGON - VERTICES AND INDICES (Copie inițială pentru resetarea animației de vertebre)

GLfloat initialDragonVertices[] = {
    -0.2f,-0.05f,0.0f,   // 0
     0.0f,-0.05f,0.0f,   // 1
     0.6f,-0.05f,0.0f,   // 2
     0.6f, 0.1f,0.0f,    // 3
     0.0f, 0.1f,0.0f,    // 4
     0.6f,-0.1f,0.0f,    // 5
     0.9f, 0.0f,0.0f,    // 6
     0.6f, 0.2f,0.0f,    // 7
     0.55f,0.1f,0.0f,    // 8
     0.6f, 0.15f,0.0f,   // 9
     0.5f, 0.2f,0.0f,    // 10
     0.7f,-0.1f,0.0f,    // 11
     0.85f,-0.1f,0.0f,   // 12
     0.85f,-0.05f,0.0f,  // 13
     0.7f,-0.05f,0.0f,   // 14
     0.0f,-0.15f,0.0f,   // 15
     0.1f,-0.15f,0.0f,   // 16
     0.1f,-0.05f,0.0f,   // 17
     0.3f,-0.15f,0.0f,   // 18
     0.4f,-0.15f,0.0f,   // 19
     0.4f,-0.05f,0.0f,   // 20
     0.3f,-0.05f,0.0f,   // 21
    -0.12f,0.0f,0.0f,    // 22
    -0.2f, 0.0f,0.0f,    // 23
    -0.4f,-0.05f,0.0f,   // 24
    -0.4f, 0.0f,0.0f,    // 25
    -0.6f,-0.2f,0.0f,    // 26
    -0.6f,-0.15f,0.0f,   // 27
    -0.8f,-0.2f,0.0f,    // 28
    -0.8f,-0.15f,0.0f,   // 29
    -1.0f,-0.2f,0.0f,    // 30
    -0.9f,-0.175f,0.0f,  // 31
    -0.9f, 0.0f,0.0f,    // 32
    -0.1f, 0.025f,0.0f,  // 33
    -0.1f, 0.2f,0.0f,    // 34
     0.1f, 0.3f,0.0f,    // 35
     0.0f, 0.4f,0.0f,    // 36
     0.3f, 0.1f,0.0f,    // 37
     0.3f, 0.4f,0.0f,    // 38
     0.4f, 0.1f,0.0f,    // 39
     0.3f, 0.2f,0.0f,    // 40
     0.0f, 0.7f,0.0f,    // 41
    -0.3f, 0.7f,0.0f,    // 42
    -0.1f, 0.5f,0.0f,    // 43
    -0.3f, 0.5f,0.0f,    // 44
    -0.5f, 0.7f,0.0f,    // 45
    -0.2f, 0.7f,0.0f,    // 46
    -0.5f, 1.0f,0.0f,    // 47
    -0.8f, 1.0f,0.0f,    // 48
    -0.5f, 0.8f,0.0f     // 49
};

GLfloat currentDragonVertices[sizeof(initialDragonVertices) / sizeof(GLfloat)];

GLuint dragonBodyIndices[] = {
    1,2,3, 
    3,4,1, 
    0,1,4,
    5,6,7, 
    8,3,9, 
    8,9,10,
    11,12,13, 
    13,14,11,
    15,16,17, 
    17,1,15,
    18,19,20,
    20,21,18,
    22,23,0, 
    24,0,23,
    23,25,24, 
    25,27,26,
    26,24,25, 
    28,26,27,
    27,29,28, 
    30,28,29,
    31,29,32,
    33,4,34
};

GLuint dragonWingIndices[] = {
    4,37,35,
    37,38,36,
    37,39,40,
    36,38,41,
    36,41,42,
    44,36,43,
    44,43,42,
    45,44,42,
    45,46,47,
    47,48,49
};

// BOSS DRAGON - VERTICES

GLfloat bossVertices[] = {
    -0.8f,  0.6f, 0.0f,   // 0
    -0.6f,  0.6f, 0.0f,   // 1
    -0.6f,  0.7f, 0.0f,   // 2
    -0.6f,  0.8f, 0.0f,   // 3
    -0.5f,  0.8f, 0.0f,   // 4
    -0.2f,  0.6f, 0.0f,   // 5
     0.0f,  0.6f, 0.0f,   // 6

    -0.8f,  0.1f, 0.0f,   // 7
    -0.7f,  0.1f, 0.0f,   // 8
    -0.2f,  0.1f, 0.0f,   // 9
    -0.8f,  0.0f, 0.0f,   // 10
    -0.7f,  0.0f, 0.0f,   // 11
    -0.2f,  0.0f, 0.0f,   // 12

     0.0f,  0.0f, 0.0f,   // 13
     0.08f, 0.46f,0.0f,  // 14

     0.12f,-0.1f, 0.0f,   // 15
    -0.1f, -0.1f, 0.0f,   // 16
     0.0f, -0.1f, 0.0f,   // 17

     0.3f, -0.24f,0.0f,  // 18
     0.4f, -0.3f, 0.0f,   // 19
     0.2f, -0.3f, 0.0f,   // 20
     0.3f, -0.3f, 0.0f,   // 21

     0.5f,  0.8f, 0.0f,   // 22
     0.43f, 0.4f, 0.0f,   // 23
     0.8f,  0.5f, 0.0f,   // 24
     0.4f,  0.0f, 0.0f,   // 25
     0.7f, -0.4f, 0.0f,   // 26

     0.4f, -0.4f, 0.0f,   // 27
     0.4f, -0.7f, 0.0f,   // 28
     0.1f, -0.9f, 0.0f,   // 29
     0.32f,-0.83f,0.0f,  // 30
     0.3f, -1.0f, 0.0f,   // 31

     0.45f,-0.1f, 0.0f,   // 32
     0.5f,  0.0f, 0.0f    // 33
};

GLuint bossBodyIndices[] = {
    0, 7, 9,
    0, 9, 5,
    5, 9, 12,
    5, 12, 6,
    6, 12, 13,

    0, 1, 3,
    3, 2, 4,

    7, 8, 11,
    7, 11, 10,
    8, 9, 12,
    8, 12, 11,

    13, 17, 15,

    6, 14, 13,
    14, 25, 13,

    13, 25, 19,
    13, 19, 18,

    25, 33, 32,

    25, 26, 19,

    19, 26, 28,
    19, 28, 29,

    29, 30, 31
};

GLuint bossWingIndices[] = {
    14, 22, 23,
    22, 24, 23,
    14, 23, 25
};

// SIMPLE SHAPES FOR OBSTACLES AND FIREBALLS
GLfloat quadVertices[] = {
    -0.5f,0.0f,0.0f,
     0.5f,0.0f,0.0f,
     0.5f,1.0f,0.0f,
    -0.5f,1.0f,0.0f
};

GLuint quadIndices[] = {
    0,1,2,
    2,3,0
};

GLfloat fireVertices[] = {
    -0.6f,-0.25f,0.0f,
     0.5f, 0.0f,0.0f,
    -0.6f, 0.25f,0.0f
};

GLuint fireIndices[] = {
    0,1,2
};

// OBSTACLE SYSTEM

void resetObstacles()
{
    obstacleCount = OBSTACLES_PER_ROUND;

    for (int i = 0; i < obstacleCount; i++)
    {
        obstacles[i] = Obstacle();

        obstacles[i].x =
            worldHalfWidth + 0.8f + i * 1.2f;

        obstacles[i].height =
            randomFloat(0.2f, 1.2f);
    }

    obstaclesPassed = 0;
    gamePhase = GamePhase::OBSTACLES;
}

// BOSS SYSTEM

void spawnBoss()
{
    gamePhase = GamePhase::BOSS;
    obstacleCount = 0;

    boss = Boss();
    boss.active = true;
    boss.lives = 5;

    boss.x = worldHalfWidth - 0.42f;
    boss.y = 0.05f;

    boss.shootTimer = randomFloat(1.0f, 2.5f);

    std::cout << "BOSS APPEARED!\n";
}

void defeatBoss()
{
    boss.active = false;
    bossesDefeated++;

    std::cout << "BOSS DEFEATED!\n";

    for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
        bossFireballs[i].active = false;

    resetObstacles();
}

// DAMAGE CALCULATION

void damageDragon()
{
    if (dragon.lives <= 0 ||
        dragon.invulnerableTimer > 0.0f)
        return;

    dragon.lives--;

    dragon.hitTimer = 0.6f;
    dragon.invulnerableTimer = 1.0f;

    std::cout << "Dragon lives: "
        << dragon.lives << "\n";

    if (dragon.lives == 0)
    {
        gamePhase = GamePhase::GAME_OVER;
        std::cout << "GAME OVER!\n";
    }
}

void damageBoss()
{
    if (!boss.active ||
        boss.invulnerableTimer > 0.0f)
        return;

    boss.lives--;

    boss.hitFlashTimer = 0.35f;
    boss.invulnerableTimer = 0.45f;

    std::cout << "Boss lives: "
        << boss.lives << "\n";

    if (boss.lives <= 0)
        defeatBoss();
}

// SHOOTING SYSTEM

void shootPlayerFireball()
{
    if (dragon.shootCooldown > 0.0f)
        return;

    for (int i = 0; i < MAX_PLAYER_FIREBALLS; i++)
    {
        if (playerFireballs[i].active)
            continue;

        playerFireballs[i].active = true;

        playerFireballs[i].position = glm::vec2(
            dragon.x + dragon.direction * 0.20f,
            dragon.y + 0.025f
        );

        playerFireballs[i].velocity = glm::vec2(
            dragon.direction * 3.5f,
            0.0f
        );

        dragon.shootCooldown = 0.45f;
        break;
    }
}

void shootBossFireball()
{
    for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
    {
        if (bossFireballs[i].active)
            continue;

        bossFireballs[i].position = glm::vec2(
            boss.x - 0.24f,
            boss.y + 0.08f
        );

        glm::vec2 target(
            dragon.x,
            dragon.y + 0.05f
        );

        glm::vec2 direction =
            target - bossFireballs[i].position;

        float length = glm::length(direction);

        if (length > 0.001f)
            direction /= length;
        else
            direction = glm::vec2(-1.0f, 0.0f);

        bossFireballs[i].velocity = direction * 1.25f * ((float)(bossesDefeated / 4.0f) + 1.0f);
        bossFireballs[i].active = true;

        break;
    }
}

// COLLISION

bool overlaps(
    float ax, float ay, float ar,
    float bx, float by, float br)
{
    float dx = ax - bx;
    float dy = ay - by;

    float radius = ar + br;

    return dx * dx + dy * dy < radius * radius;
}

// DRAWING FUNCTION

void draw(
    const Mesh& mesh,
    const glm::mat4& model,
    const glm::vec4& color)
{
    glm::mat4 mvp = projection * model;

    glUniformMatrix4fv(
        transformLoc, 1, GL_FALSE,
        glm::value_ptr(mvp)
    );

    glUniform4fv(
        colorLoc, 1,
        glm::value_ptr(color)
    );

    glBindVertexArray(mesh.vao);

    glDrawElements(
        GL_TRIANGLES,
        mesh.count,
        GL_UNSIGNED_INT,
        nullptr
    );
}

glm::mat4 translateScale(
    float x, float y,
    float sx, float sy)
{
    glm::mat4 model(1.0f);

    model = glm::translate(
        model,
        glm::vec3(x, y, 0.0f)
    );

    return glm::scale(
        model,
        glm::vec3(sx, sy, 1.0f)
    );
}

// GAME UPDATE

void updateGame(float dt)
{
    if (gamePhase == GamePhase::GAME_OVER)
        return;

    dragon.hitTimer =
        std::max(0.0f, dragon.hitTimer - dt);

    dragon.invulnerableTimer =
        std::max(0.0f, dragon.invulnerableTimer - dt);

    dragon.shootCooldown =
        std::max(0.0f, dragon.shootCooldown - dt);

    boss.hitFlashTimer =
        std::max(0.0f, boss.hitFlashTimer - dt);

    boss.invulnerableTimer =
        std::max(0.0f, boss.invulnerableTimer - dt);

    bool left =
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;

    bool right =
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    bool fly =
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    bool descend =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

    bool shoot =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    if (left)
        dragon.direction = -1.0f;

    if (right)
        dragon.direction = 1.0f;

    if (shoot)
        shootPlayerFireball();

    // Vertical movement
    if (fly)
        dragon.y += 1.0f * dt;
    else if (descend)
        dragon.y -= 1.0f * dt;
    else
        dragon.y -= 0.5f * dt;

    dragon.y = std::max(-0.865f, dragon.y);

    dragon.y = std::min(
        worldHalfHeight - 0.3f,
        dragon.y
    );

    // Player wing animation
    float targetSpeed = fly ? 20.0f : 5.0f;

    dragon.wingSpeed +=
        (targetSpeed - dragon.wingSpeed) *
        std::min(1.0f, 6.0f * dt);

    dragon.wingPhase += dragon.wingSpeed * dt;

    // Side-scrolling (cu multiplicatorul tău bazat pe boși)
    float horizontalSpeed = 0.0f;

    if (dragon.y > -0.865f)
    {
        if (left || right || fly || !fly)
            horizontalSpeed = dragon.direction * 1.2f * ((float)bossesDefeated / 4.0f + 1);
    }

    // Obstacles
    if (gamePhase == GamePhase::OBSTACLES)
    {
        for (int i = 0; i < obstacleCount; i++)
        {
            obstacles[i].x -= horizontalSpeed * dt;

            if (obstacles[i].hit)
            {
                obstacles[i].fallAngle -= 5.0f * dt;

                obstacles[i].fallAngle =
                    std::max(
                        -PI / 2.0f,
                        obstacles[i].fallAngle
                    );
            }

            if (!obstacles[i].counted &&
                obstacles[i].x < dragon.x - 0.25f)
            {
                obstacles[i].counted = true;
              
                
                obstaclesPassed++;
            }
        }

        // Verificarea coliziunii dragonului cu obstacolele
        for (int i = 0; i < obstacleCount; i++)
        {
            if (obstacles[i].hit)
                continue;

            float obstacleTop =
                GROUND_Y + obstacles[i].height + 0.23f;

            if (std::abs(obstacles[i].x - dragon.x) < 0.12f &&
                dragon.y - 0.04f < obstacleTop &&
                dragon.y + 0.12f > GROUND_Y)
            {
                // 1. Scade viața dragonului și activează efectul de hit/invulnerabilitate
                damageDragon();

                // 2. Dă-l înapoi pe axa X (recul)
                dragon.x -= dragon.direction * 0.15f;

                // 3. Marchează obstacolul ca lovit (va cădea/prăbuși)
                obstacles[i].hit = true;
            }
        }

        if (obstaclesPassed >= OBSTACLES_PER_ROUND)
            spawnBoss();
    }

    // Boss movement and shooting
    if (gamePhase == GamePhase::BOSS && boss.active)
    {
        boss.time += dt;

        boss.x = worldHalfWidth - 0.42f;

        boss.y =
            0.05f + 0.15f * sin(boss.time * 1.8f);

        boss.shootTimer -= dt;

        if (boss.shootTimer <= 0.0f)
        {
            shootBossFireball();
            boss.shootTimer = randomFloat(1.0f, 2.8f);
        }
    }

    // Player fireballs
    for (int i = 0; i < MAX_PLAYER_FIREBALLS; i++)
    {
        if (!playerFireballs[i].active)
            continue;

        playerFireballs[i].position +=
            playerFireballs[i].velocity * dt;

        if (std::abs(playerFireballs[i].position.x) >
            worldHalfWidth + 0.5f)
        {
            playerFireballs[i].active = false;
            continue;
        }

        if (gamePhase == GamePhase::OBSTACLES)
        {
            for (int j = 0; j < obstacleCount; j++)
            {
                if (obstacles[j].hit)
                    continue;

                float boardY =
                    GROUND_Y + obstacles[j].height + 0.1f;

                if (std::abs(
                    playerFireballs[i].position.x -
                    obstacles[j].x) < 0.09f &&
                    std::abs(
                        playerFireballs[i].position.y -
                        boardY) < 0.15f)
                {
                    obstacles[j].hit = true;

                    
                    playerFireballs[i].active = false;

                    score += 1;
                    if (score > loadHighScore()) {
                        std::ofstream outFile("highscore.txt");
                        if (outFile.is_open()) {
                            highScore = true;
                            outFile << score;
                            outFile.close();
                        }
                    }
                    break;
                }
            }
        }
        else if (gamePhase == GamePhase::BOSS && boss.active)
        {
            // Hitbox centered on the boss's body
            if (overlaps(
                playerFireballs[i].position.x,
                playerFireballs[i].position.y,
                0.025f,
                boss.x - 0.15f,
                boss.y + 0.12f,
                0.25f))
            {
                playerFireballs[i].active = false;
                damageBoss();
            }
        }
    }

    // Boss fireballs
    for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
    {
        if (!bossFireballs[i].active)
            continue;

        bossFireballs[i].position +=
            bossFireballs[i].velocity * dt;

        if (std::abs(bossFireballs[i].position.x) >
            worldHalfWidth + 0.5f ||
            std::abs(bossFireballs[i].position.y) >
            worldHalfHeight + 0.5f)
        {
            bossFireballs[i].active = false;
            continue;
        }

        if (overlaps(
            bossFireballs[i].position.x,
            bossFireballs[i].position.y,
            0.025f,
            dragon.x,
            dragon.y,
            0.12f))
        {
            bossFireballs[i].active = false;
            damageDragon();
        }
    }
}

// RENDER GAME

void resetGame()
{
   
    dragon.lives = 5;
    dragon.x = -0.35f;
    dragon.y = -0.865f;
    dragon.direction = 1.0f;
    dragon.hitTimer = 0.0f;
    dragon.invulnerableTimer = 0.0f;
    dragon.shootCooldown = 0.0f;
    highScore = false;

  
    score = 0;
    obstaclesPassed = 0;
    bossesDefeated = 0;

  
    boss.active = false;
    for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
        bossFireballs[i].active = false;

    
    for (int i = 0; i < MAX_PLAYER_FIREBALLS; i++)
        playerFireballs[i].active = false;


    resetObstacles();
    
    std::cout << "The game restarted!\n";
}

void renderGame()
{
    // ==========================================
    // ANIMAȚIE CAP ȘI COADĂ (Vertebre)
    // ==========================================
    for(size_t i = 0; i < sizeof(initialDragonVertices) / sizeof(GLfloat); i++) {
        currentDragonVertices[i] = initialDragonVertices[i];
    }

    float currentTime = (float)glfwGetTime();
    bool isSpacePressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

  // HEAAD
    for (int i = 5; i <= 14; i++) {
        float origY = initialDragonVertices[i * 3 + 1];
        float origX = initialDragonVertices[i * 3];
        if (origX > 0.6f) {
            currentDragonVertices[i * 3 + 1] = origY + sin(currentTime * 4.0f) * 0.03f;
        }
    }

   // TAIL
    float tailAmplitude = 0.01f; 
    if (dragon.y > GROUND_Y + 0.035f) {
        tailAmplitude = 0.04f; 
    }
    if (isSpacePressed) {
        tailAmplitude = 0.07f; 
    }

    
    for (int i = 22; i <= 32; i++) {
        float origX = initialDragonVertices[i * 3];     
        float origY = initialDragonVertices[i * 3 + 1];
        float tailFactor = std::abs(origX + 0.12f); 
        currentDragonVertices[i * 3 + 1] = origY + sin(currentTime * 7.0f + origX * 4.0f) * tailAmplitude * tailFactor;
    }

    // VBO UPDATE
    dragonBodyMesh.updateVertices(currentDragonVertices, sizeof(currentDragonVertices) / sizeof(GLfloat));

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(programID);

    // Ground
    glm::mat4 groundModel = translateScale(
        0.0f,
        -worldHalfHeight,
        2.0f * worldHalfWidth,
        GROUND_Y + worldHalfHeight
    );

    draw(
        quadMesh,
        groundModel,
        glm::vec4(0.1f, 0.6f, 0.1f, 1.0f)
    );

    // Player damage rotation
    float hitAngle = 0.0f;

    if (dragon.hitTimer > 0.0f)
    {
        float elapsed = 0.6f - dragon.hitTimer;
        float fraction;

        if (elapsed < 0.3f)
            fraction = elapsed / 0.3f;
        else
            fraction = (0.6f - elapsed) / 0.3f;

        fraction = std::max(
            0.0f,
            std::min(1.0f, fraction)
        );

        hitAngle =
            glm::radians(90.0f) * fraction;
    }

    glm::mat4 dragonModel = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(dragon.x, dragon.y, 0.0f)
    );

    dragonModel = glm::rotate(
        dragonModel,
        hitAngle,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    if (dragon.direction < 0.0f)
    {
        dragonModel = glm::rotate(
            dragonModel,
            PI,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }

    dragonModel = glm::scale(
        dragonModel,
        glm::vec3(0.25f, 0.25f, 1.0f)
    );

    bool showDragon = true;

    if (dragon.invulnerableTimer > 0.0f)
    {
        showDragon =
            ((int)(dragon.invulnerableTimer * 12.0f) % 2 == 0);
    }

    if (showDragon)
    {
        // BODY
        draw(
            dragonBodyMesh,
            dragonModel,
            glm::vec4(0.07f, 0.07f, 0.14f, 1.0f)
        );

        // 2. WING ANIMATION - smooth flapping
        float dynamicWingAngle = sin(dragon.wingPhase) * 0.70f - 5.0f;

        glm::mat4 wingModel = glm::translate(
            dragonModel,
            glm::vec3(-0.1f, 0.1f, 0.0f)
        );

        wingModel = glm::rotate(
            wingModel,
            dynamicWingAngle,
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        wingModel = glm::translate(
            wingModel,
            glm::vec3(0.1f, -0.1f, 0.0f)
        );

        draw(
            dragonWingMesh,
            wingModel,
            glm::vec4(0.07f, 0.07f, 0.14f, 1.0f)
        );
    }

    // Obstacles
    for (int i = 0; i < obstacleCount; i++)
    {
        if (obstacles[i].x < -worldHalfWidth - 0.5f ||
            obstacles[i].x > worldHalfWidth + 0.5f)
            continue;

        glm::mat4 base = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(
                obstacles[i].x,
                GROUND_Y,
                0.0f
            )
        );

        base = glm::rotate(
            base,
            obstacles[i].fallAngle,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        glm::mat4 pole = glm::scale(
            base,
            glm::vec3(
                0.02f,
                obstacles[i].height,
                1.0f
            )
        );

        draw(
            quadMesh,
            pole,
            glm::vec4(0.18f, 0.09f, 0.1f, 1.0f)
        );

        glm::mat4 board = glm::translate(
            base,
            glm::vec3(
                0.0f,
                obstacles[i].height - 0.02f,
                0.0f
            )
        );

        board = glm::scale(
            board,
            glm::vec3(0.08f, 0.25f, 1.0f)
        );

        draw(
            quadMesh,
            board,
            glm::vec4(0.43f, 0.25f, 0.11f, 1.0f)
        );
    }

    if (boss.active)
    {
        bool showBoss = true;

        if (boss.hitFlashTimer > 0.0f)
        {
            showBoss =
                ((int)(boss.hitFlashTimer * 18.0f) % 2 == 0);
        }

        if (showBoss)
        {
            glm::mat4 bossModel = glm::translate(
                glm::mat4(1.0f),
                glm::vec3(boss.x, boss.y, 0.0f)
            );

            bossModel = glm::scale(
                bossModel,
                glm::vec3(0.42f, 0.42f, 1.0f)
            );

            // Boss body
            draw(
                bossBodyMesh,
                bossModel,
                glm::vec4(0.12f, 0.28f, 0.38f, 1.0f)
            );

            // Boss wing animation - smooth flapping
            float wingAngle =
                0.22f * sin(boss.time * 5.0f);

            glm::mat4 bossWingModel = bossModel;

            bossWingModel = glm::translate(
                bossWingModel,
                glm::vec3(0.4f, 0.0f, 0.0f)
            );

            bossWingModel = glm::rotate(
                bossWingModel,
                wingAngle,
                glm::vec3(1.0f, 0.0f, 0.0f)
            );

            bossWingModel = glm::translate(
                bossWingModel,
                glm::vec3(-0.4f, 0.0f, 0.0f)
            );

            draw(
                bossWingMesh,
                bossWingModel,
                glm::vec4(0.12f, 0.28f, 0.38f, 1.0f)
            );
        }

        // Boss lives
        for (int i = 0; i < 5; i++)
        {
            glm::vec4 color =
                i < boss.lives
                ? glm::vec4(1.0f, 0.2f, 0.1f, 1.0f)
                : glm::vec4(0.25f, 0.1f, 0.1f, 1.0f);

            glm::mat4 life = translateScale(
                worldHalfWidth - 0.6f + i * 0.11f,
                worldHalfHeight - 0.12f,
                0.08f,
                0.06f
            );

            draw(quadMesh, life, color);
        }
    }

    // Player fireballs
    for (int i = 0; i < MAX_PLAYER_FIREBALLS; i++)
    {
        if (!playerFireballs[i].active)
            continue;

        glm::mat4 model = translateScale(
            playerFireballs[i].position.x,
            playerFireballs[i].position.y,
            0.045f,
            0.045f
        );

        if (playerFireballs[i].velocity.x < 0.0f)
        {
            model = glm::rotate(
                model,
                PI,
                glm::vec3(0.0f, 0.0f, 1.0f)
            );
        }

        draw(
            fireMesh,
            model,
            glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)
        );
    }

    // Boss fireballs
    for (int i = 0; i < MAX_BOSS_FIREBALLS; i++)
    {
        if (!bossFireballs[i].active)
            continue;

        glm::mat4 model = translateScale(
            bossFireballs[i].position.x,
            bossFireballs[i].position.y,
            0.065f,
            0.065f
        );

        float angle = atan2(
            bossFireballs[i].velocity.y,
            bossFireballs[i].velocity.x
        );

        model = glm::rotate(
            model,
            angle,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        draw(
            fireMesh,
            model,
            glm::vec4(1.0f, 0.55f, 0.0f, 1.0f)
        );
    }

    // Player lives
    for (int i = 0; i < 5; i++)
    {
        glm::vec4 color =
            i < dragon.lives
            ? glm::vec4(0.1f, 0.6f, 0.1f, 1.0f)
            : glm::vec4(0.45f, 0.12f, 0.12f, 1.0f);

        glm::mat4 life = translateScale(
            -worldHalfWidth + 0.12f + i * 0.12f,
            worldHalfHeight - 0.12f,
            0.09f,
            0.06f
        );

        draw(quadMesh, life, color);
    }
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }

    // --- HINT-URI ADAUGATE PENTRU OPENGL 4.1 PE MACOS ---
    #ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #endif

    window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "Dragon Game",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Verificăm versiunea activă
    std::cout << "OpenGL Version activă: " << glGetString(GL_VERSION) << std::endl;

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed\n";
        glfwTerminate();
        return -1;
    }

    int framebufferWidth;
    int framebufferHeight;

    glfwGetFramebufferSize(
        window,
        &framebufferWidth,
        &framebufferHeight
    );

    updateProjection(
        framebufferWidth,
        framebufferHeight
    );

    glfwSetFramebufferSizeCallback(
        window,
        window_callback
    );

    glClearColor(
        0.37f, 0.84f, 1.0f, 1.0f
    );

    programID = LoadShaders(
        "SimpleVertexShader.vertexshader",
        "SimpleFragmentShader.fragmentshader"
    );

    if (programID == 0)
    {
        glfwTerminate();
        return -1;
    }

    transformLoc =
        glGetUniformLocation(programID, "transform");

    colorLoc =
        glGetUniformLocation(programID, "color");

    // Copiem verteprele inițiale în array-ul curent
    for(size_t i = 0; i < sizeof(initialDragonVertices) / sizeof(GLfloat); i++) {
        currentDragonVertices[i] = initialDragonVertices[i];
    }

    // Player dragon (Folosim array-ul curent pentru creare)
    dragonBodyMesh.create(
        currentDragonVertices,
        sizeof(currentDragonVertices) / sizeof(GLfloat),
        dragonBodyIndices,
        sizeof(dragonBodyIndices) / sizeof(GLuint)
    );

    dragonWingMesh.create(
        initialDragonVertices,
        sizeof(initialDragonVertices) / sizeof(GLfloat),
        dragonWingIndices,
        sizeof(dragonWingIndices) / sizeof(GLuint)
    );

    // Boss body
    bossBodyMesh.create(
        bossVertices,
        sizeof(bossVertices) / sizeof(GLfloat),
        bossBodyIndices,
        sizeof(bossBodyIndices) / sizeof(GLuint)
    );

    // Boss wing
    bossWingMesh.create(
        bossVertices,
        sizeof(bossVertices) / sizeof(GLfloat),
        bossWingIndices,
        sizeof(bossWingIndices) / sizeof(GLuint)
    );

    // Other meshes
    quadMesh.create(
        quadVertices,
        sizeof(quadVertices) / sizeof(GLfloat),
        quadIndices,
        sizeof(quadIndices) / sizeof(GLuint)
    );

    fireMesh.create(
        fireVertices,
        sizeof(fireVertices) / sizeof(GLfloat),
        fireIndices,
        sizeof(fireIndices) / sizeof(GLuint)
    );

    resetObstacles();

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        float currentTime = (float)glfwGetTime();

        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        deltaTime = std::max(
            0.0f,
            std::min(0.05f, deltaTime)
        );

        updateGame(deltaTime);
        renderGame();

       
        std::string title =
            "Dragon: " + std::to_string(dragon.lives) +
            "/5 | Score: " + std::to_string(score) +
            " | High Score: " + std::to_string(loadHighScore()) +
            " | Bosses defeated: " + std::to_string(bossesDefeated);

        if (highScore) {
            title += " | New High Score!";
        }

        if (boss.active)
        {
            title +=
                " | Boss: " +
                std::to_string(boss.lives) +
                "/5";
        }

        if (gamePhase == GamePhase::GAME_OVER)
        {
            title += " | GAME OVER - PRESS R TO RESTART";

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            {
                resetGame(); 
            }
        }

        glfwSetWindowTitle(
            window,
            title.c_str()
        );

        glfwSwapBuffers(window);
    }

    // Cleanup
    dragonBodyMesh.destroy();
    dragonWingMesh.destroy();

    bossBodyMesh.destroy();
    bossWingMesh.destroy();

    quadMesh.destroy();
    fireMesh.destroy();

    glDeleteProgram(programID);

    glfwTerminate();

    return 0;
}
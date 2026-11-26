// game.cpp
// -----------------------------------------------------------------------------
// Game class IMPLEMENTATION only. The declaration is in game.h, main() is in
// 源2.cpp. Do not #include this file anywhere - it is compiled by the project.
//
// Code layout (one definition per symbol, no .cpp is ever #included twice):
//   game.h                    -> class Game declaration
//   game.cpp                  -> class Game implementation (this file)
//   shaderr.h / shaderr.cpp   -> Shader class (this is the separated shader code)
//   camera.cpp                -> Camera class (header-only)
//   mesh.cpp                  -> Vertex / Texture / Mesh (header-only)
//   model.cpp                 -> Model + TextureFromFile (header-only)
//   myimage.cpp               -> Image helper (header-only)
//   glad.c / stb_image.cpp    -> third-party implementations, compiled once
// -----------------------------------------------------------------------------

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <memory>

#include "game.h"
#include "shaderr.h"
#include "camera.cpp"
#include "model.cpp"

namespace
{
    // Side length of one cube, in world units. The vertex data below is a
    // 1.0 unit cube, so every cube is simply scaled by this factor.
    // Neighbouring cubes are placed exactly kCubeSize apart (no gaps).
    constexpr float kCubeSize = 0.5f;

    // Sphere-ish tolerance for "is there already a cube at this spot".
    constexpr float kSameSpot = 1e-4f;

    // Cross-hair: half of its total length in pixels, then the bar
    // half-thickness as a fraction of that half length. The rim is drawn
    // first and is a bit thicker, the white core is drawn on top of it.
    constexpr float kCrosshairHalfLengthPx = 10.0f;
    constexpr float kCrosshairRimThickness = 0.22f;
    constexpr float kCrosshairCoreThickness = 0.13f;

    // Ray vs axis-aligned box (slab method).
    //   origin/dir  : the ray (dir should be normalised)
    //   boxMin/Max  : box corners
    //   tHit        : distance along the ray to the entry point
    //   normal      : outward normal of the face that was entered
    // Returns false when the ray misses the box or the box is behind it.
    bool rayIntersectBox(const glm::vec3& origin, const glm::vec3& dir,
                         const glm::vec3& boxMin, const glm::vec3& boxMax,
                         float& tHit, glm::vec3& normal)
    {
        float tMin = -FLT_MAX;
        float tMax = FLT_MAX;
        glm::vec3 hitNormal(0.0f);

        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(dir[axis]) < 1e-8f)
            {
                // ray is parallel to this slab: it must already be inside it
                if (origin[axis] < boxMin[axis] || origin[axis] > boxMax[axis])
                    return false;
                continue;
            }

            const float invDir = 1.0f / dir[axis];
            float tNear = (boxMin[axis] - origin[axis]) * invDir;
            float tFar = (boxMax[axis] - origin[axis]) * invDir;

            glm::vec3 axisNormal(0.0f);
            if (tNear > tFar)
            {
                std::swap(tNear, tFar);
                axisNormal[axis] = 1.0f;   // enters through the max face
            }
            else
            {
                axisNormal[axis] = -1.0f;  // enters through the min face
            }

            if (tNear > tMin)
            {
                tMin = tNear;
                hitNormal = axisNormal;
            }
            if (tFar < tMax)
                tMax = tFar;

            if (tMin > tMax)
                return false;
        }

        if (tMax < 0.0f)
            return false; // box is entirely behind the ray

        tHit = (tMin > 0.0f) ? tMin : 0.0f;
        normal = hitNormal;
        return true;
    }
}

Game::Game(int w, int h) : width(w), height(h)
{
}

const float Game::kCubeVertices[216] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

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

// ------------------------------------------------------------------ callbacks

void Game::framebufferSizeCallback(GLFWwindow* win, int w, int h)
{
    glViewport(0, 0, w, h);
}

void Game::mouseCallback(GLFWwindow* win, double xpos, double ypos)
{
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (!self)
        return;

    if (self->mouseFirstSample)
    {
        // first callback only stores the position, otherwise the camera jumps
        self->lastX = xpos;
        self->lastY = ypos;
        self->mouseFirstSample = false;
        return;
    }

    self->xoffset = xpos - self->lastX;
    self->yoffset = self->lastY - ypos; // y grows downwards in screen space
    self->lastX = xpos;
    self->lastY = ypos;
}

void Game::scrollCallback(GLFWwindow* win, double xOffset, double yOffset)
{
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (!self)
        return;

    self->fov -= (float)yOffset;
    if (self->fov < 1.0f)
        self->fov = 1.0f;
    if (self->fov > 45.0f)
        self->fov = 45.0f;
}

void Game::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods)
{
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (!self)
        return;

    // right button pressed -> try to place a cube on the face we are aiming at
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        self->tryPlaceCube();
}

// ---------------------------------------------------------------------- setup

bool Game::initWindow()
{
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    glfwReady = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(width, height, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    return true;
}

bool Game::initGL()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    return true;
}

bool Game::initScene()
{
    // cube VBO/VAO
    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal (byte offset 3 * sizeof(float) = 12)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // shaders (the shader code itself lives in shaderr.cpp)
    litShader = std::make_unique<Shader>("vertex", "fragmenttex");
    if (!litShader->isValid())
    {
        std::cout << "lit shader failed to build" << std::endl;
        return false;
    }

    // cross-hair: no vertex data is needed (the quad is built from
    // gl_VertexID), but core profile still wants a bound VAO for glDrawArrays
    glGenVertexArrays(1, &crosshairVAO);

    crosshairShader = std::make_unique<Shader>("crosshair_vertex", "crosshair_fragment");
    if (!crosshairShader->isValid())
    {
        // cosmetic only: keep running, just without a cross-hair
        std::cout << "crosshair shader failed to build, cross-hair disabled" << std::endl;
    }

    camera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));
    camera->setMouseSensitivity(0.05f);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // the starting cube, straight in front of the camera (crosshair is centred)
    addCube(glm::vec3(0.0f, 0.0f, 0.0f));

    // The assimp model (cube.obj) is a 2x2x2 cube centred on the origin, so it
    // would completely swallow the small cubes. It is loaded but no longer
    // drawn - uncomment the Draw call in render() to bring it back.
    model = std::make_unique<Model>("cube.obj");
    return true;
}

bool Game::init()
{
    if (!initWindow())
        return false;
    if (!initGL())
        return false;
    return initScene();
}

Game::~Game()
{
    // release GL objects while the context is still current
    litShader.reset();
    crosshairShader.reset();
    model.reset();
    camera.reset();

    if (cubeVBO)
        glDeleteBuffers(1, &cubeVBO);
    if (cubeVAO)
        glDeleteVertexArrays(1, &cubeVAO);
    if (crosshairVAO)
        glDeleteVertexArrays(1, &crosshairVAO);

    if (window)
        glfwDestroyWindow(window);

    if (glfwReady)
        glfwTerminate();
}

// ----------------------------------------------------------------- main loop

void Game::processInput()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // X shows/hides the cross-hair. Edge detection: the key must be released
    // before it can toggle again, otherwise holding it would flicker.
    const bool toggleDown = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
    if (toggleDown && !crosshairToggleHeld)
        crosshairVisible = !crosshairVisible;
    crosshairToggleHeld = toggleDown;
}

void Game::update(float dt)
{
    camera->setspeed(2.0f * dt);
    camera->ProcessMouseMovement((float)xoffset, (float)yoffset);
    xoffset = 0.0;
    yoffset = 0.0;
    camera->processInput(window);
}

// ---------------------------------------------------------------- cube grid

void Game::addCube(const glm::vec3& center)
{
    for (const auto& existing : cubes)
    {
        if (glm::length(existing - center) < kSameSpot)
            return; // that spot is already taken
    }
    cubes.push_back(center);
}

void Game::drawCube(const glm::vec3& center)
{
    // the vertex data is a 1.0 unit cube, scale it down to kCubeSize
    glm::mat4 model(1.0f);
    model = glm::translate(model, center);
    model = glm::scale(model, glm::vec3(kCubeSize));

    litShader->setMat4("model", model);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Game::tryPlaceCube()
{
    // With GLFW_CURSOR_DISABLED the cursor does not move, so the pick ray is
    // the camera forward vector - i.e. whatever is under the screen centre.
    const glm::vec3 origin = camera->Position;
    const glm::vec3 dir = glm::normalize(camera->Front);

    const float half = kCubeSize * 0.5f;
    const glm::vec3 halfExtent(half, half, half);

    bool hit = false;
    float nearest = FLT_MAX;
    glm::vec3 hitCenter(0.0f);
    glm::vec3 hitNormal(0.0f);

    for (const auto& center : cubes)
    {
        float t = 0.0f;
        glm::vec3 normal(0.0f);
        if (rayIntersectBox(origin, dir, center - halfExtent, center + halfExtent, t, normal)
            && t < nearest)
        {
            nearest = t;
            hitCenter = center;
            hitNormal = normal;
            hit = true;
        }
    }

    if (!hit)
        return; // aiming at nothing, place nothing

    // the new cube goes right next to the face we hit, one grid step away
    addCube(hitCenter + hitNormal * kCubeSize);
}

void Game::render()
{
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    if (fbHeight == 0)
        return; // window is minimized

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(fov),
                                            (float)fbWidth / (float)fbHeight,
                                            0.1f, 100.0f);
    glm::mat4 view = camera->GetViewMatrix();

    // viewPos must be uploaded while the program is bound: glUniform* acts on
    // the currently active program, which is why use() comes first.
    litShader->use();
    litShader->setMat4("projection", projection);
    litShader->setMat4("view", view);
    litShader->setVec3("viewPos", camera->Position);

    // Directional light: constant direction, constant colour (no time factor).
    litShader->setVec3("light.direction", lightDir);
    litShader->setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
    litShader->setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
    litShader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    litShader->setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
    litShader->setVec3("material.diffuse", 1.0f, 0.5f, 0.31f);
    litShader->setVec3("material.specular", 0.5f, 0.5f, 0.5f);
    litShader->setFloat("material.shininess", 32.0f);

    // every cube in the array, all drawn the same way
    for (const auto& center : cubes)
        drawCube(center);
    glBindVertexArray(0);

    // The assimp model is a 2x2x2 cube on the origin and would hide the small
    // cubes; uncomment to draw it again.
    // if (model)
    //     model->Draw(*litShader);

    // the cross-hair is drawn last so it always sits on top of the scene
    renderCrosshair(fbWidth, fbHeight);
}

void Game::renderCrosshair(int fbWidth, int fbHeight)
{
    if (!crosshairVisible || !crosshairShader || !crosshairShader->isValid())
        return;
    if (fbWidth <= 0 || fbHeight <= 0)
        return;

    // screen-space overlay: no depth test, and the soft edges need blending
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1.0 in NDC equals fbHeight / 2 pixels, hence the factor 2
    const float halfSize = kCrosshairHalfLengthPx * 2.0f / (float)fbHeight;
    const float aspect = (float)fbWidth / (float)fbHeight;

    crosshairShader->use();
    crosshairShader->setFloat("uAspect", aspect);
    crosshairShader->setFloat("uHalfSize", halfSize);

    glBindVertexArray(crosshairVAO);

    // dark rim first ...
    crosshairShader->setFloat("uThickness", kCrosshairRimThickness);
    crosshairShader->setVec3("uColor", 0.0f, 0.0f, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ... then the white core on top of it
    crosshairShader->setFloat("uThickness", kCrosshairCoreThickness);
    crosshairShader->setVec3("uColor", 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Game::run()
{
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput();
        update(deltaTime);
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

// main() lives in 源2.cpp so the Game class and the entry point can be built
// separately. Defining it here as well gives two definitions of main and
// another LNK2005.

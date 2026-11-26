#ifndef GAME_H
#define GAME_H

// Declarations only - no implementation, no main().
// game.cpp provides the definitions, the entry point file provides main().
// Never #include "game.cpp": it is compiled on its own, and including it
// would give every Game member function a second definition (LNK2005).

#include <glm/glm.hpp>

#include <memory>
#include <vector>

// forward declarations keep this header light: the full types are only
// needed inside game.cpp, where the unique_ptr destructors are instantiated
struct GLFWwindow;

class Shader;
class Camera;
class Model;

class Game
{
public:
    Game(int w = 800, int h = 600);
    ~Game();

    // creates window, GL context and all scene resources
    bool init();

    // main loop, returns when the window is closed
    void run();

    // ---- cube grid ----
    // Adds one cube whose CENTRE is `center` (world space). A cube that is
    // already at that spot is not added twice. Cube side length is kCubeSize
    // (defined in game.cpp), so neighbouring cubes are one kCubeSize apart.
    void addCube(const glm::vec3& center);

    // read-only access to every existing cube
    const std::vector<glm::vec3>& getCubes() const { return cubes; }

private:
    // GLFW callbacks are free functions, so they are static and reach the
    // instance through glfwGetWindowUserPointer().
    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);
    static void mouseCallback(GLFWwindow* win, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* win, double xOffset, double yOffset);
    static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods);

    bool initWindow();
    bool initGL();
    bool initScene();

    void processInput();
    void update(float dt);
    void render();

    // draws one cube of size kCubeSize at `center`; litShader must be bound
    void drawCube(const glm::vec3& center);

    // right-click handler: casts a ray through the crosshair, and if it hits a
    // cube, adds a new cube on the face that was hit
    void tryPlaceCube();

    // draws the aiming cross-hair at the centre of the screen. Screen space
    // only: depth test off, alpha blended, drawn after the 3D scene.
    void renderCrosshair(int fbWidth, int fbHeight);

    GLFWwindow* window = nullptr;
    bool glfwReady = false;

    int width = 800;
    int height = 600;

    unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;

    std::unique_ptr<Shader> litShader;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Model>  model;

    // cross-hair: an empty VAO (the quad comes from gl_VertexID) plus its own
    // tiny shader pair. Toggled with X.
    unsigned int crosshairVAO = 0;
    std::unique_ptr<Shader> crosshairShader;
    bool crosshairVisible = true;
    bool crosshairToggleHeld = false;

    // every cube that currently exists: one world-space centre per entry
    std::vector<glm::vec3> cubes;

    // directional light: the direction the light travels (sun high above,
    // pointing down and slightly to the left/back). No position, no animation.
    glm::vec3 lightDir{ -0.2f, -1.0f, -0.3f };

    float fov = 45.0f;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    double lastX = 400.0;
    double lastY = 300.0;
    double xoffset = 0.0;
    double yoffset = 0.0;
    bool   mouseFirstSample = true;

    // 36 vertices * 6 floats (position + normal)
    static const float kCubeVertices[216];
};

#endif // GAME_H

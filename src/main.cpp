#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <math.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight{
    glm::vec3 position;
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    Camera camera;
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, -3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
glm::vec3 lightSpotPos(0.0f,2.5f,-4.0f);

bool isSpotlightActivated = false;

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Grafika Projekat", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //ovo kad zakomentarisemo vis enam ne flipje teksturu
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    //Shader ourShader("resources/shaders/object.vs", "resources/shaders/object.fs");

    // load models
    // -----------
    //Model ourModel("resources/objects/backpack/backpack.obj");
    //ourModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(0.0f, 0.0f, 0.0f);
    pointLight.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    pointLight.diffuse = glm::vec3(0.6f, 0.6f, 0.6f);
    pointLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.0035f;
    pointLight.quadratic = 0.001f;

    SpotLight spotLight;
    spotLight.position = glm::vec3(0.0f, 2.5f, -4.0f);
    spotLight.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    spotLight.diffuse = glm::vec3(0.6f, 0.6f, 0.6f);
    spotLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);
    spotLight.direction = glm::vec3(0.0f,0.0f,-1.0f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.0035f;
    spotLight.quadratic = 0.001f;
    spotLight.cutOff = glm::cos(glm::radians(2.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(21.5f));

    Shader lightingShader("soba.vs", "soba.fs");
    Shader lightCubeShader("sijalica.vs", "sijalica.fs");
    Shader slikaShader("slika.vs", "slika.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices1[] = {
            // positions          // normals           // texture coords
            -10.0f,  10.0f,  10.0f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -10.0f,  10.0f, -10.0f, 1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -10.0f,  0.0f, -10.0f, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -10.0f, 0.0f, -10.0f, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -10.0f, 0.0f,  10.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -10.0f,  10.0f,  10.0f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,

            10.0f,  10.0f,  10.0f,  -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            10.0f,  10.0f, -10.0f,  -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            10.0f, 0.0f, -10.0f,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            10.0f, 0.0f, -10.0f,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            10.0f, 0.0f,  10.0f,  -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            10.0f,  10.0f,  10.0f,  -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,



            -10.0f, 0.0f,  10.0f,  0.0f,  0.0f,  -1.0f,  0.0f,  0.0f,
            10.0f, 0.0f,  10.0f,  0.0f,  0.0f,  -1.0f,  1.0f,  0.0f,
            10.0f,  10.0f,  10.0f,  0.0f,  0.0f,  -1.0f,  1.0f,  1.0f,
            10.0f,  10.0f,  10.0f,  0.0f,  0.0f,  -1.0f,  1.0f,  1.0f,
            -10.0f,  10.0f,  10.0f,  0.0f,  0.0f,  -1.0f,  0.0f,  1.0f,
            -10.0f, 0.0f,  10.0f,  0.0f,  0.0f,  -1.0f,  0.0f,  0.0f,



            -10.0f, 0.0f, -10.0f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,
            10.0f, 0.0f, -10.0f,  0.0f,  0.0f, 1.0f,   1.0f,  0.0f,
            10.0f,  10.0f, -10.0f,  0.0f,  0.0f, 1.0f,   1.0f,  1.0f,
            10.0f,  10.0f, -10.0f,  0.0f,  0.0f, 1.0f,   1.0f,  1.0f,
            -10.0f,  10.0f, -10.0f,  0.0f,  0.0f, 1.0f,  0.0f,  1.0f,
            -10.0f, 0.0f, -10.0f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,
    };

    float vertices2[] = {               //pod

            -10.0f, 0.0f, -10.0f,  0.0f, 1.0f,  0.0f,  0.0f,  4.0f,
            10.0f, 0.0f, -10.0f,  0.0f, 1.0f,  0.0f,  4.0f,  4.0f,
            10.0f, 0.0f,  10.0f,  0.0f, 1.0f,  0.0f,  4.0f,  0.0f,
            10.0f, 0.0f,  10.0f,  0.0f, 1.0f,  0.0f,  4.0f,  0.0f,
            -10.0f, 0.0f,  10.0f,  0.0f, 1.0f,  0.0f,  0.0f,  0.0f,
            -10.0f, 0.0f, -10.0f,  0.0f, 1.0f,  0.0f,  0.0f,  4.0f,

    };

    float vertices3[] = {               //plafon
            -10.0f,  10.0f, -10.0f,  0.0f,  -1.0f,  0.0f,  0.0f,  1.0f,
            10.0f,  10.0f, -10.0f,  0.0f,  -1.0f,  0.0f,  1.0f,  1.0f,
            10.0f,  10.0f,  10.0f,  0.0f,  -1.0f,  0.0f,  1.0f,  0.0f,
            10.0f,  10.0f,  10.0f,  0.0f,  -1.0f,  0.0f,  1.0f,  0.0f,
            -10.0f,  10.0f,  10.0f,  0.0f,  -1.0f,  0.0f,  0.0f,  0.0f,
            -10.0f,  10.0f, -10.0f,  0.0f,  -1.0f,  0.0f,  0.0f,  1.0f


    };

    float vertices4[] ={
            -0.5f, 8.0f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, 8.0f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f,7.0f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f,7.0f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f,7.0f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, 8.0f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f, 8.0f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f, 8.0f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f,7.0f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f,7.0f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f,7.0f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f, 8.0f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f,7.0f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,7.0f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,7.0f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,7.0f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,7.0f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,7.0f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f, 8.0f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, 8.0f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, 8.0f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, 8.0f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, 8.0f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, 8.0f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f,7.0f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f,7.0f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f, 8.0f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f, 8.0f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f, 8.0f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f,7.0f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f,7.0f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f,7.0f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f, 8.0f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f, 8.0f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f, 8.0f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f,7.0f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
    };

    float vertices[] = {
            -9.8f, 8.0f, 3.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top right
            -9.8f, 5.0f, 3.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // bottom right
            -9.8f, 5.0f, -3.0f, 1.0f, 0.0f, 0.0f, 0.0f,1.0f,  // bottom left
            -9.8f, 8.0f, -3.0f, 1.0f, 0.0f, 0.0f, 0.0f,0.0f,   // top left
    };
    unsigned int indices[] = {  // note that we start from 0!
            0, 1, 3,  // first Triangle
            1, 2, 3   // second Triangle
    };
    unsigned int slikaVBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &slikaVBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, slikaVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, zidoviVAO,podVAO,plafonVAO;
    glGenVertexArrays(1, &zidoviVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);

    glBindVertexArray(zidoviVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &podVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);

    glBindVertexArray(podVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &plafonVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3), vertices3, GL_STATIC_DRAW);

    glBindVertexArray(plafonVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO,lightCubeVBO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &lightCubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices4), vertices4, GL_STATIC_DRAW);

    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------
    unsigned int diffuseMap1 = loadTexture(FileSystem::getPath("resources/textures/cigle2.jpeg").c_str());
    unsigned int specularMap1 = loadTexture(FileSystem::getPath("resources/textures/belo.png").c_str());
    unsigned int diffuseMap2 = loadTexture(FileSystem::getPath("resources/textures/pod.jpeg").c_str());
    unsigned int diffuseMap3 = loadTexture(FileSystem::getPath("resources/textures/plafon.png").c_str());
    unsigned int diffuseMap4 = loadTexture(FileSystem::getPath("resources/textures/slika.jpeg").c_str());
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/boje.jpeg").c_str());

    // shader configuration
    // --------------------
    slikaShader.use();
    slikaShader.setInt("material.diffuse",0);
    slikaShader.setInt("material.specular",1);
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular",1);
    //model
    //Model
    Model vagon1Model(FileSystem::getPath("resources/objects/vagoni/train-cart.obj"));
    Model vagon2Model(FileSystem::getPath("resources/objects/vagoni/train-cart.obj"));
    Model tenkModel(FileSystem::getPath("resources/objects/tenk/german-panzer-ww2-ausf-b.obj"));


    //Shader objectShader("soba.vs","soba.fs");
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        slikaShader.use();
        slikaShader.setMat4("projection", projection);
        slikaShader.setMat4("view", view);
        slikaShader.setMat4("model", model);

        slikaShader.setVec3("light.position", lightPos);
        lightingShader.setVec3("viewPos", programState->camera.Position);


        slikaShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        slikaShader.setVec3("light.diffuse", 0.6f, 0.6f, 0.6f);
        slikaShader.setVec3("light.specular",0.6f,0.6f,0.6f);
        slikaShader.setFloat("material.shininess", 8.0f);
        slikaShader.setFloat("light.constant",pointLight.constant);
        slikaShader.setFloat("light.linear",pointLight.linear);
        slikaShader.setFloat("light.quadratic",pointLight.quadratic);



        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap4);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        lightingShader.use();
        lightingShader.setVec3("light.position", lightPos);
        lightingShader.setVec3("material.diffuse2",spotLight.diffuse);
        lightingShader.setVec3("viewPos", programState->camera.Position);

        // view/projection transformations
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        lightingShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        lightingShader.setVec3("light.diffuse", 0.6f, 0.6f, 0.6f);
        lightingShader.setVec3("light.specular",0.6f,0.6f,0.6f);

        lightingShader.setFloat("light.constant",pointLight.constant);
        lightingShader.setFloat("light.linear",pointLight.linear);
        lightingShader.setFloat("light.quadratic",pointLight.quadratic);



        // bind diffuse map
        lightingShader.setMat4("model", model);
        //lightingShader.setVec3("material.specular", 0.05f, 0.05f, 0.05f);
        lightingShader.setFloat("material.shininess", 64.0f);
        lightingShader.setVec3("light.specular",0.02f,0.02f,0.02f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap1);

        // render the cube
        glBindVertexArray(zidoviVAO);
        glDrawArrays(GL_TRIANGLES, 0, 24);


        //lightingShader.setVec3("material.specular", 0.15f, 0.15f, 0.15f);
        //lightingShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        lightingShader.setVec3("light.specular",0.2f,0.2f,0.2f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap3);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap1);

        glBindVertexArray(plafonVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //lightingShader.setVec3("material.specular", 0.7f, 0.7f, 0.7f);
        //lightingShader.setVec3("light.diffuse", 0.7f, 0.7f, 0.7f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap2);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap1);

        lightingShader.setVec3("light.specular",0.6f,0.6f,0.6f);
        // render the cube
        glBindVertexArray(podVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        lightingShader.setVec3("lightSpot.position", programState->camera.Position);

        if(isSpotlightActivated) {
            lightingShader.setVec3("lightSpot.ambient", 0.0f, 0.0f, 0.0f);
            lightingShader.setVec3("lightSpot.diffuse", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightSpot.specular", 1.0f, 1.0f, 1.0f);
        }else{
            lightingShader.setVec3("lightSpot.ambient", 0.0f, 0.0f, 0.0f);
            lightingShader.setVec3("lightSpot.diffuse", 0.0f, 0.0f, 0.0f);
            lightingShader.setVec3("lightSpot.specular", 0.0f, 0.0f, 0.0f);
        }
        lightingShader.setFloat("lightSpot.cutOff",spotLight.cutOff);
        lightingShader.setFloat("lightSpot.outerCutOff",spotLight.outerCutOff);
        lightingShader.setVec3("lightSpot.direction", programState->camera.Front);

        lightingShader.setFloat("lightSpot.constant",pointLight.constant);
        lightingShader.setFloat("lightSpot.linear",pointLight.linear);
        lightingShader.setFloat("lightSpot.quadratic",pointLight.quadratic);

        lightingShader.setFloat("material.shininess", 64.0f);


        //lightingShader.use();
        lightingShader.setVec3("light.ambient",glm::vec3(1.0f,1.0f,1.0f));
        glm::mat4 modelTenk = glm::mat4(1.0f);
        modelTenk = glm::scale(modelTenk, glm::vec3(1.5f));
        lightingShader.setMat4("model", modelTenk);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        tenkModel.Draw(lightingShader);



        //lightingShader.use();
        glm::mat4 modelvagon = glm::mat4(1.0f);
        modelvagon = glm::rotate(modelvagon,(float)glfwGetTime(),glm::vec3(0.0f,1.0f,0.0f));
        modelvagon = glm::translate(modelvagon,glm::vec3(8.0f,0.0f,0.0f));
        modelvagon = glm::scale(modelvagon,glm::vec3(0.1f));

        lightingShader.setMat4("model",modelvagon);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        vagon1Model.Draw(lightingShader);

        //lightingShader.use();
        modelvagon = glm::mat4(1.0f);
        modelvagon = glm::rotate(modelvagon,glm::radians(37.0f),glm::vec3(0.0f,1.0f,0.0f));
        modelvagon = glm::rotate(modelvagon,(float)glfwGetTime(),glm::vec3(0.0f,1.0f,0.0f));
        modelvagon = glm::translate(modelvagon,glm::vec3(8.0f,0.0f,0.0f));
        modelvagon = glm::scale(modelvagon,glm::vec3(0.1f));


        lightingShader.setMat4("model",modelvagon);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        vagon1Model.Draw(lightingShader);

        // also draw the lamp object
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(1.0f)); // a smaller cube
        lightCubeShader.setMat4("model", model);

        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        isSpotlightActivated = !isSpotlightActivated;
    }

    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadTexture(char const * path)
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
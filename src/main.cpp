#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const *path);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

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

struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    DirLight dirLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Fallout", NULL, NULL);
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
    stbi_set_flip_vertically_on_load(true);

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

    //blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------

    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader zastava("resources/shaders/Zastava.vs","resources/shaders/Zastava.fs");
    Shader blending("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    // load models
    // -----------
    Model ourModel("resources/objects/tree/scene.gltf");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model drvo2("resources/objects/old_tree/scene.gltf");
    drvo2.SetShaderTextureNamePrefix("material.");

    Model zemlja2("resources/objects/ground/scene.gltf");
    zemlja2.SetShaderTextureNamePrefix("material.");

    Model lobanja("resources/objects/fox_skull_obj/Fox skull OBJ/fox_skull.obj");
    lobanja.SetShaderTextureNamePrefix("material.");

    Model vatra("resources/objects/smoldering_logs_red_light_bonfire_l/scene.gltf");
    vatra.SetShaderTextureNamePrefix("material.");

    Model zbun("resources/objects/tumbleweed/scene.gltf");
    zbun.SetShaderTextureNamePrefix("material.");

    Model ranger("resources/objects/ncr_veteran_ranger_fallout_4/scene.gltf");
    ranger.SetShaderTextureNamePrefix("material.");

    Model cep("resources/objects/nuka_cola_bottle_cap/scene.gltf");
    cep.SetShaderTextureNamePrefix("material.");

    Model Ruksak("resources/objects/backpack (1)/scene.gltf");
    Ruksak.SetShaderTextureNamePrefix("material.");

    Model bobblehead("resources/objects/ncr_veteran_ranger_bobblehead/scene.gltf");
    bobblehead.SetShaderTextureNamePrefix("material.");

    Model pipBoy("resources/objects/retro-modernized_pip_boy_editable_screen/scene.gltf");
    pipBoy.SetShaderTextureNamePrefix("material.");

    float flagVertices[] = {
            //      vertex           texture        normal
            60.0f, -20.0f,  30.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -60.0f, -20.0f, -30.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -60.0f, -20.0f,  30.0f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f,


            60.0f, -20.0f,  30.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            60.0f, -20.0f, -30.0f,  1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -60.0f, -20.0f, -30.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f
    };


    unsigned int flagVAO, flagVBO;
    glGenVertexArrays(1, &flagVAO);
    glGenBuffers(1, &flagVBO);

    glBindVertexArray(flagVAO);

    glBindBuffer(GL_ARRAY_BUFFER, flagVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(flagVertices), &flagVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    glBindVertexArray(0);

    unsigned int flagTexture = loadTexture("resources/textures/41v7zxV8B4L._AC_.jpg");
    zastava.use();
    zastava.setInt("texture1", 0);

    //skyBox

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.5, 0.5, 0.5);
    pointLight.diffuse = glm::vec3(1.0, 1.0, 1.0);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.2f;
    pointLight.quadratic = 0.2f;


    // skybox vao
    unsigned int skyBoxVAO, skyBoxVBO;
    glGenVertexArrays(1, &skyBoxVAO);
    glGenBuffers(1, &skyBoxVBO);
    glBindVertexArray(skyBoxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/raspberry_rt.jpg"),
                    FileSystem::getPath("resources/textures/skybox/raspberry_lf.jpg"),
                    FileSystem::getPath("resources/textures/skybox/raspberry_dn.jpg"),
                    FileSystem::getPath("resources/textures/skybox/raspberry_up.jpg"),
                    FileSystem::getPath("resources/textures/skybox/raspberry_bk.jpg"),
                    FileSystem::getPath("resources/textures/skybox/raspberry_ft.jpg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    zastava.use();
    zastava.setInt("texture1", 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);


    float stoneVertices[] = {
            // positions          texture        normal
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f,   0.0f,  1.0f,  0.0f
    };

    glm::vec3 stonePosition[] = {
            glm::vec3(4.0f, 0.4f, 7.0f),
            glm::vec3(-5.6f, 1.8f, -6.2f),
            glm::vec3(-5.0f, 1.0f, -1.0f),


    };

    unsigned int stoneVAO, stoneVBO;
    glGenVertexArrays(1, &stoneVAO);
    glGenBuffers(1, &stoneVBO);
    glBindVertexArray(stoneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, stoneVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(stoneVertices), stoneVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glBindVertexArray(0);

    stbi_set_flip_vertically_on_load(false);
    unsigned int bushTexture = loadTexture("resources/textures/pngwing.com.png");
    stbi_set_flip_vertically_on_load(true);
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    DirLight& dirLight = programState->dirLight;
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        dirLight.direction = glm::vec3(2.0f, -2.0f, 0.3f);
        dirLight.ambient = glm::vec3(0.21f, 0.21f, 0.21f);
        dirLight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
        dirLight.specular = glm::vec3(0.2f, 0.2f, 0.2f);

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        zastava.use();
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        zastava.setMat4("view", view);
        zastava.setMat4("projection", projection);

        glBindVertexArray(flagVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flagTexture);
        model = glm::translate(model, glm::vec3(0.0f, -5.0f, 0.0f));
       //model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0.0f, 1.0f));
        zastava.setMat4("model", model);


        zastava.setVec3("dirLight.direction", dirLight.direction);
        zastava.setVec3("dirLight.ambient", dirLight.ambient);
        zastava.setVec3("dirLight.diffuse", dirLight.diffuse);
        zastava.setVec3("dirLight.specular", dirLight.specular);
        zastava.setFloat("shininess", 64.0f);


        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);
        ourShader.setFloat("shininess", 32.0f);

        pointLight.position = glm::vec3(1.0 , 0.72f, 3.0 );
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection1 = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view1 = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glDepthFunc(GL_LEQUAL);

        //Face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // render the loaded model
        //prvo drvo
        glm::mat4 modelDrvo = glm::mat4(1.0f);
        modelDrvo = glm::translate(modelDrvo,
                               glm::vec3(10.0f, 0.74f, 1.0f)); // translate it down so it's at the center of the scene
        modelDrvo = glm::rotate(modelDrvo, glm::radians(90.0f), glm::vec3(0, 0.0f, 1.0f));
        modelDrvo = glm::scale(modelDrvo, glm::vec3(2.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelDrvo);
        ourModel.Draw(ourShader);

        glm::mat4 modelRanger = glm::mat4(1.0f);
        modelRanger = glm::translate(modelRanger,
                                     glm::vec3(1.0f, 1.06f, -3.0f));
        modelRanger = glm::rotate(modelRanger, glm::radians(-30.0f), glm::vec3( 0.0f, 1.0f, 0.0f));
        modelRanger = glm::scale(modelRanger, glm::vec3(0.04f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelRanger);
        ranger.Draw(ourShader);

        glm::mat4 modelCep = glm::mat4(1.0f);
        modelCep = glm::translate(modelCep,
                                  glm::vec3(3.0f, 1.1f, -2.0f));
        modelCep = glm::rotate(modelCep, glm::radians(-90.0f), glm::vec3( 1.0f, 0.0f, 0.0f));
        modelCep = glm::scale(modelCep, glm::vec3(0.005f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelCep);
        cep.Draw(ourShader);

        glm::mat4 modelDrvo2 = glm::mat4(1.0f);
        modelDrvo2 = glm::translate(modelDrvo2,
                                    glm::vec3(-5.0f, 0.4f, 1.0f)); // translate it down so it's at the center of the scene
         modelDrvo2 = glm::rotate(modelDrvo2, glm::radians(180.0f), glm::vec3(0, 1.0f, 0.0f));
        modelDrvo2 = glm::scale(modelDrvo2, glm::vec3(1.5f));    // it's a bit too big for our scene, so scale it down
        ourShader.setVec3("dirLight.direction", -1.0f*dirLight.direction);
        ourShader.setMat4("model", modelDrvo2);
        drvo2.Draw(ourShader);

        glm::mat4 modelZemlja2 = glm::mat4(1.0f);
        modelZemlja2 = glm::translate(modelZemlja2,
                                      glm::vec3(1.0f, -.0f, 1.0f)); // translate it down so it's at the center of the scene
        modelZemlja2 = glm::rotate(modelZemlja2, glm::radians(-90.0f), glm::vec3(1.0f,  0.0f, 0));
        modelZemlja2 = glm::scale(modelZemlja2, glm::vec3(0.55f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelZemlja2);
        zemlja2.Draw(ourShader);

        glm::mat4 modelLobanja = glm::mat4(1.0f);
        modelLobanja = glm::translate(modelLobanja,
                                      glm::vec3(1.0f, 0.85f, 1.0f)); // translate it down so it's at the center of the scene
        // modelLobanja = glm::rotate(modelLobanja, glm::radians(90.0f), glm::vec3(0, 0.0f, 1.0f));
        modelLobanja = glm::scale(modelLobanja, glm::vec3(0.012f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelLobanja);
        lobanja.Draw(ourShader);

        glm::mat4 modelvatra = glm::mat4(1.0f);
        modelvatra = glm::translate(modelvatra,
                                    glm::vec3(1.0f, 0.72f, 3.0f)); // translate it down so it's at the center of the scene
        // modelvatra = glm::rotate(modelvatra, glm::radians(90.0f), glm::vec3(0, 0.0f, 1.0f));
        modelvatra = glm::scale(modelvatra, glm::vec3(1.5f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelvatra);
        vatra.Draw(ourShader);

        glm::mat4 modelZbun = glm::mat4(1.0f);
        modelZbun = glm::translate(modelZbun,
                                   glm::vec3(1.0f, 1.84f, -7.0f)); // translate it down so it's at the center of the scene
        // modelZbun = glm::rotate(modelZbun, glm::radians(90.0f), glm::vec3(0, 0.0f, 1.0f));
        modelZbun = glm::scale(modelZbun, glm::vec3(0.17f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelZbun);
        zbun.Draw(ourShader);



        glm::mat4 modelRuksak = glm::mat4(1.0f);
        modelRuksak = glm::translate(modelRuksak,
                                  glm::vec3(-1.2f, 1.0f, 4.0f));
        modelRuksak = glm::rotate(modelRuksak, glm::radians(150.0f), glm::vec3( 0.0f, 1.0f, 0.0f));
        modelRuksak = glm::scale(modelRuksak, glm::vec3(0.01f));
        ourShader.setVec3("dirLight.direction", -1.0f*dirLight.direction);// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelRuksak);
        Ruksak.Draw(ourShader);

        glm::mat4 modelBoblehead = glm::mat4(1.0f);
        modelBoblehead = glm::translate(modelBoblehead,
                                  glm::vec3(-1.2f, 1.0f, 4.3f));
        modelBoblehead = glm::rotate(modelBoblehead, glm::radians(-30.0f), glm::vec3( 0.0f, 1.0f, 0.0f));
        modelBoblehead = glm::scale(modelBoblehead, glm::vec3(0.005f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelBoblehead);
        bobblehead.Draw(ourShader);

        glm::mat4 modelPipBoy = glm::mat4(1.0f);
        modelPipBoy = glm::translate(modelPipBoy,
                                        glm::vec3(-0.5f, 0.67f, 5.0f));
        //modelPipBoy = glm::rotate(modelPipBoy, glm::radians(-30.0f), glm::vec3( 0.0f, 1.0f, 0.0f));
        modelPipBoy = glm::scale(modelPipBoy, glm::vec3(0.2f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelPipBoy);
        pipBoy.Draw(ourShader);

        glDisable(GL_CULL_FACE);
        glBindVertexArray(stoneVAO);
        glBindTexture(GL_TEXTURE_2D, bushTexture);

        blending.use();
        blending.setInt("texture1", 0);
        blending.setMat4("projection", projection);
        blending.setMat4("view", view);

        blending.setVec3("dirLight.direction", dirLight.direction);
        blending.setVec3("dirLight.ambient", dirLight.ambient);
        blending.setVec3("dirLight.diffuse", dirLight.diffuse);
        blending.setVec3("dirLight.specular", dirLight.specular);
        blending.setFloat("shininess", 32.0f);

        for(int i = 0; i < 3; i++) {
            model = glm::mat4(1.0f);
            model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(0, 1, 0));
            model = glm::translate(model, stonePosition[i]);
            model = glm::scale(model, glm::vec3(0.8f));
            blending.use();
            blending.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glEnable(GL_CULL_FACE);


        // skybox cube

        skyboxShader.use();
        view[3][0] = 0;
        view[3][1] = 0;
        view[3][2] = 0;
        view[3][3] = 0;
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyBoxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


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

unsigned int loadTexture(char const *path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
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
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
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
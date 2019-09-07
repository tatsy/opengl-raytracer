#define GLRT_API_EXPORT
#include "window.h"

#include <random>
#include <experimental/filesystem>

#include <stb_image_write.h>

#include <glm/gtx/string_cast.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include "common.h"
#include "timer.h"
#include "vertex_array_object.h"
#include "framebuffer_object.h"
#include "shader_program.h"
#include "texture_buffer.h"

namespace fs = std::experimental::filesystem;

static void error_callback(int err, const char *desc) {
    printf("GLFW error: %s (%d)\n", desc, err);
}

namespace glrt {

Window::Window() {

    // Set error callback
    glfwSetErrorCallback(error_callback);

    // GLFW initialization
    if (glfwInit() == GLFW_FALSE) {
        FatalError("GLFW initialization failed!");
    }

#if defined(_WIN32) || defined(_WIN64)
    const char *glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#elif defined(__APPLE__) && defined(__MACH__)
    const char *glsl_version = "#version 410";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    const char *glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window_ = glfwCreateWindow(512, 512, "OpenGL ray tracer", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        FatalError("Failed to create GLFW window!");
    }
    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    glfwSwapInterval(0);

    // Setup ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 16.0f);
    ImGui::StyleColorsDark();

    // Load OpenGL library
    const int version = gladLoadGL();
    if (version == 0) {
        FatalError("GLAD: failed to load OpenGL library\n");
    }

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Callbacks
    auto resizeCallback = [](GLFWwindow *w, int width, int height) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(w))->resizeDefault(width, height);
    };
    glfwSetWindowSizeCallback(window_, resizeCallback);

    auto mouseCallback = [](GLFWwindow *w, int button, int action, int mods) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(w))->mouseDefault(button, action, mods);
    };
    glfwSetMouseButtonCallback(window_, mouseCallback);

    auto cursorPosCallback = [](GLFWwindow *w, double xpos, double ypos) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(w))->cursorPosDefault(xpos, ypos);
    };
    glfwSetCursorPosCallback(window_, cursorPosCallback);

    auto keyboardCallback = [](GLFWwindow *w, int key, int scancode, int action, int mods) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(w))->keyboardDefault(key, scancode, action, mods);
    };
    glfwSetKeyCallback(window_, keyboardCallback);
}

void Window::mainloop(const std::shared_ptr<Scene> &scene, double fps) {
    Timer killTimer;
    killTimer.start();

    // Set scene and resize window
    this->scene = scene;
    resize(scene->width, scene->height);

    // User's initialization
    initialize();

    // Update window size (for Apple's Letina display)
    int renderBufferWidth, renderBufferHeight;
    glfwGetFramebufferSize(window_, &renderBufferWidth, &renderBufferHeight);
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);

    // Mainloop
    timer.start();
    double duration = 1.0;
    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        // Handle events
        glfwPollEvents();

        // Draw current frame
        if (timer.count() >= 1.0 / fps) {
            // ImGui
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

//            ImGui::Begin("Status");
//            ImGui::Text("Vendor: %s", glGetString(GL_VENDOR));
//            ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
//            ImGui::Text("OpenGL: %s", glGetString(GL_VERSION));
//            ImGui::Text("GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
//            ImGui::Text("FPS: %7.3f", 1.0 / duration);
//            ImGui::Text("Size: %d x %d", width(), height());
//
//            static char temp[256] = "output.png";
//            ImGui::InputText(": file name", temp, IM_ARRAYSIZE(temp));
//
//            const std::string outFile = std::string(temp);
//            if (ImGui::Button("Capture")) {
//                saveCurrentFrame(outFile);
//            }
//
//            ImGui::End();

            ImGui::Render();

            int screenWidth, screenHeight;
            glfwMakeContextCurrent(window_);
            glfwGetFramebufferSize(window_, &screenWidth, &screenHeight);
            glViewport(0, 0, screenWidth, screenHeight);

            render();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwMakeContextCurrent(window_);
            glfwSwapBuffers(window_);

            if (killTimer.count() > 50.0) {
                saveCurrentFrame("output.png");
            }

            if (killTimer.count() > 60.0) {
                glfwSetWindowShouldClose(window_, true);
            }

            // Reset timer
            duration = timer.count();
            timer.reset();
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();
}

// ---------------------------------------------------------------------------------------------------------------------
// PROTECTED methods
// ---------------------------------------------------------------------------------------------------------------------

void Window::initialize() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // VAO
    vao = std::make_shared<VertexArrayObject>();

    // FBO
    resetBuffer();

    // Shader
    screenProgram = std::make_shared<ShaderProgram>();
    screenProgram->create();
    screenProgram->attachShader(ShaderStage::fromFile("screen.vert", ShaderType::Vertex));
    screenProgram->attachShader(ShaderStage::fromFile("screen.frag", ShaderType::Fragment));
    screenProgram->link();

    rtProgram = std::make_shared<ShaderProgram>();
    rtProgram->create();
    rtProgram->attachShader(ShaderStage::fromFile("raytrace.vert", ShaderType::Vertex));
    rtProgram->attachShader(ShaderStage::fromFile("raytrace.frag", ShaderType::Fragment));
    rtProgram->link();
}

void Window::render() {
    static int select = 0;
    select ^= 0x1;

    rtProgram->start();
    fbo[select]->bind();
    vao->bind();

    GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, bufs);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static std::random_device randev;
    static std::mt19937 mt(randev());
    static std::uniform_real_distribution<float> dist;

    const glm::mat4 camMat = scene->viewM * scene->modelM;
    const glm::mat4 projMat = scene->projM;
    const glm::mat4 c2wMat = glm::inverse(camMat);
    const glm::mat4 s2cMat = glm::inverse(projMat);
    rtProgram->setMatrix4x4("u_c2wMat", c2wMat);
    rtProgram->setMatrix4x4("u_s2cMat", s2cMat);
    rtProgram->setUniform2f("u_seed", glm::vec2(dist(mt), dist(mt)));
    rtProgram->setUniform1i("u_nSamples", 4);
    rtProgram->setUniform2f("u_windowSize", glm::vec2((float)width(), (float)height()));

    rtProgram->setUniform1i("u_nTris", (int)scene->triangles.size());
    rtProgram->setUniform1i("u_nLights", (int)scene->lights.size());

    // Screen textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo[select ^ 0x1]->textureId(0));
    rtProgram->setUniform1i("u_framebuffer", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fbo[select ^ 0x1]->textureId(1));
    rtProgram->setUniform1i("u_counter", 1);

    // Scene texture buffers
    scene->vertTexBuffer->bind(2);
    rtProgram->setUniform1i("u_vertBuffer", 2);

    scene->triTexBuffer->bind(3);
    rtProgram->setUniform1i("u_triBuffer", 3);

    scene->mtrlTexBuffer->bind(4);
    rtProgram->setUniform1i("u_matBuffer", 4);

    scene->lightTexBuffer->bind(5);
    rtProgram->setUniform1i("u_lightBuffer", 5);

    // BVH
    scene->bvhTexBuffer->bind(6);
    rtProgram->setUniform1i("u_bvhBuffer", 6);

    // Volume textures
    if (!scene->volumes.empty()) {
        rtProgram->setUniform1i("u_hasVolume", 1);
        rtProgram->setUniform3f("u_bboxMin", scene->volumes[0].bboxMin);
        rtProgram->setUniform3f("u_bboxMax", scene->volumes[0].bboxMax);
        rtProgram->setUniform1f("u_densityMax", scene->volumes[0].maxValue);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_3D, scene->volumes[0].densityTex);
        rtProgram->setUniform1i("u_densityTex", 7);

        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_3D, scene->volumes[0].temperatureTex);
        rtProgram->setUniform1i("u_temperatureTex", 8);
    } else {
        rtProgram->setUniform1i("u_hasVolume", 0);
    }

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, 6);

    fbo[select]->unbind();
    vao->unbind();
    rtProgram->end();

    // Render to screen
    screenProgram->start();
    vao->bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    screenProgram->setMatrix4x4("u_mvMat", camMat);
    screenProgram->setMatrix4x4("u_projMat", scene->projM);
    screenProgram->setUniform2f("u_windowSize", glm::vec2((float)width(), (float)height()));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo[select]->textureId(0));
    screenProgram->setUniform1i("u_framebuffer", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fbo[select]->textureId(1));
    screenProgram->setUniform1i("u_counter", 1);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    vao->unbind();
    screenProgram->end();
}

// ---------------------------------------------------------------------------------------------------------------------
// PRIVATE methods
// ---------------------------------------------------------------------------------------------------------------------

void Window::resizeDefault(int width, int height) {
    // Update window size
    glfwSetWindowSize(window_, width, height);

    // Update viewport following actual window size
    int renderBufferWidth, renderBufferHeight;
    glfwGetFramebufferSize(window_, &renderBufferWidth, &renderBufferHeight);
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);

    // Update FBO
    resetBuffer();
}

void Window::mouseDefault(int button, int action, int mods) {
    int width, height;
    glfwGetWindowSize(window_, &width, &height);

    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);

    mouseEvent = MouseEvent(MouseButton(button), MouseAction(action), KeyModifier(mods), xpos, ypos, width, height);
    mouse(mouseEvent);
}

void Window::keyboardDefault(int key, int scancode, int action, int mods) {
    // ImGui
    ImGui_ImplGlfw_KeyCallback(window_, key, scancode, action, mods);

    // Custom
    keyboard(key, scancode, action, mods);
}

void Window::cursorPosDefault(double xpos, double ypos) {
    int width, height;
    glfwGetWindowSize(window_, &width, &height);

    const auto button = mouseEvent.button();
    const auto mods = mouseEvent.modifier();
    mouseEvent = MouseEvent(button, MouseAction::Move, mods, xpos, ypos, width, height);
    mouse(mouseEvent);
}

void Window::resetBuffer() {
    fbo[0] = std::make_shared<FramebufferObject>(width(), height(), GL_RGB32F, GL_RGB, GL_FLOAT);
    fbo[0]->addColorAttachment(width(), height(), GL_R32F, GL_RED, GL_FLOAT);
    fbo[1] = std::make_shared<FramebufferObject>(width(), height(), GL_RGB32F, GL_RGB, GL_FLOAT);
    fbo[1]->addColorAttachment(width(), height(), GL_R32F, GL_RED, GL_FLOAT);

    GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    fbo[0]->bind();
    glDrawBuffers(2, bufs);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fbo[0]->unbind();
    fbo[1]->bind();
    glDrawBuffers(2, bufs);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fbo[0]->unbind();
}

void Window::saveCurrentFrame(const std::string &filename) const {
    // Read pixels
    const int w = width();
    const int h = height();
    auto bytes = std::make_unique<uint8_t[]>(w * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (void*)bytes.get());

    // Invert vertically
    for (int y = 0; y < h / 2; y++) {
        for (int x = 0; x < w; x++) {
            const int iy = h - y - 1;
            for (int c = 0; c < 4; c++) {
                std::swap(bytes[(y * w + x) * 4 + c], bytes[(iy * w + x) * 4 + c]);
            }
        }
    }

    // Check file existance
    fs::path filePath(filename.c_str());
    std::string baseName = filePath.stem().string();
    std::string ext = filePath.extension().string();
    int count = 0;
    while (fs::exists(filePath)) {
        char temp[256];
        sprintf(temp, "%s_%d%s", baseName.c_str(), count++, ext.c_str());
        filePath = fs::path(temp);
    }

    // Save
    stbi_write_png(filePath.string().c_str(), w, h, 4, bytes.get(), 0);
    Info("Save: %s", filePath.string().c_str());
}

}  // namespace glrt


#define GLRT_API_EXPORT
#include "window.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include "common.h"
#include "timer.h"
#include "vertex_array_object.h"
#include "shader_program.h"

static void error_callback(int err, const char *desc) {
    printf("GLFW error: %s (%d)\n", desc, err);
}

namespace glrt {

Window::Window(const std::string &title, int width, int height) {
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
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
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

void Window::mainloop(double fps) {
    // User's initialization
    initialize();


    // Update window size (for Apple's Letina display)
    int renderBufferWidth, renderBufferHeight;
    glfwGetFramebufferSize(window_, &renderBufferWidth, &renderBufferHeight);
    glViewport(0, 0, renderBufferWidth, renderBufferHeight);

    // Mainloop
    timer.start();
    double duration;
    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        // Handle events
        glfwPollEvents();

        // Draw current frame
        if (timer.count() >= 1.0 / fps) {
            // ImGui
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Status");
            ImGui::Text("Vendor: %s", glGetString(GL_VENDOR));
            ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
            ImGui::Text("OpenGL: %s", glGetString(GL_VERSION));
            ImGui::Text("GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
            ImGui::Text("FPS: %7.3f", 1.0 / duration);
            ImGui::Text("Size: %d x %d", width(), height());
            ImGui::End();

//            ImGui::Begin("Capture frame");
//            static char temp[256] = "output.png";
//            ImGui::InputText(": file name", temp, IM_ARRAYSIZE(temp));
//
//            outFile = std::string(temp);
//            if (ImGui::Button("Capture")) {
//                saveCurrentFrame(outFile);
//            }
            ImGui::End();
            ImGui::Render();

            int screenWidth, screenHeight;
            glfwMakeContextCurrent(window_);
            glfwGetFramebufferSize(window_, &screenWidth, &screenHeight);
            glViewport(0, 0, screenWidth, screenHeight);

            render();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwMakeContextCurrent(window_);
            glfwSwapBuffers(window_);

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

    vao = std::make_shared<VertexArrayObject>();

    program = std::make_shared<ShaderProgram>();
    program->create();
    program->attachShader(ShaderStage::fromFile("screen.vert", ShaderType::Vertex));
    program->attachShader(ShaderStage::fromFile("screen.frag", ShaderType::Fragment));
    program->link();
}

void Window::render() {
    glm::vec3 org = glm::vec3(50.0f, 52.0f, 295.6f);
    glm::vec3 dir = glm::vec3(0.0f, -0.042612f, -1.0f);
    glm::mat4 camMat = glm::lookAt(org + 140.0f * dir,
                                   org + 141.0f * dir,
                                   glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projMat = glm::perspective(glm::radians(70.0f), (float)width() / (float)height(), 1.0f, 100.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program->start();
    vao->bind();

    program->setMatrix4x4("u_mvMat", camMat);
    program->setMatrix4x4("u_projMat", projMat);
    program->setUniform1f("u_time", (float)timer.count());
    program->setUniform2f("u_windowSize", glm::vec2((float)width(), (float)height()));

    glDrawArrays(GL_TRIANGLES, 0, 6);

    vao->unbind();
    program->end();
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

}  // namespace glrt


#pragma once

#include <string>
#include <memory>

#include "api.h"
#include "common.h"
#include "event.h"
#include "timer.h"
#include "scene.h"

namespace glrt {

class GLRT_API Window {
public:
    Window();
    void mainloop(const std::shared_ptr<Scene> &scene, double fps = -1.0);

    inline int width() const {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        return width;
    }

    inline int height() const {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        return height;
    }

protected:
    virtual void initialize();
    virtual void render();
    virtual void resize(int width, int height) { resizeDefault(width, height); }
    virtual void mouse(const MouseEvent &ev) {}
    virtual void keyboard(int key, int scancode, int action, int mods) {}

private:
    void resizeDefault(int width, int height);
    void mouseDefault(int button, int action, int mods);
    void keyboardDefault(int key, int scancode, int action, int mods);
    void cursorPosDefault(double xpos, double ypos);

    void resetBuffer();
    void saveCurrentFrame(const std::string &filename, bool overwrite = true) const;

    GLFWwindow *window_;
    MouseEvent mouseEvent;

    std::shared_ptr<VertexArrayObject> vao = nullptr;
    std::shared_ptr<FramebufferObject> fbo[2] = { 0 };
    std::shared_ptr<ShaderProgram> screenProgram = nullptr;
    std::shared_ptr<ShaderProgram> rtProgram = nullptr;
    int trials = 0;
    Timer timer;

    std::shared_ptr<Scene> scene = nullptr;
};

}  // namespace glrt

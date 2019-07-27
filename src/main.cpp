#include <iostream>

#include "window.h"
using namespace glrt;

int main(int argc, char **argv) {
    Window window("OpenGL Ray Tracer", 1024, 1024);
    window.mainloop(60.0);
}

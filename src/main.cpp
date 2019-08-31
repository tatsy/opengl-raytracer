#include <iostream>
#include <memory>

#include "core/window.h"
#include "core/argparse.h"
#include "core/scene.h"
using namespace glrt;

int main(int argc, char **argv) {
    // Parse command line arguments
    ArgumentParser &parser = ArgumentParser::getInstance();
    parser.addArgument("-i", "--input", "", true, "Input XML file");
    if (!parser.parse(argc, argv)) {
        std::cout << parser.helpText() << std::endl;
        return 1;
    }

    // Parameters
    const std::string filename = parser.getString("input");

    // Initialize window
    auto window = std::make_unique<Window>();

    // Parse scene JSON
    auto scene = std::make_shared<Scene>();
    scene->parse(filename);

    // Start rendering
    window->mainloop(scene);
}

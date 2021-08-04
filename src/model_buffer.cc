#include "model_buffer.h"

model_buffer::model_buffer() {

}

model_buffer::~model_buffer() {

}

void model_buffer::create_mesh(std::string fileName) {
    // TODO: Read data from file

    mesh.vertices = {
        {{-0.5f, -0.5f}, {0.f, 0.f}},
        {{-0.5f, 0.5f}, {0.f, 1.f}},
        {{0.5f, 0.5f}, {1.f, 1.f}},
        {{0.5f, -0.5f}, {1.f, 0.f}}
    };

    mesh.indices = {
        0, 1, 2, 2, 3, 0
    };
}

model_buffer::render_to

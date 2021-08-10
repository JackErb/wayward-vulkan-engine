#include "controller.h"

#include <GLFW/glfw3.h>

namespace wayward {

DebugController::DebugController(wvk::WvkApplication *app) : app{app} {
    camera.transform.position = glm::vec3(2.0f, 0.0f, 0.8f);
    camera.transform.yaw = glm::radians(180.0f);

    app->setCamera(&camera);
}

DebugController::~DebugController() {

}

void DebugController::update() {
    updateCamera();
}

void DebugController::updateCamera() {
    const float moveSpeed = 0.025f;
    const float lookSpeed = 0.012f;

    glm::vec3 direction = camera.transform.direction();

    if (app->isKeyPressed(GLFW_KEY_W))
        camera.transform.position += moveSpeed * direction;
    if (app->isKeyPressed(GLFW_KEY_S))
        camera.transform.position -= moveSpeed * direction;
    if (app->isKeyPressed(GLFW_KEY_A))
        camera.transform.position += moveSpeed * glm::cross(direction, VECTOR_UP);
    if (app->isKeyPressed(GLFW_KEY_D))
        camera.transform.position -= moveSpeed * glm::cross(direction, VECTOR_UP);

    glm::vec2 mousePosition = app->getCursorPos();
    if (glm::distance(mousePosition, camera.lastMousePosition) > 2.0) {
        camera.transform.yaw  -= (camera.lastMousePosition.x - mousePosition.x) * lookSpeed;
        camera.transform.roll -= (camera.lastMousePosition.y - mousePosition.y) * lookSpeed;
    }

    camera.lastMousePosition = mousePosition;
}

}

#include "controller.h"

#include <GLFW/glfw3.h>
#include <math.h>

namespace wayward {

DebugController::DebugController(wvk::WvkApplication *app) : app{app} {
    camera.transform.position = glm::vec3(2.0f, 0.0f, 0.8f);
    camera.transform.yaw = glm::radians(180.0f);
    camera.transform.roll = glm::radians(180.0f);

    lightTransform.position = glm::vec3(2.0f, 2.0f, 2.0f);
    lightTransform.lookingTowards(glm::vec3(0.0f, 0.0f, 0.0f));

    app->setCamera(&camera);
}

DebugController::~DebugController() {

}

void DebugController::update() {
    updateCamera();
}

void DebugController::updateCamera() {
    const float moveSpeed = app->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.035f : 0.025f;
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

    if (app->getFrame() % 10 == 0) {
        logger::debug("yaw: " + std::to_string(glm::degrees(camera.transform.yaw)));
        logger::debug("roll: " + std::to_string(glm::degrees(camera.transform.roll)));
    }

    glm::vec2 mousePosition = app->getCursorPos();
    if (glm::distance(mousePosition, camera.lastMousePosition) > 2.0) {
        camera.transform.yaw  -= (camera.lastMousePosition.x - mousePosition.x) * lookSpeed;
        camera.transform.roll += (camera.lastMousePosition.y - mousePosition.y) * lookSpeed;

        // Restrict range of roll from 90 degrees to 270 degrees
        const float MIN_ROLL = glm::radians(91.0);
        const float MAX_ROLL = glm::radians(269.0);

        if (camera.transform.roll < MIN_ROLL) {
            camera.transform.roll = MIN_ROLL;
        } else if (camera.transform.roll > MAX_ROLL) {
            camera.transform.roll = MAX_ROLL;
        }
    }

    camera.lastMousePosition = mousePosition;
}

}

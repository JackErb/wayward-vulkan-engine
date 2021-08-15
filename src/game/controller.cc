#include "controller.h"

#include <GLFW/glfw3.h>
#include <math.h>


namespace wayward {

DebugController::DebugController(wvk::WvkApplication *app) : app{app} {
    /* Set up camera */
    camera.transform.position = glm::vec3(2.0, 2.0, 2.0);
    camera.transform.lookingTowards(glm::vec3(0.0, 0.0, 0.0));

    app->setCamera(&camera);


    /* Set up lights */
    float shadowMapWidth = 2.5;
    float shadowMapHeight = 2.5;
    float zNear = 0.1;
    float zFar = 10.0;

    lightTransform.position = glm::vec3(2.0, 2.0, 2.0);
    lightTransform.lookingTowards(glm::vec3(0.0, 0.0, 0.0));
    TransformMatrices lightTransformMatrices = lightTransform.orthoProjection(-shadowMapWidth/2, shadowMapWidth/2, -shadowMapHeight/2, shadowMapHeight/2, zNear, zFar);

    app->setLight(0, &lightTransformMatrices);

    loadModels();
}

DebugController::~DebugController() {

}

void DebugController::loadModels() {
    std::vector<wvk::Vertex> vertices = {
        {{-5.f, -5.f, -1.5f}, {0.f, 0.f, 1.f}, {0.f, 0.f}, 0},
        {{-5.f, 5.f, -1.5f}, {0.f, 0.f, 1.f}, {0.f, 1.f}, 0},
        {{5.f, 5.f, -1.5f}, {0.f, 0.f, 1.f}, {1.f, 1.f}, 0},
        {{5.f, -5.f, -1.5f}, {0.f, 0.f, 1.f}, {1.f, 0.f}, 0}
    };

    std::vector<uint32_t> indices = {2, 1, 0, 0, 3, 2};

    wvk::WvkDevice &device = app->getDevice();

    wvk::WvkModel *floor = new wvk::WvkModel(device, vertices, indices);
    app->addModel(floor);

    wvk::WvkModel *viking_room = new wvk::WvkModel(device, "viking_room.obj.model");
    app->addModel(viking_room);
}

void DebugController::update() {
    updateCamera();
}

void DebugController::updateCamera() {
    if (app->isKeyPressed(GLFW_KEY_TAB)) {
        app->enableCursor(!app->cursorEnabled());
    }

    const float moveSpeed = app->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.065f : 0.035f;
    const float lookSpeed = 0.012f;

    glm::vec3 direction = camera.transform.direction();

    if (app->isKeyHeld(GLFW_KEY_W))
        camera.transform.position += moveSpeed * direction;
    if (app->isKeyHeld(GLFW_KEY_S))
        camera.transform.position -= moveSpeed * direction;
    if (app->isKeyHeld(GLFW_KEY_A))
        camera.transform.position += moveSpeed * glm::cross(direction, VECTOR_UP);
    if (app->isKeyHeld(GLFW_KEY_D))
        camera.transform.position -= moveSpeed * glm::cross(direction, VECTOR_UP);


    #if 1
    if (app->getFrame() % 600 == 0)
    logger::debug("yaw: " + std::to_string(camera.transform.yaw) + ", roll: " + std::to_string(camera.transform.roll));
    #endif

    glm::vec2 mousePosition = app->getCursorPos();
    if (app->getFrame() == 0) {
        // Set initial mouse position
        camera.lastMousePosition = mousePosition;
    }

    if (!app->cursorEnabled() && glm::distance(mousePosition, camera.lastMousePosition) > 2.0) {
        camera.transform.yaw  -= (camera.lastMousePosition.x - mousePosition.x) * lookSpeed;
        camera.transform.roll -= (camera.lastMousePosition.y - mousePosition.y) * lookSpeed;

        // Restrict range of roll from 90 degrees to 270 degrees
        const float MIN_ROLL = glm::radians(-89.0);
        const float MAX_ROLL = glm::radians(89.0);

        camera.transform.capRoll();
    }

    camera.lastMousePosition = mousePosition;
}

}

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
    wvk::WvkDevice &device = app->getDevice();

    std::vector<wvk::MeshVertex> vertices = {
        {{-5.f, -5.f, -1.5f}, {0.f, 0.f, 1.f}, {0.f, 0.f}, 0},
        {{-5.f, 5.f, -1.5f}, {0.f, 0.f, 1.f}, {0.f, 1.f}, 0},
        {{5.f, 5.f, -1.5f}, {0.f, 0.f, 1.f}, {1.f, 1.f}, 0},
        {{5.f, -5.f, -1.5f}, {0.f, 0.f, 1.f}, {1.f, 0.f}, 0}
    };

    std::vector<uint32_t> indices = {2, 1, 0, 0, 3, 2};

    wvk::WvkModel *floor = new wvk::WvkModel(device, vertices, indices);
    app->addModel(floor);

    wvk::WvkModel *viking_room = new wvk::WvkModel(device, "viking_room.obj.model", 1);
    app->addModel(viking_room);

    //wvk::WvkSkeleton *skeleton = new wvk::WvkSkeleton(device, "astronaut.glb");
    //app->addSkeleton(skeleton);
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

    glm::vec2 mousePosition = app->getCursorPos();
    if (app->getFrame() == 0) {
        // Set initial mouse position
        camera.lastMousePosition = mousePosition;
    }

    if (!app->cursorEnabled() && glm::distance(mousePosition, camera.lastMousePosition) > 2.0) {
        camera.transform.yaw  -= (camera.lastMousePosition.x - mousePosition.x) * lookSpeed;
        camera.transform.roll -= (camera.lastMousePosition.y - mousePosition.y) * lookSpeed;

        // TODO : if the position difference is too large, ignore it entirely
    }

    // Cap the roll to prevent the camera from tipping over
    static const float MIN_ROLL = glm::radians(-89.5);
    static const float MAX_ROLL = glm::radians(89.5);

    if (camera.transform.roll < MIN_ROLL) camera.transform.roll = MIN_ROLL;
    if (camera.transform.roll > MAX_ROLL) camera.transform.roll = MAX_ROLL;

    camera.lastMousePosition = mousePosition;
}

}

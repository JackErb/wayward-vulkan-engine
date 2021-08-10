#pragma once

#include "../glm.h"

static glm::vec3    VECTOR_UP = glm::vec3(0.0f, 0.0f, 1.0f);
static glm::vec3 VECTOR_RIGHT = glm::vec3(0.0f, 1.0f, 0.0f);

struct TransformMatrices {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

struct Transform {
    glm::vec3 position; // position in worldspace
    float yaw;
    float roll;

    glm::vec3 direction() {
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, yaw, VECTOR_UP);
        rotationMatrix = glm::rotate(rotationMatrix, roll, VECTOR_RIGHT);

        return rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    }

    TransformMatrices perspectiveProjection(float aspectRatio) {
        static float fov = glm::radians(60.0f);
        static float zNear = 0.1f;
        static float zFar = 100.0f;

        TransformMatrices matrices{};
        matrices.model = glm::mat4(1.0f);
        matrices.view = glm::lookAt(position, position + direction(), VECTOR_UP);
        matrices.projection = glm::perspective(fov, aspectRatio, zNear, zFar);
        matrices.projection[1][1] *= -1;

        return matrices;
    }
};


struct Camera {
    glm::vec2 lastMousePosition;

    Transform transform;
};

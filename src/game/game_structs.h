#pragma once

#include "../glm.h"

#include <logger.h>

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

    // Returns angles between point and (1, 0)
    static float getAngle(glm::vec2 a) {
        static glm::vec2 b = glm::vec2(1.0f, 0.0f);
        static float bdist = 1;

        float adist = a.x * a.x + a.y * a.y;

        float cx = b.x - a.x;
        float cy = b.y - a.y;
        float cdist = cx * cx + cy * cy;

        float theta = acos((adist + bdist - cdist) / (2 * sqrt(adist) * sqrt(bdist)));
        return a.y >= 0 ? theta : -theta;
    }

    void setDirection(glm::vec3 direction) {
        yaw = getAngle(glm::vec2(direction.x, direction.y));
        roll = getAngle(glm::vec2(-direction.x, direction.z));


        glm::vec3 newd = this->direction();
        logger::debug(std::to_string(direction.x) + " " + std::to_string(direction.y) + " " + std::to_string(direction.z));
        logger::debug(std::to_string(newd.x) + " " + std::to_string(newd.y) + " " + std::to_string(newd.z));

        logger::debug("Yaw: " + std::to_string(glm::degrees(yaw)));
        logger::debug("Roll: " + std::to_string(glm::degrees(roll)));
    }

    void lookingTowards(glm::vec3 point) {
        setDirection(point - position);
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

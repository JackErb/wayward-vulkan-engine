#pragma once

#include "../glm.h"

#include <logger.h>

static glm::vec3    VECTOR_UP = glm::vec3(0.0f, 0.0f, 1.0f);
static glm::vec3 VECTOR_RIGHT = glm::vec3(0.0f, 1.0f, 0.0f);

struct TransformMatrices {
    glm::mat4 view;
    glm::mat4 projection;
};

struct Transform {
    static constexpr float MIN_ROLL = glm::radians(-89.5);
    static constexpr float MAX_ROLL = glm::radians(89.5);

    glm::vec3 position; // position in worldspace
    float yaw;
    float roll;

    glm::mat4 rotationMatrix() {
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, yaw, VECTOR_UP);
        rotationMatrix = glm::rotate(rotationMatrix, roll, VECTOR_RIGHT);

        return rotationMatrix;
    }

    glm::vec3 direction() {
        return rotationMatrix() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // Returns angles between point and (1, 0)
    static float getAngle(glm::vec2 a, glm::vec2 b) {
        //a = glm::normalize(a);
        float adist = a.x * a.x + a.y * a.y;
        if (abs(adist) < 0.001) {
            return 0.0;
        }

        //b = glm::normalize(b);
        float bdist = b.x * b.x + b.y * b.y;
        if (bdist == 0.0) {
            return 0.0;
        }

        float cx = b.x - a.x;
        float cy = b.y - a.y;
        float cdist = cx * cx + cy * cy;

        // Law of cosines
        float theta = acos((adist + bdist - cdist) / (2 * sqrt(adist) * sqrt(bdist)));
        return a.y >= 0 ? theta : -theta;
    }

    void capRoll() {
        if (roll < MIN_ROLL) roll = MIN_ROLL;
        if (roll > MAX_ROLL) roll = MAX_ROLL;
    }

    void setDirection(glm::vec3 direction) {
        float directionDist = sqrt(direction.x * direction.x + direction.y * direction.y);

        yaw = getAngle(glm::vec2(direction.x, direction.y), glm::vec2(1, 0));
        roll = getAngle(glm::vec2(directionDist, -direction.z), glm::vec2(1, 0));
        capRoll();
    }

    void lookingTowards(glm::vec3 point) {
        setDirection(point - position);
    }

    TransformMatrices perspectiveProjection(float aspectRatio) {
        static float fov = glm::radians(60.0);
        static float zNear = 0.1;
        static float zFar = 100.0;

        TransformMatrices matrices{};
        matrices.view = glm::lookAt(position, position + direction(), VECTOR_UP);
        matrices.projection = glm::perspective(fov, aspectRatio, zNear, zFar);
        matrices.projection[1][1] *= -1;

        return matrices;
    }

    TransformMatrices orthoProjection(float x1, float x2, float y1, float y2, float zNear, float zFar) {
        TransformMatrices matrices{};
        matrices.view = glm::lookAt(position, position + direction(), VECTOR_RIGHT);
        matrices.projection = glm::ortho(x1, x2, y1, y2, zNear, zFar);
        matrices.projection[1][1] *= -1;

        return matrices;
    }
};


struct Camera {
    glm::vec2 lastMousePosition;

    Transform transform;
};

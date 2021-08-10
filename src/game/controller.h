#pragma once

#include "game_structs.h"
#include "../app.h"

namespace wayward {

class DebugController {
public:
    DebugController(wvk::WvkApplication*);
    ~DebugController();

    DebugController(const DebugController&) = delete;
    DebugController& operator=(const DebugController&) = delete;

    void update();

private:
    void updateCamera();

    wvk::WvkApplication *app;

    Camera camera;
    Transform lightTransform;
};

};

#pragma once

#include "game_structs.h"
#include "../app.h"
#include "../wvk_model.h"

#include <vector>

#define MAX_MODELS 10

namespace wayward {

class DebugController {
public:
    DebugController(wvk::WvkApplication*);
    ~DebugController();

    DebugController(const DebugController&) = delete;
    DebugController& operator=(const DebugController&) = delete;

    void update();

private:
    void loadModels();
    void updateCamera();

    wvk::WvkApplication *app;

    std::vector<wvk::WvkModel> models;

    Camera camera;
    Transform lightTransform;
};

};

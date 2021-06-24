#pragma once

#include "wvk_window.h"
#include "wvk_device.h"

namespace wvk {

class WvkApplication {
  public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    WvkApplication();
    ~WvkApplication();

    WvkApplication(const WvkApplication &) = delete;
    WvkApplication &operator=(const WvkApplication &) = delete;

    void run();

  private:
    WvkWindow window{WIDTH, HEIGHT, "Hello Vulkan!"};
    WvkDevice device{window};
};

};

#include "wvk_buffer.h"
#include "wvk_device.h"
#include "wvk_model.h"
#include "anim/skeleton.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "glm.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>
#include <array>

namespace wvk {

class WvkSkeleton {
public:
    WvkSkeleton(WvkDevice& device, std::string modelFilename);
    ~WvkSkeleton();

    WvkSkeleton(const WvkSkeleton &) = delete;
    WvkSkeleton &operator=(const WvkSkeleton &) = delete;

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

private:
    void createIndexBuffer();
    void createVertexBuffer();

    WvkDevice& device;

    Buffer vertexBuffer;
    Buffer vertexStagingBuffer;

    Buffer indexBuffer;
    Buffer indexStagingBuffer;

    wvk::Skeleton skeleton;
};

}

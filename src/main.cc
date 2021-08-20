#include "app.h"

#include "resource_path.h"
#include <logger.h>


int main(int argc, char** argv) {
#if defined(__APPLE__)
    std::string appPath(argv[0]);
    const std::string contents = "Contents/";
    size_t index = appPath.rfind(contents);

    const std::string fullPath = appPath.substr(0, index + contents.size()) + "resources/";

    setResourcePath(fullPath.c_str());
#else
    setResourcePath("C:/Users/Jack/Documents/GitHub/wayward-vulkan-engine/src/out/build/x64-Debug/resources/");
#endif

    wvk::WvkApplication app;

    app.run();
}

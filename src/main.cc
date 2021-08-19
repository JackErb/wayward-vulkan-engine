#include "app.h"

#include "resource_path.h"
#include <logger.h>


int main(int argc, char** argv) {
#if defined(__APPLE__)
    std::string appPath(argv[0]);
    const std::string contents = "Contents/";
    size_t index = appPath.rfind(contents);

    setResourcePath(appPath.substr(0, index + contents.size()) + "resources/");
#else
    setResourcePath("C:/Users/Jack/Documents/GitHub/wayward-vulkan-engine/src/out/build/x64-Debug/resources/");
#endif

    wvk::WvkApplication app;

    for (size_t i = 0; i < argc; i++) {
        logger::debug(argv[i]);
    }

    app.run();
}


#ifdef __APPLE__

int main(int argc, char** argv) {
    setResourcePath(argv[0]);
    wvk::WvkApplication app;

    for (size_t i = 0; i < argc; i++) {
        logger::debug(argv[i]);
    }

    app.run();
}

#else
/*
int main(void) {
    setResourcePath("C:/Users/Jack/Documents/GitHub/wayward-vulkan-engine/src/resources/");
    wvk::WvkApplication app;

    app.run();
    return 0;
}
*/

#endif
#include "resources.h"
#include <string>

std::string gResourcePath;

void setResourcePath(const char* applicationPath) {
#if defined(__APPLE__)
    std::string appPath(applicationPath);
    const std::string contents = "Contents/";
    size_t index = appPath.rfind(contents);

    gResourcePath = appPath.substr(0, index + contents.size()) + "resources/";
#endif
}

std::string resourcePath() {
    return gResourcePath;
}

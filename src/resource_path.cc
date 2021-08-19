#include "resource_path.h"
#include <string>

static std::string gResourcePath;

void setResourcePath(const char* applicationPath) {
    gResourcePath = applicationPath;
}

std::string resourcePath() {
    return gResourcePath;
}

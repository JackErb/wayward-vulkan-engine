#include "app.h"

#include "resource_path.h"
#include <logger.h>

int main(int argc, char **argv) {
    setResourcePath(argv[0]);
    wvk::WvkApplication app;

    for (size_t i = 0; i < argc; i++) {
        logger::debug(argv[i]);
    }

    app.run();
}

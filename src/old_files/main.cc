#include "app.h"

#include "resources.h"
#include <logger.h>

int main(int argc, char **argv) {
    setResourcePath(argv[0]);
    application app;
    app.run();
}

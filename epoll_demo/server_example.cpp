#ifndef SERVER_EXAMPLE_
#define SERVER_EXAMPLE_

#include "Server.h"

int main() {
    Server srv("localhost", 8081);
    if (srv.bind() == -1) {
        return -1;
    }
    if (srv.listen() == -1) {
        return -1;
    }
    srv.loop();

    return 0;
}

#endif
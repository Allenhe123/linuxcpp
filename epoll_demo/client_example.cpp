#ifndef CLIENT_EXAMPLE_
#define CLIENT_EXAMPLE_

#include "Client.h"
#include <chrono>
#include <thread>
#include <iostream>

int main() {
    Client cli;
    cli.connect("localhost", 8081);

    for (;;)
    {
        cli.send("hello server", 12, MSG_REQ_NAME);
        auto msg = cli.recv();
        std::cout << msg.get() << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}

#endif
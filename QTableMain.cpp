#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ev++.h>
#include "TCPServer.hpp"
#include "Logger.h"
#include "Shared.h"

using namespace std;
CREATE_LOGGER;

DirectMap ids_map;

int main() {
    INIT_LOGGER;
    FILE *file = fopen("table", "rb");
    char buffer[4096];
    uint64_t keyseq = 0;
    uint64_t dup = 0;
    while(fread(buffer, 4096, 1, file)){
        for (int i = 0; i < 1024; i++){
            keyseq++;
            uint32_t data = *(uint32_t *)(buffer+(i*4));
            ids_map.insert(std::make_pair(data, keyseq));
        }
    }
    LOG(INFO) << "Import completed: " << keyseq << " items. Dup: " << dup;
    fclose(file);
#ifdef __APPLE__
    setenv("LIBEV_FLAGS", "8", 1);
#endif
    ev::default_loop loop;
    TCPServer serv;
    loop.run();
    return 0;
}

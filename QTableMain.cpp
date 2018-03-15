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
    FILE *file = fopen("data/biliquery.bin", "rb");
    if(!file){
        LOG(WARNING) << "Database not found, recreating...";
        // Load all data to map
        DirectMap temp_map;
        FILE *file = fopen("data/table", "rb");
        char buffer[4096];
        uint64_t keyseq = 0;
        while(fread(buffer, 4096, 1, file)){
            for (int i = 0; i < 1024; i++){
                keyseq++;
                uint32_t data = *(uint32_t *)(buffer+(i*4));
                temp_map.insert(std::make_pair(data, keyseq));
            }
            printf("Please wait... %lld\r",keyseq);
            fflush(stdout);
        }
        LOG(INFO) << "Import completed: " << keyseq << " items";
        fclose(file);
        FILE *db = fopen("data/biliquery.bin", "wb");
        FILE *dups = fopen("data/duplicate.bin", "wb");

        uint32_t uint32_min = 0x00000000;
        uint32_t uint32_max = 0xFFFFFFFF;

        for(uint32_t i = 0;i < uint32_max;i++){
            auto count = temp_map.count(i);
            if(!count){
                fwrite(&uint32_min,4,1,db);
                continue;
            }else if(count > 1){
                LOG(INFO) << "Write four byte duplicate for " << i;
                fwrite(&uint32_max,4,1,db);
                continue;
            }
            auto its = temp_map.equal_range(i);
            for (auto it = its.first; it != its.second; ++it) {
                if(count == 1){
                    // Write directly to file
                    fwrite(&it->second,4,1,db);
                }else{
                    // Duplicated item
                    fwrite(&it->first,4,1,dups);
                    fwrite(&it->second,4,1,dups);
                }
            }
            if(i % 10000 == 0){
                printf("Please wait... %u/%u\r",i,uint32_max);
                fflush(stdout);
            }
        }
        LOG(INFO) << "Database created";
        fclose(db);
        fclose(dups);
    }else{
        fclose(file);
    }
    

    LOG(INFO) << "Trying to load duplicate database";

    file = fopen("data/duplicate.bin", "rb");
    char buffer[8];
    uint64_t keyseq = 0;
    while(fread(buffer, 8, 1, file)){
        keyseq++;
        uint32_t crc32 = *(uint32_t *)buffer;
        uint32_t uid = *(uint32_t *)(buffer+4);
        ids_map.insert(std::make_pair(crc32, uid));
    }
    LOG(INFO) << "Import completed: " << keyseq << " items";
    fclose(file);

#ifdef __APPLE__
    setenv("LIBEV_FLAGS", "8", 1);
#endif
    ev::default_loop loop;
    TCPServer serv;
    loop.run();
    return 0;
}

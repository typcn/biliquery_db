//
//  Responder.cpp
//  TYDB
//
//  Created by TYPCN on 2016/9/19.
//
//

#include "Responder.hpp"
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sstream>
#include <string.h>
#include <execinfo.h>
#include "Logger.h"
#include "Shared.h"

#define TRACE_BUFSIZE 1000
#define MAX_URL_LEN 32
#define HTTP_BANNER "Server: TYPCN-API\r\n"


uint32_t *dup_block;
size_t dup_block_size;
FILE *db_file;
size_t db_file_size;

Responder::Responder(ConnHandler *hdl) : handler(hdl) {
    db_file = fopen("data/biliquery.bin", "rb");
    db_file_size = lseek(fd, 0, SEEK_END);
    
    int fd = open("data/duplicate.bin", O_RDONLY);
    dup_block_size = lseek(fd, 0, SEEK_END);
    dup_block = (uint32_t *)mmap(NULL,dup_block_size, PROT_READ, MAP_PRIVATE, fd, 0);
}

void Responder::print_error(const char *msg){
    char st_text[TRACE_BUFSIZE];
    int wlen = snprintf(st_text, TRACE_BUFSIZE, "HTTP/1.0 500\r\n" HTTP_BANNER "\r\nError: %s\tConnection: %p\nStack trace:",msg,(void *)handler);
    void* callstack[8];
    int i, frames = backtrace(callstack, 8);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        wlen++;
        strncpy(st_text + wlen, strs[i], TRACE_BUFSIZE - wlen);
        wlen = strlen(st_text);
        if(TRACE_BUFSIZE - wlen < 1){
            st_text[TRACE_BUFSIZE-1] = 0x0;
            st_text[TRACE_BUFSIZE-2] = '.';
            st_text[TRACE_BUFSIZE-3] = '.';
            st_text[TRACE_BUFSIZE-4] = '.';
            break;
        }
        st_text[wlen] = '\n';
    }
    handler->send((uint8_t *)st_text, strlen(st_text));
    free(strs);
}

int BinarySearch(uint32_t value){  
    uint32_t len = dup_block_size/8;
    uint32_t low = 0;  
    uint32_t high = len - 1;  
    while(low <= high){  
        uint32_t mid = low + (high - low) / 2;  
        if (dup_block[mid*2] == value)  
            return mid*2;  
        else if (dup_block[mid*2] > value)  
            high = mid - 1;  
        else  
            low = mid + 1;  
    }
    return -1;  
}  

#define END(msg) print_error(msg);free(data);handler->shutdown();return;
#define CUT free(data);handler->shutdown();return;

void Responder::send_result(uint8_t *data, int len){
    if(len < 13){
        END("Request length invalid");
    }
    // Step 1, Get Request Content
    data[len] = 0x0;
    char *req;
    if(data[0] == 'G' && data[1] == 'E' && data[2] == 'T'){
        char *ptr = (char *)&data[5];
        char *pch = strstr(ptr, "HTTP/");
        if(!pch || (pch - ptr) < 2){
            END("Cannot extract request path");
        }
        (pch-1)[0] = 0x0;
        req = ptr;
    }else{
        CUT;
    }

    // Step 2 , Convert request string
    int url_len = strlen(req);
    if(url_len < 1 || url_len > 8){
        END("Request is not 32bit unsigned int");
    }
    
    uint8_t req_b[] = "00000000";
    memcpy(req_b+(8-url_len),req,url_len);
    
    uint8_t ubuf[4];
    ubuf[0] = (hexmap[req_b[0]] << 4) + hexmap[req_b[1]];
    ubuf[1] = (hexmap[req_b[2]] << 4) + hexmap[req_b[3]];
    ubuf[2] = (hexmap[req_b[4]] << 4) + hexmap[req_b[5]];
    ubuf[3] = (hexmap[req_b[6]] << 4) + hexmap[req_b[7]];

    uint32_t requested_key = *(uint32_t *)&ubuf;

    char *resp;
    if(requested_key > 0){
        if(((uint64_t)requested_key)*4 > db_file_size){
            END("Request out of index");
        }
        fseek(db_file, ((uint64_t)requested_key)*4 ,SEEK_SET);
        uint32_t query_res;
        int nread = fread(&query_res,4,1,db_file);
        if(!nread || query_res == 0){
            resp = (char *)"HTTP/1.0 200 OK\r\n" HTTP_BANNER "\r\n{\"error\":1}";
        }else if(query_res == 0xFFFFFFFF){

            int pos = BinarySearch(requested_key);
            if(pos == -1){
                LOG(ERROR) << "Cannot search for key";
                CUT;
            }

            char resp_buf[1024] = "HTTP/1.0 200 OK\r\n" HTTP_BANNER "\r\n{\"error\":0,\"data\":[";
            size_t wpos = strlen(resp_buf);

            while(1){
                if(pos >= dup_block_size/4){
                    break;
                }
                uint32_t crc32 = dup_block[pos];
                uint32_t uid = dup_block[pos+1];
                if(crc32 != requested_key){
                    break;
                }
                wpos += sprintf(resp_buf+wpos, "{\"id\":%u},",uid);
                pos += 2;
            }
            memcpy(resp_buf+wpos-1,"]}\0",3);
            resp = resp_buf;

        }else{
            char resp_buf[128];
            const char *fmt = "HTTP/1.0 200 OK\r\n" HTTP_BANNER "\r\n{\"error\":0,\"data\":[{\"id\":%u}]}";
            sprintf(resp_buf,fmt,query_res);
            resp = resp_buf;
        }
    }else{
        resp = (char *)"HTTP/1.0 400\r\n" HTTP_BANNER "\r\nBad request";
    }
    handler->send((uint8_t *)resp, strlen(resp));
    handler->shutdown();
    free(data);
    return;
}

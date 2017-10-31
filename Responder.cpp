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
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sstream>
#include <string.h>
#include <execinfo.h>
#include "Logger.h"
#include "Shared.h"

#define TRACE_BUFSIZE 1000
#define MAX_URL_LEN 32
#define HTTP_BANNER "Server: TYPCN-API\r\n"

Responder::Responder(ConnHandler *hdl) : handler(hdl) {

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
    if(url_len !=8){
        END("Request is not 32bit unsigned int");
    }
    uint8_t ubuf[4];
    ubuf[0] = (hexmap[req[0]] << 4) + hexmap[req[1]];
    ubuf[1] = (hexmap[req[2]] << 4) + hexmap[req[3]];
    ubuf[2] = (hexmap[req[4]] << 4) + hexmap[req[5]];
    ubuf[3] = (hexmap[req[6]] << 4) + hexmap[req[7]];

    uint32_t requested_key = *(uint32_t *)&ubuf;

    char *resp;
    if(requested_key > 0){
        auto its = ids_map.equal_range(requested_key);
        if(its.first == its.second){
            resp = (char *)"HTTP/1.0 200 OK\r\n" HTTP_BANNER "\r\n{\"error\":1}";
        }else{
            char resp_buf[1024] = "HTTP/1.0 200 OK\r\n" HTTP_BANNER "\r\n{\"error\":0,\"data\":[";
            size_t pos = strlen(resp_buf);
            for (auto it = its.first; it != its.second; ++it) {
                LOG(INFO) << it->first << " " << it->second;
                pos += sprintf(resp_buf+pos, "{\"id\":%llu},",it->second);
            }
            memcpy(resp_buf+pos-1,"]}\0",3);
            resp = resp_buf;
        }
    }else{
        resp = (char *)"HTTP/1.0 400\r\n" HTTP_BANNER "\r\nBad request";
    }
    handler->send((uint8_t *)resp, strlen(resp));
    handler->shutdown();
    return;
}

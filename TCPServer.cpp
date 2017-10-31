//
//  TCPServer.cpp
//  TYDB
//
//  Created by TYPCN on 2016/9/18.
//
//

#include <stdio.h>
#include <ev++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "TCPServer.hpp"
#include "ConnHandler.hpp"
#include "Logger.h"

void TCPServer::io_accept(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
        LOG(WARNING) << "Invalid event " << revents;
        return;
    }
    
    sockaddr *incoming_addr;
    socklen_t incoming_len;
    
    if(af == AF_INET){
        incoming_len = sizeof(sockaddr_in);
        incoming_addr = (sockaddr *)malloc(incoming_len);
    }else{
        incoming_len = sizeof(sockaddr_in6);
        incoming_addr = (sockaddr *)malloc(incoming_len);
    }
    
    int client_sd = accept(watcher.fd, incoming_addr, &incoming_len);
    
    ConnHandler *hdl = new ConnHandler(client_sd);
    LOG(VERBOSE) << (void *)hdl << ": New connection " << client_sd;
    
    free(incoming_addr);
}

TCPServer::TCPServer(){

    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "zt0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
#ifdef __APPLE__
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#else
    addr.sin_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
#endif
    addr.sin_port =  htons((unsigned short)6071);
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int enable = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) (void *) &enable, sizeof(int));
    
    if (::bind(fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
        LOG(FATAL) << "Failed to bind, " << strerror(errno);
        exit(-1);
        return;
    }
    
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    listen(fd, 0);
    
    io.set<TCPServer, &TCPServer::io_accept>(this);
    io.start(fd, ev::READ);
    
    sio.set<&TCPServer::signal_cb>();
    sio.start(SIGINT);
    
    af = AF_INET;
    
    LOG(INFO) << "Server listening on 6071";
}

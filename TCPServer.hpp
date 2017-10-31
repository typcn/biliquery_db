//
//  TCPServer.hpp
//  TYDB
//
//  Created by TYPCN on 2016/9/18.
//
//

#ifndef TCPServer_hpp
#define TCPServer_hpp

#include <stdio.h>
#include <unistd.h>
#include <vector>


using namespace std;

class TCPServer {
private:
    ev::io io;
    ev::sig sio;
    int fd;
    int af;
    socklen_t addrlen;
    
public:
    
    TCPServer();
    void io_accept(ev::io &watcher, int revents);
    void on_data(int fd, const uint8_t *buf, uint32_t len, const struct sockaddr* addr, socklen_t addrlen);
    void on_event(int fd, int code, const char* msg, const struct sockaddr* addr, socklen_t addrlen);
    
    static void signal_cb(ev::sig &signal, int revents) {
        signal.loop.break_loop();
    }
    
    virtual ~TCPServer() {
        io.stop();
        sio.stop();
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }
};

#endif /* TCPServer_hpp */

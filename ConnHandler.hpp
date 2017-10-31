//
//  ConnHandler.hpp
//  TYDB
//
//  Created by TYPCN on 2016/9/19.
//
//

#ifndef ConnHandler_hpp
#define ConnHandler_hpp

#include <stdio.h>
#include <stdio.h>
#include <ev++.h>
#include <list>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "SpinLock.h"

struct Buffer{
    uint8_t *data;
    int32_t len;
    int32_t wrote_len;
};

class ConnHandler {
private:
    ev::io io_read;
    ev::io io_write;
    int sfd;
    int Buffer_size;
    
    bool is_writeable;
    pthread_spinlock_t writeable_lock;
    
    std::list<Buffer *> write_queue;
    pthread_mutex_t queue_lock;
    
    void write_cb(ev::io &watcher, int revents);
    void read_cb(ev::io &watcher, int revents);
    
    virtual ~ConnHandler() {
        io_read.stop();
        io_write.stop();
        close(sfd);
        for (std::list<Buffer *>::iterator it=write_queue.begin(); it != write_queue.end(); ++it){
            Buffer *b = *it;
            free(b->data);
            free(b);
        }
        pthread_spin_destroy(&writeable_lock);
        pthread_mutex_destroy(&queue_lock);
    }
    
public:
    bool alive;
    
    ConnHandler(int s);
    void add_queue(const uint8_t *data, int len);
    void send(const uint8_t *data, int len);
    void shutdown();
};

#endif /* ConnHandler_hpp */

//
//  ConnHandler.cpp
//  TYDB
//
//  Created by TYPCN on 2016/9/19.
//
//

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <string.h>
#include "ConnHandler.hpp"
#include "Responder.hpp"
#include "Logger.h"

ConnHandler::ConnHandler(int s) : sfd(s) {
    alive = true;
    is_writeable = true;
initmtxlock:
    int rv = pthread_mutex_init(&queue_lock,  NULL);
    if(rv != 0){
        if(errno == EAGAIN){
            goto initmtxlock;
        }
    }

initspinlock:
    rv = pthread_spin_init(&writeable_lock, PTHREAD_PROCESS_PRIVATE);
    if(rv != 0){
        if(errno == EAGAIN){
            goto initspinlock;
        }
    }
    
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
    io_read.set<ConnHandler, &ConnHandler::read_cb>(this);
    io_read.start(s, ev::READ);
    io_write.set<ConnHandler, &ConnHandler::write_cb>(this);
    
    Buffer_size = sizeof(Buffer);
}

void ConnHandler::read_cb(ev::io &watcher, int revents) {
    uint8_t *data = (uint8_t *)malloc(1000);
    
    ssize_t nread = recv(watcher.fd, data, 1000, 0);
    
    if(nread < 0){
        free(data);
        LOG(WARNING) << (void *)this << ": Read fd " << watcher.fd << " failed with errno " << errno;
        return;
    }else if(nread == 0){
        free(data);
        LOG(VERBOSE) << (void *)this << ": Closing connection " << watcher.fd;
        this->shutdown();
    }else{
        LOG(VERBOSE) << (void *)this << ": Data read complete";
        Responder hdl(this);
        hdl.send_result(data,nread);
    }
}

void ConnHandler::write_cb(ev::io &watcher, int revents) {
    pthread_mutex_lock(&queue_lock);
    
    while (!write_queue.empty()) {
        Buffer* buffer = write_queue.front();
        
        ssize_t written = ::send(sfd, (const char *) buffer->data + buffer->wrote_len, buffer->len - buffer->wrote_len, 0);
        if (written < 0) {
            LOG(WARNING) << "Send fd " << watcher.fd << " failed with errno " << errno;
            return;
        }
        
        buffer->wrote_len += written;
        
        if (buffer->wrote_len != buffer->len) {
            pthread_mutex_unlock(&queue_lock);
            return;
        }
        
        write_queue.pop_front();
        free(buffer->data);
        free(buffer);
    }
    
    pthread_mutex_unlock(&queue_lock);
    io_write.stop();
    pthread_spin_lock(&writeable_lock);
    is_writeable = true;
    pthread_spin_unlock(&writeable_lock);
}


void ConnHandler::send(const uint8_t *data, int len){
    pthread_spin_lock(&writeable_lock);
    bool writeable = is_writeable;
    pthread_spin_unlock(&writeable_lock);
    if(writeable){
        // 可写直接发送
        ssize_t written = ::send(sfd, (const char *) data, len, 0);
        if(written < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // Buffer 满了，设置为不可写
                pthread_spin_lock(&writeable_lock);
                is_writeable = false;
                pthread_spin_unlock(&writeable_lock);
                // 加入队列并等待可写事件回调
                LOG(VERBOSE) << (void *)this << ": Blocked, Queued up packet";
                this->add_queue(data, len);
                io_write.start(sfd, ev::WRITE);
            }else{
                LOG(VERBOSE) << (void *)this << ": Connection closed " << sfd;
                this->shutdown();
            }
        }else if(written < len){
            // 只发了一部分
            pthread_spin_lock(&writeable_lock);
            is_writeable = false;
            pthread_spin_unlock(&writeable_lock);
            this->add_queue(data + written, len - written);
            io_write.start(sfd, ev::WRITE);
        }
    }else{
        this->add_queue(data, len);
    }
}

void ConnHandler::add_queue(const uint8_t *data, int len){
    pthread_mutex_lock(&queue_lock);
    void *mem = malloc(len);
    memcpy(mem, data, len);
    Buffer *buf = (Buffer *)malloc(Buffer_size);
    buf->data = (uint8_t *)mem;
    buf->wrote_len = 0;
    buf->len = len;
    write_queue.push_back(buf);
    pthread_mutex_unlock(&queue_lock);
}

void ConnHandler::shutdown(){
    LOG(VERBOSE) << (void *)this << ": Shutdown connection " << sfd;
    alive = false;
    ::shutdown(sfd, SHUT_RDWR);
    delete this;
}

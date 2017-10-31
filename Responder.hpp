//
//  Responder.hpp
//  TYDB
//
//  Created by TYPCN on 2016/9/19.
//
//

#ifndef Responder_hpp
#define Responder_hpp

#include <stdio.h>
#include <string>
#include "ConnHandler.hpp"

class Responder{
private:
    ConnHandler *handler;
    void print_error(const char *msg);
    int do_base64_decode_avx2(const char *msg, char *out);
    
public:
    Responder(ConnHandler *hdl);
    void send_result(uint8_t *data, int len);
};

#endif /* Responder_hpp */

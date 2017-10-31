#ifndef logging_hpp
#define logging_hpp
#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <sstream>
#define FATAL 1
#ifdef ERROR
#undef ERROR
#endif
#define ERROR 2
#define WARNING 3
#define INFO 4
#define VERBOSE 5
class logger {
    int level;
    std::stringstream buffer;
public:
    static pthread_mutex_t mutex;
    logger(int level) : level(level) {
        if (level == 1) {
            buffer << "FATAL ";
        } else if (level == 2) {
            buffer << "ERROR ";
        } else if (level == 3) {
            buffer << "WARN  ";
        } else if (level == 4) {
            buffer << "INFO  ";
        } else if (level == 5) {
            buffer << "VERB  ";
        }
    }
    template <typename T>
    logger& operator<<(const T& value) {
        buffer << value;
        return *this;
    }
    ~logger() {
        pthread_mutex_lock(&mutex);
        std::cout << buffer.str() << std::endl << std::flush;
        if (level == 1) {
            std::cout << "Program ended due the fatal log" << std::endl << std::flush;
            abort();
        }
        pthread_mutex_unlock(&mutex);
    }
};
#define CREATE_LOGGER pthread_mutex_t logger::mutex;
#define INIT_LOGGER pthread_mutex_init(&logger::mutex, NULL);

#ifndef DEBUG
#define LOG(LEVEL) if (LEVEL < 5) logger(LEVEL)
#else
#define LOG(LEVEL) logger(LEVEL)
#endif

#endif

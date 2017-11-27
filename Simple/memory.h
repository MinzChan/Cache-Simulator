#ifndef CACHE_MEMORY_H_
#define CACHE_MEMORY_H_

#include <stdint.h>
#include "storage.h"

#define MAX 100000000

class Memory: public Storage {
    public:
        Memory(){
            _visit_cnt = 0;
        }
        ~Memory(){ }
        int _visit_cnt;
        // Main access process
        void HandleRequest(uint64_t addr, int bytes, int read, char *content, int &hit, int &time);

    private:
        // Memory implement
//         unsigned char _memory[MAX]={0};
        DISALLOW_COPY_AND_ASSIGN(Memory);
};

#endif //CACHE_MEMORY_H_ 
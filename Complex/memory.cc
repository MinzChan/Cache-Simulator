#include <iostream>
#include <iomanip>
#include "memory.h"
#include "def.h"
using namespace std;

void Memory::HandleRequest(uint64_t addr, int byte_num, int read_or_write,
                           char *content, int &hit, int &time, int calculate_time) {
    hit = 1;
    _visit_cnt += 1;
//    cout << "memory request: " << dec << addr << endl;
    
    if(calculate_time){
        time = _latency.hit_latency + _latency.bus_latency;
        _stats.access_time += time;
    }
    
    if(read_or_write == READ){
//        memcpy((void*)content, (const void*)(_memory + addr), byte_num);
    }
    else if(read_or_write == WRITE){
//        memcpy((void*)(_memory + addr), (const void*)content, byte_num);
    }
    else{
        cerr << "[Memory]: Invalid operation. Expected 'READ' or 'WRITE'." << endl;
    }
}


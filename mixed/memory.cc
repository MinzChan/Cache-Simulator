#include <iostream>
#include <iomanip>
#include "memory.h"
#include "def.h"
using namespace std;

void Memory::HandleRequest(uint64_t addr, int byte_num, int read_or_write, char *content, int &hit, int &time) {
    hit = 1;
    _visit_cnt += 1;
    time = _latency.hit_latency + _latency.bus_latency;
    _stats.access_time += time;
    
//    cout << hex << *((unsigned int*)(&_memory[0x101ac])) << endl;
    if(read_or_write == READ){
        memcpy((void*)content, (const void*)(_memory + addr), byte_num);
//        cout << "Reading from memory. Size: " << byte_num<< endl;
//        cout << "addr: "<< hex << addr;
//        cout << endl;
    }
    else if(read_or_write == WRITE){
        memcpy((void*)(_memory + addr), (const void*)content, byte_num);
    }
    else{
        cerr << "[Memory]: Invalid operation. Expected 'READ' or 'WRITE'." << endl;
    }
}


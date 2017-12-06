#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include "def.h"
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;

/* Memory and Cache */
Memory memory;
Cache level1, level2;

/* Counting */
int hit_cnt = 0;
int request_cnt = 0;

/* Configure memory and cache */
void Configure(){
    level1.SetLower(&level2);
    level2.SetLower(&memory);
    level1.SetMem(&memory);
    level2.SetMem(&memory);
    
    StorageStats s;
    s.access_time = 0;
    memory.SetStats(s);
    level1.SetStats(s);
    level2.SetStats(s);
    
    StorageLatency ml;
    ml.bus_latency = 0;
    ml.hit_latency = 100;
    memory.SetLatency(ml);
    
    StorageLatency l1, l2, l3;
    CacheConfig cfg1, cfg2, cfg3;
    l1.bus_latency = 0;
    l1.hit_latency = 1;
    cfg1.size = (1 << 15);
    cfg1.associativity = 8;
    cfg1.line_size = 64;
    cfg1.write_policy = WRITE_BACK;
    cfg1.write_allocate_policy = WRITE_ALLOCATE;
    cfg1.replace_policy = LFU;
    cfg1.prefetch_num = 0;
    
    l2.bus_latency = 6;
    l2.hit_latency = 8;
    cfg2.size = (1 << 18);
    cfg2.associativity = 8;
    cfg2.line_size = 64;
    cfg2.write_policy = WRITE_BACK;
    cfg2.write_allocate_policy = WRITE_ALLOCATE;
    cfg2.replace_policy = LFU;
    cfg2.prefetch_num = 4;
    
    level1.SetLatency(l1);
    level2.SetLatency(l2);
    
    level1.SetConfig(cfg1);
    level2.SetConfig(cfg2);
    
    /* build cache */
    level1.BuildCache();
    level2.BuildCache();
}

void HandleTrace(const char* trace){
    int hit, time;  // used to collect results
    uint64_t addr;
    char content[64];  // write content
    string request;
    ifstream fin;
    
    fin.open(trace);
    while(fin >> request >> hex >> addr){
        if(request == "r"){
            level1.HandleRequest(addr, 4, READ, content, hit, time, YES);
        }
        else{
            level1.HandleRequest(addr, 4, WRITE, content, hit, time, YES);
        }
        request_cnt += 1;
        hit_cnt += hit;
    }
    fin.close();
}

int main(int argc, char * argv[]) {
    /* get file name from command */
    if(argc < 2){
        cerr << "Missing operand. Please specify the trace." << endl;
        return 0;
    }
    else if(argc > 2){
        cerr << "Too many operands. Please specify the trace only." << endl;
        return 0;
    }
    const char* trace = argv[1];
                               
    Configure();
    HandleTrace(trace);
    
    /* Used to tell the difference between three write policies */
//    for(int i = 6;i <= 10; ++i){
//        cout << "line_size: " << (1 << i) << endl;
//        level1.ReleaseCache();
//        ConfigureCache(1 << i);
//        level1.BuildCache();
//        HandleTrace(trace);
//        cout << "Miss Rate: " << 1 - (float)hit_cnt / request_cnt << endl;
//        StorageStats s;
//        int rst = 0;
//        level1.GetStats(s);
//        printf("Total l1 access time: %dns\n", s.access_time);
//        rst += s.access_time;
//        memory.GetStats(s);
//        printf("Total Memory access time: %dns\n", s.access_time);
//        rst += s.access_time;
//        cout << "ACCESS TIME: "<< rst << endl;
//        cout << "---------" << endl;
//    }
    
//    StorageStats s;
//    level1.GetStats(s);
    printf("Total L1 access time: %dns\n", s.access_time);
//    memory.GetStats(s);
//    printf("Total Memory access time: %dns\n", s.access_time);
    printf("Total Memory access cnt: %d\n", memory._visit_cnt);
    cout << "Miss Rate: " << 1 - (float)hit_cnt / request_cnt << endl;
//    cout << "Miss time: " << request_cnt - hit_cnt << endl;
    return 0;
}

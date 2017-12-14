#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <iostream>
#include <stdint.h>
#include <set>
#include <string>
#include "storage.h"
using namespace std;

/*
 A cache with 'size' bytes, 'set_num' sets.
 Each set has 'associativity' lines.
 Each line has 'line_size' bytes.
 size = set_num * associativity * line_size
 */
typedef struct CacheConfig_ {
    int size;  // bytes
    int associativity;  // Number of ways: how many lines in one set
    int set_num; // Number of cache sets
    int line_size;
    int write_policy; // 0|1 for back|through
    int write_allocate_policy; // 0|1 for no-alc|alc
    int replace_policy;  // 0|1 for LRU|LFU
    int prefetch_num;  // how many lines to prefetch? 0: no prefetch
} CacheConfig;

typedef struct CacheLine_{
    int valid;
    int modified;
    uint64_t tag;
    int time_stamp;  // last visit
    int visit_cnt;  // how many times it is visited
    int IRR;  // Inter-reference recency
    int recency;
    char* line;
    set<uint64_t> visited_lines;  // the starting addr of the lines that have been visited between two adjesent visit to the same line
}CacheLine;

typedef struct CacheSet_{
    set<uint64_t> visited_tags;  // the tags of the lines that have been visited
    CacheLine* cache_lines;
}CacheSet;

// Splitting address
typedef struct CacheAddress_{
    uint64_t tag;
    int set;
    int offset;
    int index; // the index of the required line in that set
} CacheAddress;

class Cache: public Storage {
public:
    Cache(string name){
        /* initialize time stamp */
        _time_stamp = 0;
        _total_hit = 0;
        _total_visit = 0;
        _name = name;
    }

    ~Cache(){
        ReleaseCache();
    }

    // Sets & Gets
    void SetConfig(CacheConfig cc);
    void GetConfig(CacheConfig cc);
    void BuildCache();
    void ReleaseCache();
    void SetLower(Storage *ll) { _lower = ll; }
    void SetMem(Storage *ll) { _mem = ll; }  // for Bypass
    // Main access process
    void HandleRequest(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time, int calculate_time);
    double CalculateMissRate() {
        return 1 - (double)_total_hit / _total_visit;
    }
    double AMAT();

private:
    CacheAddress SetAddrInfo(uint64_t addr);
    uint64_t getbit(uint64_t addr,int s,int e);
    int CacheHit(CacheAddress& addr_info);
    int FoundEmptyLine(CacheAddress& addr_info);
    void FindReplacement(CacheAddress& addr_info);
    void FindLRU(CacheAddress& addr_info);
    void FindLFU(CacheAddress& addr_info);
    void FindLIRS(CacheAddress& addr_info);
    void ReplaceLine(CacheAddress& addr_info, char* new_line, int& time);
    void ReadFromCache(CacheAddress& addr_info, int byte_num, char* content);
    void WriteToCache(CacheAddress& addr_info, int byte_num, char* content);
    CacheLine* GetCacheLine(CacheAddress& addr_info);
    CacheSet* GetCacheSet(CacheAddress& addr_info);
    void PrintSet(CacheAddress& addr_info);
    void FinalCheck();
    void PrefetchStrategy(uint64_t addr);
    int BypassCondition(CacheAddress& addr_info);
    void LoadLineFromLower(uint64_t addr, CacheAddress& addr_info, int& lower_hit, int& lower_time, int calculate_time);
    void InitializeLine(CacheLine* line);
    void SetLineValid(CacheLine* line, CacheAddress& addr_info);
    void VisitLine(CacheLine* line);
    uint64_t GetAddrOfLine(CacheAddress& addr_info);
    void UpdateLIRS(CacheAddress& addr_info);
    void UpdateAllLIRS(CacheAddress& addr_info);
    
    CacheSet* _cache;
    CacheConfig _config;
    Storage *_lower;
    Storage *_mem;
    DISALLOW_COPY_AND_ASSIGN(Cache);
    int _tag_bit;
    int _offset_bit;
    int _set_bit;
    uint64_t _time_stamp;  // used for counting
    int _total_visit;  // total visit in current cache
    int _total_hit;  // total hit cnt in current cache
    string _name;
};

#endif //CACHE_CACHE_H_ 

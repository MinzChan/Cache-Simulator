#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <iostream>
#include <stdint.h>
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
} CacheConfig;

typedef struct CacheLine_{
    int valid;
    int modified;
    uint64_t tag;
    int time_stamp;  // last visit
    char* line;
}CacheLine;

typedef struct CacheSet_{
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
        Cache(){
            /* initialize time stamp */
            _time_stamp = 0;
            _total_visit = 0;
            _total_hit = 0;
            _dirty_cnt = 0;
            _write_hit = 0;
            _replace_dirty_by_w = 0;
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
        // Main access process
        void HandleRequest(uint64_t addr, int bytes, int read,
                         char *content, int &hit, int &time);
        double CalculateMissRate() {
            cout << "total visit: " << _total_visit << endl;
            return 1 - (double)_total_hit / _total_visit;
        }
        int _dirty_cnt;
        int _write_hit;
        int _replace_dirty_by_w;
    
    private:
        CacheAddress SetAddrInfo(uint64_t addr);
        uint64_t getbit(uint64_t addr,int s,int e);
        int CacheHit(CacheAddress& addr_info);
        int FoundEmptyLine(CacheAddress& addr_info);
        void FindLRU(CacheAddress& addr_info);
        void ReplaceLine(CacheAddress& addr_info, char* new_line, int& time, int read_or_write);
        void ReadFromCache(CacheAddress& addr_info, int byte_num, char* content);
        void WriteToCache(CacheAddress& addr_info, int byte_num, char* content);
        CacheLine* GetCacheLine(CacheAddress& addr_info);
        CacheSet* GetCacheSet(CacheAddress& addr_info);
        void PrintSet(CacheAddress& addr_info);
    
        CacheSet* _cache;
        CacheConfig _config;
        Storage *_lower;
        DISALLOW_COPY_AND_ASSIGN(Cache);
        int _tag_bit;
        int _offset_bit;
        int _set_bit;
        uint64_t _time_stamp;  // used for counting
        int _total_visit;  // total visit in current cache
        int _total_hit;  // total hit cnt in current cache
};

#endif //CACHE_CACHE_H_ 

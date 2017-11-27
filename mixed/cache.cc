/*
 1）将地址分成t/s/b
 2）自己开一片空间给cache 初始函数？
     之后记得要关掉哦 析构函数中
 3）write allocate是个啥？？？
 4）流程：
     - read
         查找，是否命中
         命中则返回数据即可（向content中写入数据）
         不命中则向下一层申请那个块，大小为line_size，写到content里面。
         下一层返回回来一那个块，存到这一层中，可能涉及到替换。
             需要遍历这个set中所有的块，有空余位置则存入
             一直没有空余位置的话，LRU替换
                 替换时，需要看这个块有没有被修改过，如果有的话，需要写回到内存！
             状态设为valid，设置tag, modified
         返回数据
     - write
         查找，是否命中
         命中则根据策略更新,将content中的数据写到对应内存的位置
             writeback，先写到cache中，设置修改为为yes
         不命中则向下一层请求那个块，大小为line_size，写到content里面。
         下一层返回回来一那个块，存到这一层中，可能涉及到替换。
             需要遍历这个set中所有的块，有空余位置则存入
             一直没有空余位置的话，LRU替换
                 替换时，需要看这个块有没有被修改过，如果有的话，需要写回到内存！
                 状态设为valid，设置tag, modified
         返回数据
 5）write policy没有处理！如何直接写回memory？？？
 6）如何计算时间！
 7）load store等指令怎么处理！！！
 */
#include <iostream>
#include "cache.h"
#include "def.h"
using namespace std;

/* asked for 'byte_num' bytes starting from addr */
void Cache::HandleRequest(uint64_t addr, int byte_num, int read_or_write,
                          char *content, int &hit, int &time) {
    CacheAddress addr_info = SetAddrInfo(addr);
    time = _latency.bus_latency + _latency.hit_latency;
    
    /* Missed */
    if(!CacheHit(addr_info)){
        hit = NO;
        cout << "[Cache]: " << hex << addr << " Missed."<< endl;
        /* Request the line from lower cache */
        int lower_hit, lower_time;
        char* new_line = new char[_config.line_size];  // store the new block returned from lower cache
        uint64_t new_addr = addr & (~((1 << _offset_bit) - 1));
        _lower->HandleRequest(new_addr, _config.line_size, READ, new_line, lower_hit, lower_time);
        
        time += lower_time;
        _stats.access_time += _latency.bus_latency;
        
        if(!FoundEmptyLine(addr_info)){
            //cout << "[Cache]: No empty line!" << endl;
            FindLRU(addr_info);
        }
        ReplaceLine(addr_info, new_line, time);
        
        delete []new_line;
    }
    else{
        hit = YES;
        　_stats.access_time += time;
        cout << "[Cache]: " << hex << addr << " Hit."<< endl;
    }
    
    /* Handle Request */
    if(read_or_write == READ){
        ReadFromCache(addr_info, byte_num, content);
    }
    else if(read_or_write == WRITE){
        WriteToCache(addr_info, byte_num, content);
    }
    else{
        cerr << "[Cache]: Invalid operation. Expected 'READ' or 'WRITE'." << endl;
    }
    PrintSet(addr_info);
}

void Cache::SetConfig(CacheConfig cfg){
    _config.size = cfg.size;
    _config.associativity = cfg.associativity;
    _config.line_size = cfg.line_size;
    _config.set_num = (_config.size / _config.associativity) / _config.line_size;
    _config.write_policy = cfg.write_policy;
    _config.write_allocate_policy = cfg.write_allocate_policy;
    
    _offset_bit = 0;
    _set_bit = 0;
    int tmp = _config.set_num;
    while(tmp != 1){
        _set_bit += 1;
        tmp >>= 1;
    }
    tmp = _config.line_size;
    while(tmp != 1){
        _offset_bit += 1;
        tmp >>= 1;
    }
    _tag_bit = 64 - _offset_bit - _set_bit;
}

CacheAddress Cache::SetAddrInfo(uint64_t addr){
    CacheAddress addr_info;
    addr_info.offset = (int)getbit(addr, 0, _offset_bit - 1);
    addr_info.set = (int)getbit(addr, _offset_bit, _offset_bit + _set_bit - 1);
    addr_info.tag = getbit(addr, _offset_bit + _set_bit, 63);
    return addr_info;
}

uint64_t Cache::getbit(uint64_t addr,int s,int e){
    uint64_t mask = 0xffffffffffffffff;
    mask = (mask >> s) << s;
    mask = (mask << (63 - e)) >> (63 - e);
    uint64_t rst = addr & mask;
    rst = rst >> s;
    return rst;
}

/* whether the data is in cache or not.
 if hit, update addr_info.index
 */
int Cache::CacheHit(CacheAddress& addr_info){
    CacheSet* cache_set = GetCacheSet(addr_info);
    for(int i = 0; i < _config.associativity; ++i){
        CacheLine line = *((cache_set->cache_lines) + i);
        if(line.valid == NO){
            continue;
        }
        if(line.tag == addr_info.tag){
            addr_info.index = i;
            return YES;
        }
    }
    return NO;
}

/* whether there's an empty line in that set or not.
 if there is, update addr_info.index
 */
int Cache::FoundEmptyLine(CacheAddress& addr_info){
    CacheSet* cache_set = GetCacheSet(addr_info);
    for(int i = 0; i < _config.associativity; ++i){
        CacheLine line = *((cache_set->cache_lines) + i);
        if(line.valid == NO){
            addr_info.index = i;
            return YES;
        }
    }
    return NO;
    
}

/* find LRU in set and store its index in addr_info.index */
void Cache::FindLRU(CacheAddress& addr_info){
    int pos = 0;
    uint64_t min_stamp = 0xfffffffffffffff;
    CacheSet* cache_set = GetCacheSet(addr_info);
    for(int i = 0; i < _config.associativity; ++i){
        CacheLine line = *((cache_set->cache_lines) + i);
        if(line.time_stamp < min_stamp){
            min_stamp = line.time_stamp;
            pos = i;
        }
    }
    addr_info.index = pos;
}

/* replace with new_line */
void Cache::ReplaceLine(CacheAddress& addr_info, char* new_line, int& time){
    CacheLine* line = GetCacheLine(addr_info);
    
    if(line->valid == YES){
        //cout << "[Cache]: Replace " << hex << line->tag << " with " << addr_info.tag << endl;
    }
    
    /* old_line modified?(Only with WRITE_BACK policy) */
    if(line->valid == YES && (line->modified == YES && _config.write_policy == WRITE_BACK)){
        int lower_time, lower_hit;
        uint64_t old_addr = (addr_info.tag << (addr_info.set + addr_info.offset)) |
                            (addr_info.set << addr_info.offset);
        char* old_line = new char[_config.line_size];
        memcpy((void*)old_line, (const void*)(line->line), _config.line_size);
        _lower->HandleRequest(old_addr, _config.line_size, WRITE, old_line, lower_hit, lower_time);
        delete []old_line;
        time += lower_time;
    }
    
    /* replace with new line, set status */
    memcpy((void*)(line->line), new_line, _config.line_size);
    line->valid = YES;
    line->tag = addr_info.tag;
    line->modified = NO;
}

/* read 'byte_num' bytes from cache and stores in 'content' */
void Cache::ReadFromCache(CacheAddress& addr_info, int byte_num, char* content){
    //cout << "Reading from cache. size: "<< byte_num << endl;
    CacheLine* line = GetCacheLine(addr_info);
    memcpy((void*)content, (const void*)((line->line) + addr_info.offset), byte_num);
    line->time_stamp = _time_stamp;
    _time_stamp += 1;
}

/* store 'byte_num' bytes in content into cache */
void Cache::WriteToCache(CacheAddress& addr_info, int byte_num, char* content){
    CacheLine* line = GetCacheLine(addr_info);
    memcpy((void*)((line->line) + addr_info.offset), (const void*)content, byte_num);
    line->time_stamp = _time_stamp;
    _time_stamp += 1;
    line->modified = YES;
}

CacheLine* Cache::GetCacheLine(CacheAddress& addr_info){
    return &(_cache[addr_info.set].cache_lines[addr_info.index]);
}

CacheSet* Cache::GetCacheSet(CacheAddress& addr_info){
    return &(_cache[addr_info.set]);
}

void Cache::BuildCache(){
    /* initialize cache */
    _cache = new CacheSet[_config.set_num];
    for(int i = 0; i < _config.set_num; ++i){
        // _cache[i] is a CacheSet
        _cache[i].cache_lines = new CacheLine[_config.associativity];
        for(int j = 0; j < _config.associativity; ++j){
            // _cache[i].cache_lines[j] is a CacheLine
            _cache[i].cache_lines[j].line = new char[_config.line_size];
            _cache[i].cache_lines[j].valid = NO;
            _cache[i].cache_lines[j].modified = NO;
        }
    }
    //cout << "Cache set up!" << endl;
}

void Cache::ReleaseCache(){
    /* Release Cache */
    for(int i = 0; i < _config.set_num; ++i){
        for(int j = 0; j < _config.associativity; ++j){
            delete []_cache[i].cache_lines[j].line;
        }
        delete []_cache[i].cache_lines;
    }
    delete []_cache;
    cout << "Cache released!" << endl;
}

void Cache::PrintSet(CacheAddress& addr_info){
    CacheSet* cache_set = GetCacheSet(addr_info);
    string blank = "    ";
    string ans[2] = {"NO", "YES"};
    for(int i = 0; i < _config.associativity; ++i){
        CacheLine line = *((cache_set->cache_lines) + i);
        cout << "Line " << i << ": "<< endl;
        cout << blank << "valid: " << ans[line.valid] << endl;
        cout << blank << "modified: " << ans[line.modified] << endl;
        cout << blank << "tag: " << hex << line.tag << endl;
    }
    cout << "-----------------------" << endl;
}

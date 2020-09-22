
#include "pri/pager.h"
#include "cppkit/ck_file.h"
#include <cstring>
#include <unistd.h>

using namespace pri;
using namespace cppkit;
using namespace std;

pager::pager(const string& fileName) :
    _fileName(fileName),
    _f(ck_file::open(fileName, "r+")) {
}

void pager::create(const std::string& fileName) {
    auto f = ck_file::open(fileName, "w+");

    vector<uint8_t> block(pager::block_size());

    memset(&block[0], 0, pager::block_size());
    *(uint32_t*)&block[0] = 1;

    ck_fs::block_write_file(&block[0], pager::block_size(), f);
}

uint64_t pager::block_start_from(uint64_t ofs) const
{
    return (ofs / pager::block_size()) * pager::block_size();
}

ck_memory_map pager::map_page_from(uint64_t ofs) const {
    auto blockStart = block_start_from(ofs);
    size_t blockOfs = ofs - blockStart;

    return std::move(ck_memory_map(fileno(_f),
                                   blockStart,
                                   pager::block_size(),
                                   ck_memory_map::MM_PROT_READ | ck_memory_map::MM_PROT_WRITE,
                                   ck_memory_map::MM_TYPE_FILE | ck_memory_map::MM_SHARED,
                                   blockOfs));
}

uint64_t pager::append_page() const {
    uint32_t lastNBlocks;

    // The idea here is that we want to append space for 1 block to the end of the file and update our
    // nblocks field in the special block at the beginning of the file. We'd also like to return the file
    // offset of the new blocks.
    //
    // To keep this lock free I'm calling _update_nblocks() (which uses the gcc compiler intrinsic for
    // compare and swap).

    do {
        lastNBlocks = _read_nblocks();
        ftruncate(fileno(_f), ((lastNBlocks+1)*pager::block_size()));
    } while(!_update_nblocks(lastNBlocks, lastNBlocks+1));
    
    return pager::block_size() + (lastNBlocks * pager::block_size());
}

void pager::sync() const {
    fsync(fileno(_f));
}

uint32_t pager::_read_nblocks() const {
    auto mm = map_page_from(0);
    return *((uint32_t*)mm.map());
}

bool pager::_update_nblocks(uint32_t lastVal, uint32_t newVal) const {
    uint32_t out = __sync_val_compare_and_swap((uint32_t*)map_page_from(0).map(), lastVal, newVal);
    return out == lastVal;
}

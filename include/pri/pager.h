
#ifndef __pri_pager_h
#define __pri_pager_h

#include "cppkit/ck_memory_map.h"
#include "cppkit/ck_file.h"
#include <string>
#include <vector>

namespace pri
{

class pager final
{
public:
    pager(const std::string& fileName);
    pager(const pager&) = delete;
    pager(pager&&) = delete;
    ~pager() noexcept {}
    pager& operator=(const pager&) = delete;
    pager& operator=(pager&&) = delete;

    static size_t block_size() {return 4096;}

    static void create(const std::string& fileName);

    uint64_t block_start_from(uint64_t ofs) const;
    cppkit::ck_memory_map map_page_from(uint64_t ofs) const;
    uint64_t append_page() const;

    void sync() const;
private:
    uint32_t _read_nblocks() const;
    bool _update_nblocks(uint32_t lastVal, uint32_t newVal) const;

    std::string _fileName;
    cppkit::ck_file _f;
};

}

#endif

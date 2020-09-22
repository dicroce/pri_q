
#include "pri/pager.h"
#include "cppkit/ck_memory_map.h"
#include "cppkit/ck_file.h"
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <unistd.h>
#include <set>
#include <vector>
#include <functional>

namespace pri
{

typedef std::function<int(const uint8_t* a, const uint8_t* b)> CMP;

struct pri_q
{
public:
    pri_q(const std::string& fileName, CMP cmp) :
        _pager(fileName),
        _cmp(cmp) {
    }

    ~pri_q() noexcept {};

    static void create_pri_q(const std::string& file_name, size_t item_size) {
        if((pager::block_size() % item_size) != 0)
            CK_THROW(("item_size should fit evenly in 4k block."));

        pager::create(file_name);

        pager p(file_name);

        auto header_block = p.map_page_from(0);
        *_n_items(header_block) = 0;
        *_item_size(header_block) = item_size;

        p.sync();
    }

    void push(const uint8_t* p, bool commit = true) {
        _begin_transaction();

        auto header_block = _pager.map_page_from(0);
        auto n_items = *_n_items(header_block);
        auto item_size = *_item_size(header_block);

        if(((n_items * item_size) % pager::block_size()) == 0)
            _pager.append_page();
        
        auto nb_ofs = pager::block_size() + (n_items * item_size);
        auto nb = _pager.map_page_from(nb_ofs);

        memcpy(nb.map(), p, *_item_size(header_block));

        std::set<uint32_t> change_set;
        _journal_write(change_set, 0);
        _journal_write(change_set, (uint32_t)_pager.block_start_from(nb_ofs));

        _enforce_heap_property_up(change_set, n_items);

        *_n_items(header_block) = n_items + 1;

        if(commit)
            _commit_transaction();
    }

    void pop() {
        _begin_transaction();

        auto header_block = _pager.map_page_from(0);
        auto n_items = *_n_items(header_block);

		if(n_items == 0)
			throw std::out_of_range("index is out of range.");

        std::set<uint32_t> change_set;
        _journal_write(change_set, 0);

        _swap_by_index(header_block, change_set, 0, n_items - 1);

        *_n_items(header_block) = n_items - 1;

        _enforce_heap_property_down(change_set, 0);

        _commit_transaction();
    }

    template<typename CC>
    void top(CC cc) {
        auto header_block = _pager.map_page_from(0);
        auto n_items = *_n_items(header_block);

		if(n_items == 0)
			throw std::out_of_range("index is out of range.");

        auto item = _map_item(header_block, 0);
        cc(item.map());
    }

private:
    bool _journal_write(std::set<uint32_t>& visited_blocks, uint32_t pos) {
        if(visited_blocks.find(pos) == visited_blocks.end()) {
            visited_blocks.insert(pos);
            _write_block_to_journal(pos);
            return true;
        }
        return false;
    }
    static uint32_t* _n_items(const cppkit::ck_memory_map& header_block) {
        return (uint32_t*)(header_block.map() + 4);
    }

    static uint32_t* _item_size(const cppkit::ck_memory_map& header_block) {
        return (uint32_t*)(header_block.map() + 8);
    }

	inline uint32_t _parent(uint32_t i) {
        return (i - 1) / 2;
    }

	inline uint32_t _left_child(uint32_t i) {
        return (2 * i) + 1;
    }

	inline uint32_t _right_child(uint32_t i) {
        return (2 * i) + 2;
    }

    inline uint32_t _item_position(const cppkit::ck_memory_map& header_block, uint32_t i) {
        auto item_size = *_item_size(header_block);
        return pager::block_size() + (i * item_size);
    }

    cppkit::ck_memory_map _map_item(const cppkit::ck_memory_map& header_block, uint32_t i) {
        return std::move(_pager.map_page_from(_item_position(header_block, i)));
    }

    int _cmp_by_index(const cppkit::ck_memory_map& header_block, uint32_t a, uint32_t b) {
        auto item_a = _map_item(header_block, a);
        auto item_b = _map_item(header_block, b);
        return _cmp(item_a.map(), item_b.map());
    }

    void _swap_by_index(const cppkit::ck_memory_map& header_block, std::set<uint32_t>& change_set, uint32_t a, uint32_t b) {
        auto item_size = *_item_size(header_block);

        auto item_a = _map_item(header_block, a);
        auto item_a_p = item_a.map();

        auto item_b = _map_item(header_block, b);
        auto item_b_p = item_b.map();

        uint8_t temp_buffer[item_size];
        memcpy(&temp_buffer[0], item_a_p, item_size);
        memcpy(item_a_p, item_b_p, item_size);
        memcpy(item_b_p, &temp_buffer[0], item_size);

        _journal_write(change_set, (uint32_t)_pager.block_start_from(_item_position(header_block, a)));
        _journal_write(change_set, (uint32_t)_pager.block_start_from(_item_position(header_block, b)));
    }

    void _begin_transaction() {
        if(cppkit::ck_fs::file_exists("pri_journal"))
            _rollback_transaction();
        auto f = cppkit::ck_file::open("pri_journal", "w+");
    }

    void _write_block_to_journal(uint32_t blockStart) {
        auto f = cppkit::ck_file::open("pri_journal", "a+");
        auto blk = _pager.map_page_from(blockStart);

        cppkit::ck_fs::block_write_file(&blockStart, sizeof(blockStart), f);
        cppkit::ck_fs::block_write_file(blk.map(), pager::block_size(), f);

        // sync our journal
        fsync(fileno(f));

        f.close();
    }

    void _rollback_transaction() {
        auto file_size = cppkit::ck_fs::file_size("pri_journal");

        auto f = cppkit::ck_file::open("pri_journal", "r+");

        uint64_t bytesToRead = file_size;

        while(bytesToRead > 0)
        {
            uint32_t block_pos = 0;
            cppkit::ck_fs::block_read_file(&block_pos, sizeof(block_pos), f);
            bytesToRead -= sizeof(block_pos);

            std::vector<uint8_t> block(pager::block_size());
            cppkit::ck_fs::block_read_file(&block[0], pager::block_size(), f);
            bytesToRead -= pager::block_size();

            auto blk = _pager.map_page_from(block_pos);
            memcpy(blk.map(), &block[0], pager::block_size());
        }

        _pager.sync();

        f.close();

        unlink("pri_journal");
    }        

    void _commit_transaction() {
        _pager.sync();

        if(cppkit::ck_fs::file_exists("pri_journal"))
            unlink("pri_journal");
    }

    void _enforce_heap_property_up(std::set<uint32_t>& change_set, uint32_t i) {
        auto header_block = _pager.map_page_from(0);
        _enforce_heap_property_up(header_block, change_set, i);
    }

    void _enforce_heap_property_up(const cppkit::ck_memory_map& header_block, std::set<uint32_t>& change_set, uint32_t i) {
        if(i > 0) {
            if(_cmp_by_index(header_block, _parent(i), i) < 0) {
                _swap_by_index(header_block, change_set, _parent(i), i);
                _enforce_heap_property_up(header_block, change_set, _parent(i));
            }
        }
    }

    void _enforce_heap_property_down(std::set<uint32_t>& change_set, uint32_t i) {
        auto header_block = _pager.map_page_from(0);
        _enforce_heap_property_down(header_block, change_set, i);
    }

    void _enforce_heap_property_down(const cppkit::ck_memory_map& header_block, std::set<uint32_t>& change_set, uint32_t i) {
        auto n_items = *_n_items(header_block);

        auto left = _left_child(i);
        auto right = _right_child(i);
        auto largest = i;

        if((left < n_items) && (_cmp_by_index(header_block, left, i) == 1))
            largest = left;
        if((right < n_items) && (_cmp_by_index(header_block, right, largest) == 1))
            largest = right;
        
        if(largest != i) {
            _swap_by_index(header_block, change_set, i, largest);
            _enforce_heap_property_down(header_block, change_set, largest);
        }
    }

    pager _pager;
    CMP _cmp;
};

}

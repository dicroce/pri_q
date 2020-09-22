
#include "test_pri_q.h"
#include "pri/pri_q.h"
#include "pri/pager.h"
#include <vector>
#include <unistd.h>

using namespace std;
using namespace cppkit;
using namespace pri;

REGISTER_TEST_FIXTURE(test_pri_q);

int uint32_cmp(const uint8_t* a, const uint8_t* b)
{
    if(*(const uint32_t*)a < *(const uint32_t*)b)
        return -1;
    else if(*(const uint32_t*)a == *(const uint32_t*)b)
        return 0;
    else return 1;
}

void test_pri_q::setup()
{
    pri_q::create_pri_q("priority.dat", sizeof(uint32_t));
}

void test_pri_q::teardown()
{
    unlink("priority.dat");
}

void test_pri_q::test_basic()
{
    pri_q priority("priority.dat", uint32_cmp);

    vector<uint32_t> vals = {50, 40, 30, 20, 10, 8, 7};

    for(auto v: vals) {
        priority.push((uint8_t*)&v);
    }

    priority.top([](const uint8_t* p){
        RTF_ASSERT(*(uint32_t*)p == 50);
    });

    uint32_t v = 60;
    priority.push((const uint8_t*)&v);

    priority.top([](const uint8_t* p){
        RTF_ASSERT(*(uint32_t*)p == 60);
    });
}

void test_pri_q::test_pop()
{
    vector<uint32_t> vals = {20, 50, 75, 100, 5, 44, 42, 200, 83, 43, 19, 99, 76, 2, 1000};

    pri_q priority("priority.dat", uint32_cmp);

    for(auto v: vals) {
        priority.push((uint8_t*)&v);
    }

    vector<uint32_t> pops = {1000, 200, 100, 99, 83, 76, 75, 50, 44, 43, 42, 20, 19, 5, 2};

    for(size_t i = 0; i < pops.size(); ++i) {
        priority.top([i, &pops](const uint8_t* p) {
            RTF_ASSERT(*(uint32_t*)p == pops[i]);
        });
        priority.pop();
    }
}

void test_pri_q::test_rollback()
{
    pri_q priority("priority.dat", uint32_cmp);

    vector<uint32_t> vals = {50};

    for(auto v: vals) {
        priority.push((uint8_t*)&v, false);
    }

    vals = {10};

    for(auto v: vals) {
        priority.push((uint8_t*)&v);
    }

    priority.top([](const uint8_t* p){
        RTF_ASSERT(*(uint32_t*)p == 10);
    });
}
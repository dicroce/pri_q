
#include "framework.h"

class test_pri_q : public test_fixture
{
public:

    RTF_FIXTURE(test_pri_q);
      TEST(test_pri_q::test_basic);
      TEST(test_pri_q::test_pop);
      TEST(test_pri_q::test_rollback);
    RTF_FIXTURE_END();

    virtual ~test_pri_q() throw() {}

    virtual void setup();
    virtual void teardown();

    void test_basic();
    void test_pop();
    void test_rollback();
};

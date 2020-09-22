# pri_q
A super simple disk based priority queue in C++ (implemented as max heap). Supports atomicity via journal file.

First, create a max heap on disk in the file "priority.dat". The size of each element in the max heap must be specified here.

    pri_q::create_pri_q("priority.dat", sizeof(uint32_t));

Here is how you use such a thing. Note: an item comparison function must be provided. In this case, a lambda is used.

    vector<uint32_t> vals = {20, 50, 75, 100, 5, 44, 42, 200, 83, 43, 19, 99, 76, 2, 1000};

    pri_q priority("priority.dat", [](const uint8_t* a, const uint8_t* b){
      if(*(const uint32_t*)a < *(const uint32_t*)b)
          return -1;
      else if(*(const uint32_t*)a == *(const uint32_t*)b)
          return 0;
      else return 1;
    });

    for(auto v: vals) {
        priority.push((uint8_t*)&v);
    }

    // Verify the elements are returned in the right order.
    vector<uint32_t> pops = {1000, 200, 100, 99, 83, 76, 75, 50, 44, 43, 42, 20, 19, 5, 2};

    for(size_t i = 0; i < pops.size(); ++i) {
        priority.top([i, &pops](const uint8_t* p) {
            RTF_ASSERT(*(uint32_t*)p == pops[i]);
        });
        priority.pop();
    }

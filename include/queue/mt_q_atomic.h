#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <atomic>
namespace q::atomic
{

    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE>  class alignas(256) mt_q
    {
        public:
        mt_q():_capacity(std::pow(2,POWER_TWO_SIZE)), _mask(_capacity-1){
            #ifndef __APPLE__
            _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #else
            _data = static_cast<DATA_TYPE*>(aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #endif
        };

        bool push(const DATA_TYPE & ptr)
        {
            uint64_t tail{_tail.load(std::memory_order_relaxed)};
            uint64_t head{_head.load(std::memory_order_relaxed)};
            if (head - tail >= _capacity) {
                return false;
            }
            while( !_pre_head.compare_exchange_strong(head, head+1, std::memory_order_acquire) ){
               if (++head - tail >= _capacity) {
                    return false;
                }
            }
            auto idx = head & _mask;
            auto write_ptr = _data + idx;
            std::memcpy(write_ptr, &ptr, sizeof(DATA_TYPE));
            // The thing is that we can write ahead but cannot commit ahead.
            while (!_head.compare_exchange_strong(head, head+1, std::memory_order_release, std::memory_order_relaxed));
            return true;

        }

        bool pop_to(DATA_TYPE* out)
        {
            uint64_t head{0};
            head = _head.load(std::memory_order_relaxed);
            uint64_t tail{_tail.load(std::memory_order_relaxed)};
            if (tail >= head) {
                return false;
            }
            while ( !_pre_tail.compare_exchange_strong(tail,tail+1, std::memory_order_acquire));
            auto idx = tail & _mask;
            DATA_TYPE* read_ptr = _data + idx;
            memcpy(out, read_ptr, sizeof(DATA_TYPE)) ;
            while(!_tail.compare_exchange_strong(tail, tail+1, std::memory_order_release, std::memory_order_relaxed));
            return true;

        }


        private:
        uint64_t _capacity;
        uint64_t _mask;

        DATA_TYPE* _data;
        alignas(256) std::atomic<uint64_t> _head{0};
        alignas(256) std::atomic<uint64_t> _tail{0};
        alignas(256) std::atomic<uint64_t> _pre_head{0};
        alignas(256) std::atomic<uint64_t> _pre_tail{0};


    };
}
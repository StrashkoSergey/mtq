#include <cstdint>
#include <cstddef>
#include <cmath>
#include <atomic>
namespace q::atomic
{
    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE>  class alignas(256) spsc_q
    {
        public:
        spsc_q(): _head{0}, _tail{0}, _capacity(std::pow(2,POWER_TWO_SIZE)), _mask(_capacity-1) {
            #ifdef __APPLE__
            _data = static_cast<DATA_TYPE*>(aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #else
            _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #endif
        };

        bool push(DATA_TYPE &d)
        {
            uint64_t tail{_tail.load(std::memory_order_relaxed)};
            uint64_t head{_head.load(std::memory_order_acquire)};
            if (head  < tail + _capacity){
                auto idx = head & _mask;
                auto write_ptr = _data + idx;
                // *write_ptr = std::move(ptr);
                std::memcpy(write_ptr, &d, sizeof(DATA_TYPE));
                _head.store(head+1, std::memory_order_release);
                // _head.notify_one();
                return true;
            }
            return false;

        }

        bool pop_to(DATA_TYPE* out)
        {
            uint64_t head{_head.load(std::memory_order_relaxed)};
            uint64_t tail{_tail.load(std::memory_order_acquire)};
            if (tail >= head)
            {
                return false;
            }
            auto idx = tail & _mask;
            DATA_TYPE* read_ptr = _data + idx;
            memcpy(out, read_ptr, sizeof(DATA_TYPE)) ;
            _tail.store(tail+1, std::memory_order_release);
            return true;
        }


        // void theaded_read(R &r) {
        //     uint64_t head = 0;
        //     while (true) {
        //         _head.wait(head);
        //         do {
        //             head = _head.load(std::memory_order_relaxed);
        //             uint64_t tail{_tail.load(std::memory_order_acquire)};
        //             if (tail >= head)
        //             {
        //                 break;
        //             }
        //             auto idx = tail & _mask;
        //             DATA_TYPE* read_ptr = _data + idx;
        //             DATA_TYPE out;
        //             memcpy(&out, read_ptr, sizeof(DATA_TYPE)) ;
        //             bool res= r(out);
        //             _tail.store(tail+1, std::memory_order_release);
        //             if (!res) {
        //                 return;
        //             }
        //         } while (true);
        //     }
        // }
        alignas(256) std::atomic<uint64_t> _head;
        alignas(256) std::atomic<uint64_t> _tail;

        uint64_t _capacity;
        uint64_t _mask;
        DATA_TYPE* _data;

    };
}
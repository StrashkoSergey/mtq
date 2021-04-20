#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <atomic>
namespace q::atomic
{
    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE>  class alignas(256) queue
    {
        static_assert((POWER_TWO_SIZE >0), "POWER_TWO_SIZE must be in range [1,16]");
        static_assert((POWER_TWO_SIZE < 17),"POWER_TWO_SIZE must be in range [1,16]");
        static constexpr uint64_t pow2(){
            uint64_t powers[] = {1,
                                2,
                                4,
                                8,
                                16,
                                32,
                                64,
                                128,
                                256,
                                512,
                                1024,
                                2048,
                                4096,
                                8192,
                                16384,
                                32768,
                                65536};
            return powers[POWER_TWO_SIZE];
        }
        static constexpr uint64_t mask(){
            uint64_t masks[] = {0,
                                1,
                                3,
                                7,
                                15,
                                31,
                                63,
                                127,
                                255,
                                511,
                                1023,
                                2047,
                                4095,
                                8191,
                                16383,
                                32767,
                                65535};

            return masks[POWER_TWO_SIZE];
        }
    public:
        queue(){
            #ifndef __APPLE__
            _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* pow2()));
            #else
            _data = static_cast<DATA_TYPE*>(aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* pow2()));
            #endif
        };
        // push_move(DATA_TYPE&& o){
        //     size_t size{0};
        //     uint64_t tail{0};
        //     uint64_t head{0};
        //     head = _head.load(std::memory_order_acquire);
        //     tail = _tail.load(std::memory_order_relaxed);
        //     size = head - tail;
        //     if (size < pow2()){
        //         auto idx = head & mask();
        //         auto write_ptr = _data + idx;
        //         new(write_ptr) DATA_TYPE(o);
        //         _head.store(head+1, std::memory_order_release);
        //         return true;
        //     }
        //     return false;

        // }
        bool push(const DATA_TYPE & ptr)
        {
            size_t size{0};
            uint64_t tail{0};
            uint64_t head{0};
            head = _head.load(std::memory_order_acquire);
            tail = _tail.load(std::memory_order_relaxed);
            size = head - tail;
            if (size < pow2()){
                auto idx = head & mask();
                auto write_ptr = _data + idx;
                new(write_ptr) DATA_TYPE(ptr);
                _head.store(head+1, std::memory_order_release);
                return true;
            }
            return false;

        }
        // DATA_TYPE pop_move(DATA_TYPE* out)
        // {
        //     size_t size{0};
        //     uint64_t head{0};
        //     uint64_t tail{0};
        //     tail = _tail.load(std::memory_order_acquire);
        //     head = _head.load(std::memory_order_relaxed);

        //     size = head - tail;
        //     if (size > 0)
        //     {
        //         auto idx = tail & mask();
        //         DATA_TYPE* read_ptr = _data + idx;
        //         auto out = new DATA_TYPE(std::move(read_ptr));
        //         memcpy(out, read_ptr, sizeof(DATA_TYPE));
        //         _tail.store(tail+1, std::memory_order_release);
        //         return true;
        //     }
        //     return false;

        // }


        bool pop_to(DATA_TYPE* out)
        {
            size_t size{0};
            uint64_t head{0};
            uint64_t tail{0};
            tail = _tail.load(std::memory_order_acquire);
            head = _head.load(std::memory_order_relaxed);

            size = head - tail;
            if (size > 0)
            {
                auto idx = tail & mask();
                DATA_TYPE* read_ptr = _data + idx;
                memcpy(out, read_ptr, sizeof(DATA_TYPE));
                _tail.store(tail+1, std::memory_order_release);
                return true;
            }
            return false;

        }


        private:
        DATA_TYPE* _data;
        alignas(256) std::atomic<uint64_t> _head{0};
        alignas(256) std::atomic<uint64_t> _tail{0};
    };
}
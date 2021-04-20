#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <atomic>
namespace mtq
{

    ///Ring buffer queue
    ///
    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE>  class alignas(256) queue
    {
        static_assert((POWER_TWO_SIZE >0), "POWER_TWO_SIZE must be in range [1,16]");
        static_assert((POWER_TWO_SIZE < 17),"POWER_TWO_SIZE must be in range [1,16]");
        static constexpr uint64_t capacity(){
            uint64_t powers[] = {
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
                65536
            };
            return powers[POWER_TWO_SIZE-1];
        }
        static constexpr uint64_t mask(){
            uint64_t masks[] = {
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
                65535
            };

            return masks[POWER_TWO_SIZE-1];
        }
    public:
        queue(){
            #ifndef __APPLE__
            _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* capacity()));
            #else
            _data = static_cast<DATA_TYPE*>(aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* pow2()));
            #endif
        };

        void pring_struct(){
            std::cout << std::left << std::setw(40) << "================" << std::left << std::setw(40) << std::endl;

            std::cout << "this* ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)this << std::left <<std::setw(40) << std::endl;
            std::cout << "_data ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)_data << std::left <<std::setw(40) << std::endl;
            std::cout << "_head ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_head << std::left <<std::setw(40) << std::endl;
            std::cout << "_tail ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_tail << std::left <<std::setw(40) << std::endl;
            std::cout << "_capacity ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)capacity() << std::left <<std::setw(40) << std::endl;
            std::cout << std::left << std::setw(40) << "-----------------" << std::endl;

        }
        bool push(const DATA_TYPE & ptr)
        {
            uint64_t tail = _tail.load(std::memory_order_relaxed);
            uint64_t head = _head.load(std::memory_order_acquire);
            if (head - tail < capacity()){
                auto write_ptr = _data + (head & mask());
                new(write_ptr) DATA_TYPE(ptr);
                _head.store(head+1, std::memory_order_release);
                return true;
            }
            return false;

        }

        bool pop_to(DATA_TYPE* out)
        {
            uint64_t head = _head.load(std::memory_order_relaxed);
            uint64_t tail = _tail.load(std::memory_order_acquire);
            if (head > tail)
            {
                DATA_TYPE* read_ptr = _data + (tail & mask());
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
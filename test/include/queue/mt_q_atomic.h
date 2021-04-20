#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <atomic>
namespace q::atomic
{


    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE>  class alignas(256) mt_q
    {
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
        mt_q(){
            #ifndef __APPLE__
            _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* pow2()));
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
            std::cout << "_capacity ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)pow2() << std::left <<std::setw(40) << std::endl;
            std::cout << std::left << std::setw(40) << "-----------------" << std::endl;

        }

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
                std::memcpy(write_ptr, &ptr, sizeof(DATA_TYPE));
                _head.store(head+1, std::memory_order_release);
                return true;
            }
            return false;

        }

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
                memcpy(out, read_ptr, sizeof(DATA_TYPE)) ;
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
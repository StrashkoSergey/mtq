#pragma once
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <mutex>
#include <cmath>
#include <cstdlib>
#include <iomanip>

namespace q::split_lock
{
    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE> class  alignas(256) mt_q
    {
        public:
        mt_q():_capacity(std::pow(2,POWER_TWO_SIZE)), _mask(_capacity-1){
            #ifndef __APPLE__
            _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #else
            _data = static_cast<DATA_TYPE*>(aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #endif
        };

        void pring_struct(){
            std::cout << std::left << std::setw(40) << "================" << std::endl;
            std::cout << "this* ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)this << std::left <<std::setw(40) << std::endl;
            std::cout << "_data ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)_data << std::left <<std::setw(40) << std::endl;
            std::cout << "_head ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_head << std::left <<std::setw(40) << std::endl;
            std::cout << "_tail ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_tail << std::left <<std::setw(40) << std::endl;
            std::cout << "_capacity ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_capacity << std::left <<std::setw(40) << std::endl;
            std::cout << "_mask ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_mask << std::left <<std::setw(40) << std::endl;
            std::cout << "_head_lock ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_head_lock << std::left <<std::setw(40) << std::endl;
            std::cout << "_tail_lock ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_tail_lock << std::left <<std::setw(40) << std::endl;
            std::cout << std::left << std::setw(40) << "-----------------" << std::endl;

        }


        bool push(const DATA_TYPE & ptr)
        {
            size_t size{0};
            // _head_tail_lock.lock();
            uint64_t tail{0};
            _tail_lock.lock();
            tail = _tail;
            _tail_lock.unlock();

            std::lock_guard<std::mutex> lg(_head_lock);
            size = _head - tail;
            if (size < _capacity){
                auto idx = _head & _mask;
                auto write_ptr = _data + idx;
                std::memcpy(write_ptr, &ptr, sizeof(DATA_TYPE));
                _head++;
//                _head_tail_lock.unlock();
                return true;
            }
//            _head_tail_lock.unlock();
            //std::cout << "H: " << _head << " T: " << _tail << std::endl;
            return false;

        }
        bool pop_to(DATA_TYPE* out)
        {
            size_t size{0};

            //std::lock_guard<std::mutex> lg(_head_tail_lock);
            //_head_tail_lock.lock();
            uint64_t head{0};

            _head_lock.lock();
            head = _head;
            _head_lock.unlock();
            std::lock_guard<std::mutex> lg(_tail_lock);

            size = head - _tail;
            if (size > 0)
            {
                auto idx = _tail & _mask;
                DATA_TYPE* read_ptr = _data + idx;
                ++_tail;
                std::memcpy(out, read_ptr, sizeof(DATA_TYPE));
                //_head_tail_lock.unlock();
                return true;
            }
            //_head_tail_lock.unlock();
            //std::cout << "POP: H: " << _head << " T: " << _tail << std::endl;
            return false;

        }


        private:
        DATA_TYPE* _data;
        uint64_t _head{0};
        uint64_t _tail{0};

        uint64_t _capacity;
        uint64_t _mask;

        std::mutex _head_lock;
        std::mutex _tail_lock;

    };
}
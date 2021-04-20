#include <cstdint>
#include <cstddef>
#include <cmath>
#include <atomic>
namespace q::atomic
{
    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE>  class alignas(256) spmc_q
    {
        public:
        mt_q():_capacity(std::pow(2,POWER_TWO_SIZE)), _mask(_capacity-1){
            #ifdef __APPLE__
            _data = static_cast<DATA_TYPE*>(aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #else
            _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
            #endif
        };

        void pring_struct(){
            std::cout << std::left << std::setw(40) << "================" << std::left << std::setw(40) << std::endl;

            std::cout << "this* ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)this << std::left <<std::setw(40) << std::endl;
            std::cout << "_data ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)_data << std::left <<std::setw(40) << std::endl;
            std::cout << "_head ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_head << std::left <<std::setw(40) << std::endl;
            std::cout << "_tail ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_tail << std::left <<std::setw(40) << std::endl;
            std::cout << "_capacity ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_capacity << std::left <<std::setw(40) << std::endl;
            std::cout << std::left << std::setw(40) << "-----------------" << std::endl;

        }

        bool push(const DATA_TYPE & ptr)
        {
            size_t size{0};
            // _head_tail_lock.lock();
            uint64_t tail{0};
            uint64_t head{0};
            tail = _tail.load(std::memory_order_relaxed);
            head = _head.load(std::memory_order_acquire);

            size = head - tail;
            if (size < _capacity){
                auto idx = head & _mask;
                auto write_ptr = _data + idx;
                std::memcpy(write_ptr, &ptr, sizeof(DATA_TYPE));
                _head.store(head+1, std::memory_order_release);
//                _head.store(head+1, std::memory_order_seq_cst);
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
            // head = _head.load(std::memory_order_seq_cst);
            // tail = _tail.load(std::memory_order_seq_cst);
            do 
            {
                head = _head.load(std::memory_order_relaxed);
                size = head - tail;
                if (size == 0)
                {
                    return false;
                }
                else 
                {
                    auto idx = tail & _mask;
                    DATA_TYPE* read_ptr = _data + idx;
                    memcpy(out, read_ptr, sizeof(DATA_TYPE)) ;
                    // _tail.store(tail+1, std::memory_order_release);
                    // return true;
                } 

            } while (! _tail.compare_exchange_weak(tail, tail+1, std::memory_order_release, std::memory_order_relaxed));
            //_head_tail_lock.unlock();
            //std::cout << "POP: H: " << _head << " T: " << _tail << std::endl;
            return true;

        }


        private:
        uint64_t _capacity;
        uint64_t _mask;

        DATA_TYPE* _data;
        alignas(256) std::atomic<uint64_t> _head{0};
        alignas(256) std::atomic<uint64_t> _tail{0};


    };
}
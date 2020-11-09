#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <atomic>
#include <csignal>
namespace q::atomic
{
    static constexpr uint64_t HIGH_BIT_MASK()
    {
        return (uint64_t(-1)) & (uint64_t(-1) >> 1);
    }
    template<typename DATA_TYPE, unsigned char POWER_TWO_SIZE>  class alignas(256) spmc_q
    {
        // template<typename DT> struct container{
        //     container(): is_empty(true){}
        //     std::atomic_bool is_empty;
        //     alignas(alignof(DT)) char data[sizeof(DT)];
        // };
        public:
        bool push(const DATA_TYPE& o)
        {
            return _q.push(o);
        }
        bool pop_to(DATA_TYPE* out)
        {
            return _q.pop(*out);
        }
        void pring_struct()
        {
            _q.pring_struct();
        }
        private:
        template<typename DT, uint64_t CAPACITY, uint64_t MASK> class alignas(256) inner_q
        {
            public:
            inner_q():_head{0}, _tail{0}
            {}
            void pring_struct(){
                std::cout << std::left << std::setw(40) << "================" << std::left << std::setw(40) << std::endl;
                std::cout << "this* ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)this << std::left <<std::setw(40) << std::endl;
                std::cout << "_data ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)_data << std::left <<std::setw(40) << std::endl;
                std::cout << "_head ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_head << std::left <<std::setw(40) << std::endl;
                std::cout << "_tail ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_tail << std::left <<std::setw(40) << std::endl;
                std::cout << "_capacity:" << std::left <<std::setw(40) <<  std::dec << CAPACITY << std::left <<std::setw(40) << std::endl;
                std::cout << "_head :" << std::left <<std::setw(40) <<  std::dec << _head << std::left <<std::setw(40) << std::endl;
                std::cout << "_tail :" << std::left <<std::setw(40) <<  std::dec << _tail << std::left <<std::setw(40) << std::endl;
                std::cout << "Flags:"<< std::left <<std::setw(40);
                for(size_t i=0; i< CAPACITY; ++i)
                {
                    std::cout << std::dec <<  _data_state[i] << std::left <<std::setw(1) << " ";
                }
                std::cout <<std::endl;
                std::cout << std::left << std::setw(40) << "-----------------" << std::endl;

            }


            bool push(const DT& o)
            {
                uint64_t head = _head.load(std::memory_order_seq_cst);
                
                uint64_t idx = head & MASK;
                uint8_t f = 0;
                if (false == _data_state[idx].compare_exchange_strong(f, 1, std::memory_order_seq_cst, std::memory_order_relaxed))
                {
                    return false;
                }
                auto h2 = _head.fetch_add(1, std::memory_order_seq_cst);
                if (h2 != head){
                    return false;
                }
                //uint64_t head = _head.fetch_add(1, std::memory_order_seq_cst);
                DATA_TYPE* write_ptr = _data + idx;
                std::memcpy(write_ptr, &o, sizeof(DATA_TYPE));
                _data_state[idx].store(3, std::memory_order_seq_cst);
                return true;

            }

            bool pop(DT& out)
            {
                uint64_t tail = _tail.load(std::memory_order_seq_cst);
                uint64_t idx = tail & MASK;
                uint8_t f = 3;
                if (false == _data_state[idx].compare_exchange_strong(f,2, std::memory_order_seq_cst, std::memory_order_relaxed))
                {
                    return false;
                }
                _tail.fetch_add(1, std::memory_order_seq_cst);

                
                DATA_TYPE* read_ptr = _data + idx;
                std::memcpy(&out,read_ptr, sizeof(DATA_TYPE));
                _data_state[idx].store(0, std::memory_order_seq_cst);
                return true;

            }
            private:
            alignas(alignof(DT)) DT _data[CAPACITY];
            std::atomic_uint8_t _data_state[CAPACITY];
            //assume that head and tail consists of two values 0x7FFFFFFFFFFFFFFF - index and 0x8000 0000 0000 0000 - state
            alignas(256) std::atomic<uint64_t> _head;
            alignas(256) std::atomic<uint64_t> _tail;

        };
        
        private:
            inner_q<DATA_TYPE, 1 << POWER_TWO_SIZE, (1 << POWER_TWO_SIZE) -1> _q;




        // public:
        // spmc_q():_capacity(std::pow(2,POWER_TWO_SIZE)), _mask(_capacity-1),_head{0}, _tail{0}{
        //     #ifdef __APPLE__
        //     _data = static_cast<DATA_TYPE*>(aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
        //     _data_state = new std::atomic_uint8_t[_capacity]();
        //     #else
        //     _data = static_cast<DATA_TYPE*>(std::aligned_alloc(alignof(DATA_TYPE), sizeof(DATA_TYPE)* _capacity));
        //     #endif
        // };
        // ~spmc_q()
        // {
        //     free(_data);
        //     free(_data_state);
        //     _data=nullptr;
        //     _data_flags=nullptr;
        // }

        // void pring_struct(){
        //     std::cout << std::left << std::setw(40) << "================" << std::left << std::setw(40) << std::endl;

        //     std::cout << "this* ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)this << std::left <<std::setw(40) << std::endl;
        //     std::cout << "_data ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)_data << std::left <<std::setw(40) << std::endl;
        //     std::cout << "_head ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_head << std::left <<std::setw(40) << std::endl;
        //     std::cout << "_tail ptr:" << std::left <<std::setw(40) <<  std::hex << (void*)&_tail << std::left <<std::setw(40) << std::endl;
        //     std::cout << "_capacity:" << std::left <<std::setw(40) <<  std::dec << _capacity << std::left <<std::setw(40) << std::endl;
        //     std::cout << "_head :" << std::left <<std::setw(40) <<  std::dec << _head << std::left <<std::setw(40) << std::endl;
        //     std::cout << "_tail :" << std::left <<std::setw(40) <<  std::dec << _tail << std::left <<std::setw(40) << std::endl;
        //     for(size_t i=0; i<_capacity; ++i)
        //     {
        //         std::cout << _data_state[i] << " ";
        //     }
        //     std::cout <<std::endl;
        //     std::cout << std::left << std::setw(40) << "-----------------" << std::endl;

        // }

        // bool push(const DATA_TYPE & ptr)
        // {
        //     uint64_t head = _head.load(std::memory_order_relaxed);
        //     uint64_t idx = head & _mask;
        //     uint8_t f = 0;
        //     if (!(_data_state[idx].compare_exchange_strong(f,1, std::memory_order_acq_rel, std::memory_order_relaxed)))
        //     {
        //         return false;
        //     }
        //     _head.fetch_add(1, std::memory_order_relaxed);
            
        //     //uint64_t head = _head.fetch_add(1, std::memory_order_seq_cst);
        //     DATA_TYPE* write_ptr = _data + idx;
        //     std::memcpy(write_ptr, &ptr, sizeof(DATA_TYPE));
        //     _data_state[idx].store(3, std::memory_order_release);
        //     return true;
        // }

        // bool pop_to(DATA_TYPE* out)
        // {
        //     uint64_t tail = _tail.load(std::memory_order_relaxed);
        //     uint64_t idx = tail & _mask;
        //     uint8_t f = 3;
        //     if (!(_data_state[tail & _mask].compare_exchange_strong(f,2, std::memory_order_acq_rel, std::memory_order_relaxed)))
        //     {
        //         return false;
        //     }
        //      _tail.fetch_add(1, std::memory_order_relaxed);

            
        //     DATA_TYPE* read_ptr = _data + idx;
        //     std::memcpy(out,read_ptr, sizeof(DATA_TYPE));
        //     _data_state[idx].store(0, std::memory_order_release);
        //     return true;
        // }


        // private:
        // uint64_t _capacity;
        // uint64_t _mask;
    
        // DATA_TYPE* _data;
        // std::atomic_bool* _data_flags;
        // std::atomic_uint8_t* _data_state; // 0 - empty, 1-writing, 2-reading, 3-written
        // //uint8_t* _use_state;
        // alignas(256) std::atomic<uint64_t> _head;
        // alignas(256) std::atomic<uint64_t> _tail;
        

    };
}
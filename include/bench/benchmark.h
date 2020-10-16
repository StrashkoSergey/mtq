#pragma once
#include <vector>
#include <chrono>
namespace q::benchmark
{
template<typename DATA_TYPE, uint8_t Q_SIZE,  template<typename, uint8_t> typename Q_TYPE, class DATA_GENERATOR> struct sample{
    public:
    sample(uint32_t n_producers=1, uint32_t n_consumers=1):
        _q{}, _in_data{}, _producers{}, _consumers{},_n_w{n_producers}, _n_r{n_consumers}, _run_usec{0}
    {
        for (uint32_t i= 0; i < _n_r; ++i)
        {
            _out_data.emplace_back();
        }
    }
    sample(const sample& old):
        _q{}, _in_data{old._in_data}, _producers{}, _consumers{}, _n_w{old._n_w}, _n_r{old._n_r}, _run_usec{0}
    {
        for (uint32_t i= 0; i < _n_r; ++i)
        {
            _out_data.emplace_back();
        }
        for(auto it = _out_data.begin(); it!= _out_data.end(); ++it)
        {
            it->reserve(_in_data.size());
            it->resize(_in_data.size());
        }

    }
    sample& operator=(sample const& other)
    {
        _in_data.assign(other._in_data.begin(), other._in_data.end());
        return *this;
    }
    void generate(uint64_t n)
    {
        DATA_GENERATOR g;
        g(n, _in_data);
        for(auto it = _out_data.begin(); it!= _out_data.end(); ++it)
        {
            it->reserve(n);
            it->resize(n);
        }

    } 
    uint64_t get_duration() const
    {
        return _run_usec;
    }
    void do_run()
    {
        //first calculate run plan
        size_t stride = _in_data.size() / _n_w;
        std::vector<std::pair<typename std::vector<DATA_TYPE>::iterator,typename std::vector<DATA_TYPE>::iterator>> strides;
        auto b = _in_data.begin();
        while (std::distance(b+stride, _in_data.end()) > 0)
        {
            strides.push_back(std::pair<typename std::vector<DATA_TYPE>::iterator,typename std::vector<DATA_TYPE>::iterator>(b, b+stride));
            b+=stride;
        }
        strides.push_back(std::pair<typename std::vector<DATA_TYPE>::iterator,typename std::vector<DATA_TYPE>::iterator>(b, _in_data.end()));
        auto s = std::chrono::system_clock::now();
        for (uint32_t i = 0; i < _n_w; ++i){
            _producers.emplace_back(&sample::do_push, this,strides[i].first, strides[i].second );
        }
        for(uint32_t i=0; i< _n_r; ++i){
            _consumers.emplace_back(&sample::do_pop, this, std::ref(_out_data[i]));
        }
        for(auto it = _producers.begin(); it!= _producers.end(); ++it)
        {
            it->join();
        }
        for(auto it = _consumers.begin(); it!= _consumers.end(); ++it)
        {
            it->join();
        }

        auto e = std::chrono::system_clock::now();

        _run_usec = std::chrono::duration_cast<std::chrono::microseconds>(e-s).count();
    }
    template<class VERIFIER>bool verify_run()
    {
        VERIFIER v;
        return v(_in_data, _out_data);
    }
    private:
    void do_push(typename std::vector<DATA_TYPE>::iterator b, typename std::vector<DATA_TYPE>::iterator e) 
    {
        for (auto it = b; it !=e; ++it)
        {
            while (!_q.push(*it));
        }
    }
    void do_pop(std::vector<DATA_TYPE>& out)
    {
        size_t pop_count = _in_data.size();
        size_t popped{0};
        while (pop_count > popped)
        {
            while (_q.pop_to(&out[popped])){
                ++popped;
            }
        }
        
    }
    Q_TYPE<DATA_TYPE, Q_SIZE> _q;
    std::vector<DATA_TYPE> _in_data;
    std::vector<std::thread> _producers;
    std::vector<std::thread> _consumers;
    std::vector<std::vector<DATA_TYPE>> _out_data;
    uint32_t _n_w;
    uint32_t _n_r;
    uint64_t _run_usec;
};


}

// template<class SAMPLE >struct queue_benchmark
// {

//     queue_benchmark(std::string name) : 
//         _name{name},
//         _run_count{n_runs},
//         _min_duration{0},
//         _max_duration{0}, 
//         _average_duration{0}
//     {
//         _runs.reserve(_run_count);
//         _runs.resize(_run_count);
//     }
//     void add_sample(SAMPLE)
//     void run(){
//         std::cout << "Generating test data..." << std::endl;

//     }
//     void add_run(uint64_t duration)
//     {
//         runs.push_back(duration);
//         average_duration += duration;
//         run_count++;
//         if (run_count == 1){
//             min_duration = duration;
//             max_duration = duration;
//         }
//         else {
//             if (duration < min_duration)
//             {
//                 min_duration = duration;
//             }
//             if (duration > max_duration){
//                 max_duration = duration;
//             }
//         }
//     }
//     void finalize()
//     {
//         average_duration /= run_count;
//     }
//     void print()
//     {
//         std::cout << _name << " N: " << std::dec << run_count << " ave: " << average_duration << " min: " << min_duration << " max: " << max_duration << std::endl;
//     }
//     protected:
//     void generate_test_data();
//     private:
//     uint64_t _run_count;
//     uint64_t _min_duration;
//     uint64_t _max_duration;
//     uint64_t _average_duration;
//     std::vector<std::vector<DATA_TYPE>> _sample_data;
//     std::vector<std::vector<DATA_TYPE>> _pop_data;
//     std::vector<uint64_t> _runs;
//     std::string _name;
    

// };
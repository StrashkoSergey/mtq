#include <thread>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "mt_q_classic.h"
#include "mt_q_split_lock.h"
#include "mt_q_atomic.h"
#include "hash.h"
#include "bench/benchmark.h"
//#define Q_SIZE 10
//hash helpers

struct SampleData {
    SampleData(uint64_t i=0, double ix=0.0, double iy=0.0): id{i}, x{ix}, y{iy} {}
    uint64_t id;
    double x;
    double y;
    SampleData& operator+=(SampleData const& other){
        x += other.x;
        y += other.y;
        id = 0;
        return *this;    
    }
};


template<typename QDATA_TYPE, uint8_t Q_SIZE, template<typename, uint8_t> typename Q_TYPE > void f_push(Q_TYPE<QDATA_TYPE, Q_SIZE>* q, 
                                                                                                        std::vector<QDATA_TYPE> const& data,
                                                                                                        QDATA_TYPE& acc)
{
    for (auto it = data.cbegin(); it !=data.cend(); ++it)
    {
        bool r = true;
        do {
            QDATA_TYPE data = *it;
            r = q->push(data);
            if (r){
                //Remove hash calculation on push
                // h.update(&data);
                //instead calculate some sum
                acc+=data;
                break;
            } 
            // else 
            // {
            //     using namespace std::chrono_literals;
            //     std::this_thread::sleep_for(10us);
            //     //std::cout << "push queue is full:" << distance(data.cbegin(), it )<<   std::endl;

            // }
        } while (!r);
        
    }
}

template<typename QDATA_TYPE, uint8_t Q_SIZE, template<typename, uint8_t> typename Q_TYPE > void f_pop(Q_TYPE<QDATA_TYPE, Q_SIZE>* q,
                                                                                                       size_t const pop_count, 
                                                                                                       std::vector<QDATA_TYPE>& out, //must be preallocated
                                                                                                       QDATA_TYPE& acc)
{
    size_t popped{0};
    while (pop_count > popped)
    {
        while (q->pop_to(&out[popped])){
            acc+=out[popped]; 
            ++popped;
        }
    }
    
}
void generate_sampledata(std::vector<SampleData> &v, size_t n, hash<SampleData> &h)
{
    v.reserve(n);
    std::random_device rd;
    std::uniform_real_distribution<double> dist(-1000, 1000);
    for (uint64_t i=0; i < n; ++i)
    {
        SampleData s(i, dist(rd), dist(rd));
        v.push_back(s);
        h.update(&s);

    }
    h.commit();

}

void generate_uin64_t_data(std::vector<uint64_t> &v, size_t n, hash<uint64_t> &h)
{
    v.reserve(n);
    std::random_device rd;
    std::uniform_int_distribution<uint32_t> dist(0, 1024);
    v.resize(n);

    for (uint64_t i=0; i < n; ++i)
    {
        v[i]=dist(rd);
        h.update(&v[i]);

    }
    h.commit();
    

}

template<typename QDATA_TYPE, uint8_t QSIZE> uint64_t bench1to1_naive(std::vector<QDATA_TYPE> const &data, hash<QDATA_TYPE> &hpush, hash<QDATA_TYPE>& hpop)
{

    //generate n random numbers
    typedef q::classic::mt_q<QDATA_TYPE, QSIZE> queue;
    queue q;
    std::cout << "Naive:" << std::endl;
    q.pring_struct();

    //bench itself
    typedef decltype (&f_push<QDATA_TYPE, QSIZE, q::classic::mt_q>) push_classic;
    push_classic ref_push=&f_push<QDATA_TYPE, QSIZE, q::classic::mt_q>;

    typedef decltype (&f_pop<QDATA_TYPE, QSIZE, q::classic::mt_q>) pop_classic;
    pop_classic ref_pop=&f_pop<QDATA_TYPE, QSIZE, q::classic::mt_q>;

    auto s = std::chrono::system_clock::now();

    std::thread t1(ref_push, &q, std::ref(data),std::ref(hpush));
    std::thread t2(ref_pop, &q, data.size(), std::ref(hpop));
//    f_pop(&q, pop_sum);
    t1.join();
    t2.join();

    auto e = std::chrono::system_clock::now();
    hpush.commit();
    hpop.commit();
    auto duration = e-s;
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    //now need to compare
    std::cout << "done:(ms) " << std::dec <<ms.count() <<std::endl;
    return ms.count();

}

template<uint8_t QSIZE> uint64_t bench1to1_split_lock(std::vector<uint64_t> const &data, hash<uint64_t> &hpush, hash<uint64_t>& hpop)
{
    // std::cout <<"starting " << n << " points" << std::endl;
    // std::vector<uint64_t> data;
    // generate_uin64_t_data(data, n);

    q::split_lock::mt_q<uint64_t, QSIZE> q;
    std::cout << "Spil lock" << std::endl;
    q.pring_struct();

    typedef decltype (&f_push<uint64_t, QSIZE, q::split_lock::mt_q>) push_classic;
    push_classic ref_push=&f_push<uint64_t, QSIZE, q::split_lock::mt_q>;

    typedef decltype (&f_pop<uint64_t, QSIZE, q::split_lock::mt_q>) pop_classic;
    pop_classic ref_pop=&f_pop<uint64_t, QSIZE, q::split_lock::mt_q>;

    auto s = std::chrono::system_clock::now();
    std::thread t1(ref_push, &q, std::ref(data),std::ref(hpush));
    std::thread t2(ref_pop, &q, data.size(), std::ref(hpop));
//    f_pop(&q, pop_sum);
    t1.join();
    t2.join();
    auto e = std::chrono::system_clock::now();
    hpush.commit();
    hpop.commit();
    auto duration = e-s;
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    //now need to compare
    std::cout << "done:(ms) " << std::dec <<ms.count() <<std::endl;
    return ms.count();

}

template<typename QDATA_TYPE, uint8_t QSIZE> uint64_t bench1to1_atomic(std::vector<QDATA_TYPE> const &data, hash<QDATA_TYPE> &hpush, hash<QDATA_TYPE>& hpop)
{
    // std::cout <<"starting " << n << " points" << std::endl;
    // std::vector<uint64_t> data;
    // generate_uin64_t_data(data, n);

    q::atomic::mt_q<QDATA_TYPE, QSIZE> q;
    std::cout << "atomic" << std::endl;
    q.pring_struct();

    typedef decltype (&f_push<QDATA_TYPE, QSIZE, q::atomic::mt_q>) push_classic;
    push_classic ref_push=&f_push<QDATA_TYPE, QSIZE, q::atomic::mt_q>;

    typedef decltype (&f_pop<QDATA_TYPE, QSIZE, q::atomic::mt_q>) pop_classic;
    pop_classic ref_pop=&f_pop<QDATA_TYPE, QSIZE, q::atomic::mt_q>;

    auto s = std::chrono::system_clock::now();
    std::thread t1(ref_push, &q, std::ref(data),std::ref(hpush));
    std::thread t2(ref_pop, &q, data.size(), std::ref(hpop));
//    f_pop(&q, pop_sum);
    t1.join();
    t2.join();
    auto e = std::chrono::system_clock::now();
    hpush.commit();
    hpop.commit();
    auto duration = e-s;
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    //now need to compare
    std::cout << "done:(ms) " << std::dec <<ms.count() <<std::endl;
    return ms.count();

}


struct bench_result{
    uint64_t run_count;
    uint64_t min_duration;
    uint64_t max_duration;
    uint64_t average_duration;
    std::vector<uint64_t> runs;
    std::string _name;
    bench_result(std::string name) : run_count{0}, min_duration{0}, max_duration{0}, average_duration{0}, _name(name) {}
    
    void add_run(uint64_t duration)
    {
        runs.push_back(duration);
        average_duration += duration;
        run_count++;
        if (run_count == 1){
            min_duration = duration;
            max_duration = duration;
        }
        else {
            if (duration < min_duration)
            {
                min_duration = duration;
            }
            if (duration > max_duration){
                max_duration = duration;
            }
        }
    }
    void finalize()
    {
        average_duration /= run_count;
    }
    void print()
    {
        std::cout << _name << " N: " << std::dec << run_count << " ave: " << average_duration << " min: " << min_duration << " max: " << max_duration << std::endl;
    }

};

/*
    Holds all necessary info to run single benchmark sample
*/

struct sample_SD_generator
{
    void operator()(uint64_t n, std::vector<SampleData>& out)
    {
        out.reserve(n);
        out.resize(n);
        std::random_device rd;
        std::uniform_real_distribution<double> dist(-1000, 1000);
        for (uint64_t i=0; i < n; ++i)
        {
            new(&out[i]) SampleData(i, dist(rd), dist(rd));
        }
    }
};

struct sample_SD_verifier
{
    bool operator()(std::vector<SampleData> const& in, std::vector<std::vector<SampleData>> const& out)
    {
        hash<SampleData> h0;
        hash<SampleData> h1;

        for(auto it = in.cbegin(); it!= in.cend(); ++it)
        {
            h0.update(&(*it));
        }
        h0.commit();
        for(auto it = out.cbegin(); it!= out.cend(); ++it)
        {
            for (auto ii= it->cbegin(); ii!= it->cend(); ++ii)
            {
                h1.update(&(*ii));
            }
        }
        h1.commit();
        return h0==h1;
    }
};

template<typename D, uint8_t SIZE, template<typename, uint8_t> typename Q, class G, class V> void benchmark(std::string id, uint64_t data_size, uint8_t n_runs, uint32_t n_producers=1, uint32_t n_consumers=1)
{
    typedef q::benchmark::sample<D, SIZE, Q, G> sample_type;
    sample_type sample(n_producers, n_consumers);
    std::cout << id <<" Generating data: "  << std::dec << data_size <<"..."<< std::endl;
    sample.generate(data_size);
    std::vector<sample_type> samples;
    samples.resize(n_runs, sample);
    std::cout << "Data generated. Running: "  << std::dec << (int)n_runs <<"..."<< std::endl;

    for(auto it= samples.begin(); it != samples.end(); ++it)
    {
        it->do_run();

    }
    std::cout << "Run complete. Verifying... " << std::endl;

    for(auto it= samples.begin(); it != samples.end(); ++it)
    {
        if (!it->template verify_run<sample_SD_verifier>())
        {
            std::cout << "failed" << std::endl;
        } else {
            std::cout<< std::dec << "Pass: " << it->get_duration() << " usec"  << std::endl;
            
        }
    }
    uint64_t total =samples.begin()->get_duration();
    uint64_t min= samples.begin()->get_duration();
    uint64_t max= samples.begin()->get_duration();
    for(auto it= samples.begin()+1; it != samples.end(); ++it)
    {
        total += it->get_duration();
        if (min > it->get_duration())
        {
            min = it->get_duration();
        }
        if (max < it->get_duration())
        {
            max = it->get_duration();
        }
    }
    uint64_t ave = total / samples.size();


    std::cout <<std::dec << id << " N: " << samples.size() << " ave: " << ave << " min: " << min << " max: " << max << std::endl;


}

int main()
{
    uint64_t data_size = 2000000;
    uint8_t n_runs = 10;
    benchmark<SampleData, 10, q::classic::mt_q,sample_SD_generator, sample_SD_verifier>("Naive", data_size, n_runs);
    benchmark<SampleData, 10, q::split_lock::mt_q,sample_SD_generator, sample_SD_verifier>("Split lock", data_size, n_runs);
    benchmark<SampleData, 10, q::atomic::mt_q,sample_SD_generator, sample_SD_verifier>("Atomic", data_size, n_runs);

    return 0;
    // hash<uint64_t> h0("h0");
    // hash<SampleData> hsd0("hsd0");

    // //bench1to1_naive(0, 100000);
    // bench_result r_naive("Naive lock");
    // bench_result r_naive_sd("Naive lock sd");
    // size_t data_size = 1000000;
    // static constexpr uint8_t q_size=12;
    // std::vector<uint64_t> data;
    // std::vector<SampleData> sd_data;
    
    // generate_uin64_t_data(data, data_size, h0 );
    // generate_sampledata(sd_data, data_size, hsd0);
    // std::cout <<h0.as_string() << std::endl;
    // std::cout <<hsd0.as_string() << std::endl;


    // for (int i=0; i < 10; i++) {
    //     hash<uint64_t> hpush("push");
    //     hash<uint64_t> hpop("pop");
    //     hash<SampleData> hpush_sd("push sd");
    //     hash<SampleData> hpop_sd("pop sd");
    //     r_naive_sd.add_run(bench1to1_naive<SampleData, q_size>(sd_data, hpush_sd, hpop_sd));
    //     r_naive.add_run(bench1to1_naive<uint64_t, q_size>(data, hpush, hpop));
    //     if (h0 == hpush)
    //     {
    //         std::cout << "PUSH ok" << std::endl;
    //     } else {
    //         std::cout << "PUSH ERROR" << std::endl;
    //         throw h0.as_string(); 
    //     }
    //     if (h0 == hpop)
    //     {
    //         std::cout << "POP ok" << std::endl;
    //     } else {
    //         std::cout << "POP ERROR" << std::endl;
    //         throw h0.as_string(); 
    //     }
        
    //     if (hsd0 == hpush_sd)
    //     {
    //         std::cout << "PUSH ok" << std::endl;
    //     } else {
    //         std::cout << "PUSH ERROR" << std::endl;
    //         throw hsd0.as_string(); 
    //     }
    //     if (hsd0 == hpop_sd)
    //     {
    //         std::cout << "POP ok" << std::endl;
    //     } else {
    //         std::cout << "POP ERROR" << std::endl;
    //         throw hsd0.as_string(); 
    //     }

    // }

    // bench_result r_split_lock("Split lock");
    // for (int i=0; i < 10; i++) {
    //     hash<uint64_t> hpush;
    //     hash<uint64_t> hpop;
    //      r_split_lock.add_run(bench1to1_split_lock<q_size>(data, hpush, hpop));
    //     if ((h0 == hpush) && (h0 == hpop)){
    //         std::cout << "ok" << std::endl;
    //     } else {
    //         std::cout << "ERROR" << std::endl;
    //         throw h0.as_string(); 
    //     }
    // }
    // bench_result r_atomic("Atomic");
    // bench_result r_atomic_sd("Atomic sd");
    // for (int i=0; i < 10; i++) {
    //     hash<uint64_t> hpush;
    //     hash<uint64_t> hpop;
    //     hash<SampleData> hpush_sd;
    //     hash<SampleData> hpop_sd;
    //     r_atomic.add_run(bench1to1_atomic<uint64_t, q_size>(data, hpush, hpop));
    //     r_atomic_sd.add_run(bench1to1_atomic<SampleData, q_size>(sd_data, hpush_sd, hpop_sd));
    //     if (h0 == hpush && h0 == hpop){
    //         std::cout << "ok" << std::endl;
    //     } else {
    //         std::cout << "ERROR" << std::endl;
    //         throw h0.as_string(); 
    //     }
    //     if (hsd0 == hpush_sd && hsd0 == hpop_sd){
    //         std::cout << "ok" << std::endl;
    //     } else {
    //         std::cout << "ERROR" << std::endl;
    //         throw h0.as_string(); 
    //     }
    // }

    // r_naive.finalize();
    // r_naive.print();
    // r_naive_sd.finalize();
    // r_naive_sd.print();
    // r_split_lock.finalize();
    // r_split_lock.print();
    // r_atomic.finalize();
    // r_atomic.print();
    // r_atomic_sd.finalize();
    // r_atomic_sd.print();
    

}
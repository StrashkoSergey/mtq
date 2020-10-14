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
//#define Q_SIZE 10
//hash helpers

struct SampleData {
    SampleData(uint64_t i=0, double ix=0.0, double iy=0.0): id{i}, x{ix}, y{iy} {}
    uint64_t id;
    double x;
    double y;
};


template<typename QDATA_TYPE, uint8_t Q_SIZE, template<typename, uint8_t> typename Q_TYPE > void f_push(Q_TYPE<QDATA_TYPE, Q_SIZE>* q, 
                                                                                                        std::vector<QDATA_TYPE> const& data,
                                                                                                        hash<QDATA_TYPE>& h)
{
    for (auto it = data.cbegin(); it !=data.cend(); ++it)
    {
        bool r = true;
        do {
            QDATA_TYPE data = *it;
            r = q->push(data);
            if (r){
                h.update(&data);
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
                                                                                                       hash<QDATA_TYPE>& h)
{
    QDATA_TYPE v;
    size_t popped{0}; 
    while (pop_count > popped)
    {
        while (q->pop_to(&v)){
            h.update(&v); 
            popped++;
        }
    }
    
}
void generate_sampledata(std::vector<SampleData> &v, size_t n, hash<SampleData> &h)
{
    v.reserve(n);
    std::random_device rd;
    std::uniform_int_distribution<double> dist(-1000, 1000);
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
    //now no sha1

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

template<uint8_t QSIZE> uint64_t bench1to1_atomic(std::vector<uint64_t> const &data, hash<uint64_t> &hpush, hash<uint64_t>& hpop)
{
    // std::cout <<"starting " << n << " points" << std::endl;
    // std::vector<uint64_t> data;
    // generate_uin64_t_data(data, n);

    q::atomic::mt_q<uint64_t, QSIZE> q;
    std::cout << "atomic" << std::endl;
    q.pring_struct();

    typedef decltype (&f_push<uint64_t, QSIZE, q::atomic::mt_q>) push_classic;
    push_classic ref_push=&f_push<uint64_t, QSIZE, q::atomic::mt_q>;

    typedef decltype (&f_pop<uint64_t, QSIZE, q::atomic::mt_q>) pop_classic;
    pop_classic ref_pop=&f_pop<uint64_t, QSIZE, q::atomic::mt_q>;

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

int main()
{
    hash<uint64_t> h0("h0");
    hash<SampleData> hsd0("hsd0");

    //bench1to1_naive(0, 100000);
    bench_result r_naive("Naive lock");
    bench_result r_naive_sd("Naive lock sd");
    size_t data_size = 1000000;
    static constexpr uint8_t q_size=12;
    std::vector<uint64_t> data;
    std::vector<SampleData> sd_data;
    
    generate_uin64_t_data(data, data_size, h0 );
    generate_sampledata(sd_data, data_size, hsd0);
    std::cout <<h0.as_string() << std::endl;
    std::cout <<hsd0.as_string() << std::endl;


    for (int i=0; i < 10; i++) {
        hash<uint64_t> hpush("push");
        hash<uint64_t> hpop("pop");
        hash<SampleData> hpush_sd("push sd");
        hash<SampleData> hpop_sd("pop sd");
        r_naive_sd.add_run(bench1to1_naive<SampleData, q_size>(sd_data, hpush_sd, hpop_sd));
        //r_naive.add_run(bench1to1_naive<uint64_t, q_size>(data, hpush, hpop));
        if (h0 == hpush)
        {
            std::cout << "PUSH ok" << std::endl;
        } else {
            std::cout << "PUSH ERROR" << std::endl;
            throw h0.as_string(); 
        }
        if (h0 == hpop)
        {
            std::cout << "POP ok" << std::endl;
        } else {
            std::cout << "POP ERROR" << std::endl;
            throw h0.as_string(); 
        }
        
        if (hsd0 == hpush_sd)
        {
            std::cout << "PUSH ok" << std::endl;
        } else {
            std::cout << "PUSH ERROR" << std::endl;
            throw hsd0.as_string(); 
        }
        if (hsd0 == hpop_sd)
        {
            std::cout << "POP ok" << std::endl;
        } else {
            std::cout << "POP ERROR" << std::endl;
            throw hsd0.as_string(); 
        }

    }

    bench_result r_split_lock("Split lock");
    for (int i=0; i < 10; i++) {
        hash<uint64_t> hpush;
        hash<uint64_t> hpop;
         r_split_lock.add_run(bench1to1_split_lock<q_size>(data, hpush, hpop));
        if ((h0 == hpush) && (h0 == hpop)){
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "ERROR" << std::endl;
            throw h0.as_string(); 
        }
    }
    bench_result r_atomic("Atomic");
    for (int i=0; i < 10; i++) {
        hash<uint64_t> hpush;
        hash<uint64_t> hpop;
        r_atomic.add_run(bench1to1_atomic<q_size>(data, hpush, hpop));
        if (h0 == hpush && h0 == hpop){
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "ERROR" << std::endl;
            throw h0.as_string(); 
        }
    }

    r_naive.finalize();
    r_naive.print();

    r_split_lock.finalize();
    r_split_lock.print();
    r_atomic.finalize();
    r_atomic.print();
    

}
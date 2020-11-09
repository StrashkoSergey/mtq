#include <thread>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "queue/mt_q_classic.h"
#include "queue/mt_q_split_lock.h"
#include "queue/mt_q_atomic.h"
#include "hash/hash.h"
#include "bench/benchmark.h"
#include "queue/spmc_q.h"
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
    uint64_t data_size = 1000000;
    uint8_t n_runs = 10;
    benchmark<SampleData, 10, q::classic::mt_q,sample_SD_generator, sample_SD_verifier>("Naive", data_size, n_runs);
    benchmark<SampleData, 10, q::split_lock::mt_q,sample_SD_generator, sample_SD_verifier>("Split lock", data_size, n_runs);
    benchmark<SampleData, 20, q::atomic::mt_q,sample_SD_generator, sample_SD_verifier>("Atomic", data_size, n_runs, 1,1);
    benchmark<SampleData, 10, q::atomic::spmc_q, sample_SD_generator, sample_SD_verifier>("Spmc1", data_size,n_runs, 1, 1);
    // benchmark<SampleData, 10, q::atomic::spmc_q, sample_SD_generator, sample_SD_verifier>("Spmc2", data_size,n_runs, 2, 1);
    // benchmark<SampleData, 10, q::atomic::spmc_q, sample_SD_generator, sample_SD_verifier>("Spmc4", data_size,n_runs, 4, 1);
    // benchmark<SampleData, 10, q::atomic::spmc_q, sample_SD_generator, sample_SD_verifier>("Spmc8", data_size,n_runs, 8, 1);
    // benchmark<SampleData, 10, q::atomic::spmc_q, sample_SD_generator, sample_SD_verifier>("Spmc16", data_size,n_runs, 16, 1);
    return 0;
}
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <random>
#include <thread>
#include <mtq/include/queue.h>
#include <int_data.h>
#include <hash/hash.h>
// tests covers:
// 1. correctness
// 2. performance
// 2.a vs mutex queue
// -2.b vs folly Producer consumer queue
// -2.c atomic_flag queue

template<typename DT, uint8_t SIZE> void f_push(mtq::queue<DT, SIZE>& q,const std::vector<DT>& v, mtq::test::hash<DT>& h) {
    int c=0;
    do {
        if (q.push(v[c])){
            h.update((v[c]));
            c++;
        }
    } while (c != v.size());
    h.commit();
}
template<typename DT, uint8_t SIZE> void f_pop(mtq::queue<DT, SIZE>&q, std::vector<DT>& v, size_t n, mtq::test::hash<DT>& h) {
    size_t c=0;
    DT o;
    v.resize(n);
    do{
        if (q.pop_to(&o)){
            v[c]=o;
            h.update(o);
            c++;
        }
    } while(c != n);
    h.commit();

}
TEST_CASE( "correctnes" )
{
    std::vector<int32_t> v_in;
    std::vector<int32_t> v_out;
    mtq::test::hash<int32_t> h_in("in");
    mtq::test::hash<int32_t> h_out("out");

    mtq::test::int_data<int32_t>::generate(v_in, 1000'000);
    mtq::queue<int32_t, 1> q;
    std::thread t_pop([&] {f_pop<int32_t, 1>(q, v_out, 1000'000, h_out);} );
    std::thread t_push([&] {f_push<int32_t, 1>(q, v_in, h_in);} );

    t_pop.join();
    t_push.join();
    REQUIRE(v_in.size() == v_out.size());
    // for (size_t i = 0; i< v_in.size(); i++)
    // {
    //     REQUIRE(v_in[i]== v_out[i]);
    // }
    REQUIRE(h_in == h_out);
}

TEST_CASE("performance")
{

}
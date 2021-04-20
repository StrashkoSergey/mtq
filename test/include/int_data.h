#pragma once
#include <random>
#include <vector>
#include <openssl/evp.h>
#include <openssl/sha.h>
namespace mtq::test{

    template<typename IntType> struct int_data
    {
        static_assert(std::is_integral<IntType>::value, "IntType must be integral");
        static void generate(std::vector<IntType>& v, size_t n){
            std::random_device rd;
            std::uniform_int_distribution<IntType> d(std::numeric_limits<IntType>::min(),std::numeric_limits<IntType>::max());
            v.resize(n);
            for (size_t i = 0; i < n; i++)
            {
                v[i] = d(rd);

            }
        }
        static void generate(IntType* to, size_t n)
        {
            std::random_device rd;
            std::uniform_int_distribution<IntType> d(std::numeric_limits<IntType>::min(),std::numeric_limits<IntType>::max());
            for (size_t i = 0; i < n; i++)
            {
                to[i] = d(rd);
            }
        }
        private:
        std::vector<IntType> _data;

    };
}
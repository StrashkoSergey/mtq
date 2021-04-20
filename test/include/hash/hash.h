#pragma once
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <cstring>
#ifdef __APPLE__
#include <sstream>
#endif
namespace mtq::test{
    template<typename TIN> class hash{
    public:
        hash(std::string id=""):
            _id(id),
            _algo(EVP_sha3_512()),
            _digest(static_cast<uint8_t*>(OPENSSL_malloc(SHA512_DIGEST_LENGTH))),
            _digest_len(SHA512_DIGEST_LENGTH),
            _context(EVP_MD_CTX_new())
        {
            EVP_DigestInit_ex(_context, _algo, nullptr);
        }
        ~hash()
        {
            OPENSSL_free(_digest);
        }

        bool operator ==(hash<TIN>const& other) const {
            return memcmp(_digest, other._digest, _digest_len) == 0;
        }

        void update(const TIN v){
            EVP_DigestUpdate(_context, &v, sizeof(v));
        }
        void update(const TIN* value){
            EVP_DigestUpdate(_context, value, sizeof(TIN));
        }
        std::string as_string()
        {
            std::string output = bytes_to_hex_string(std::vector<uint8_t>(_digest, _digest + _digest_len));
            return output;

        }
        void commit()
        {
            EVP_DigestFinal_ex(_context, _digest, &_digest_len);
            EVP_MD_CTX_destroy(_context);

        }
    private:
        std::string bytes_to_hex_string(const std::vector<uint8_t>& bytes)
        {
            std::ostringstream stream;
            for (uint8_t b : bytes)
            {
                stream << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(b);
            }
            return stream.str();
        }
        std::string _id;
        const EVP_MD* _algo;
        uint8_t* _digest;
        uint32_t _digest_len;
        EVP_MD_CTX* _context;
    };
}

// std::string bytes_to_hex_string(const std::vector<uint8_t>& bytes)
// {
//     std::ostringstream stream;
//     for (uint8_t b : bytes)
//     {
//         stream << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(b);
//     }
//     return stream.str();
// }

// //perform the SHA3-512 hash
// std::string sha3_512(const std::string& input)
// {
//     uint32_t digest_length = SHA512_DIGEST_LENGTH;
//     const EVP_MD* algorithm = EVP_sha3_512();
//     uint8_t* digest = static_cast<uint8_t*>(OPENSSL_malloc(digest_length));
//     EVP_MD_CTX* context = EVP_MD_CTX_new();
//     EVP_DigestInit_ex(context, algorithm, nullptr);
//     EVP_DigestUpdate(context, input.c_str(), input.size());
//     EVP_DigestFinal_ex(context, digest, &digest_length);
//     EVP_MD_CTX_destroy(context);
//     std::string output = bytes_to_hex_string(std::vector<uint8_t>(digest, digest + digest_length));
//     OPENSSL_free(digest);
//     return output;
// }
/*
* RFC 6979 Deterministic Nonce Generator
* (C) 2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RFC6979_GENERATOR_H_
#define BOTAN_RFC6979_GENERATOR_H_

#include <botan/bigint.h>
#include <memory>
#include <string>

namespace Botan {

class HMAC_DRBG;

class BOTAN_TEST_API RFC6979_Nonce_Generator final {
   public:
      /**
      * Note: keeps persistent reference to order
      */
      RFC6979_Nonce_Generator(std::string_view hash, const BigInt& order, const BigInt& x);

      ~RFC6979_Nonce_Generator();

      const BigInt& nonce_for(const BigInt& m);

   private:
      const BigInt& m_order;
      BigInt m_k;
      size_t m_qlen, m_rlen;
      std::unique_ptr<HMAC_DRBG> m_hmac_drbg;
      secure_vector<uint8_t> m_rng_in, m_rng_out;
};

/**
* @param x the secret (EC)DSA key
* @param q the group order
* @param h the message hash already reduced mod q
* @param hash the hash function used to generate h
*/
BOTAN_TEST_API
BigInt generate_rfc6979_nonce(const BigInt& x, const BigInt& q, const BigInt& h, std::string_view hash);

}  // namespace Botan

#endif

/*
* ARIA
* Adapted for Botan by Jeffrey Walton, public domain
*
* Further changes
* (C) 2017,2020 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*
* This ARIA implementation is based on the 32-bit implementation by Aaram Yun from the
* National Security Research Institute, KOREA. Aaram Yun's implementation is based on
* the 8-bit implementation by Jin Hong. The source files are available in ARIA.zip from
* the Korea Internet & Security Agency website.
* <A HREF="https://tools.ietf.org/html/rfc5794">RFC 5794, A Description of the ARIA Encryption Algorithm</A>,
* <A HREF="http://seed.kisa.or.kr/iwt/ko/bbs/EgovReferenceList.do?bbsId=BBSMSTR_000000000002">Korea
* Internet & Security Agency homepage</A>
*/

#include <botan/internal/aria.h>

#include <botan/internal/loadstor.h>
#include <botan/internal/prefetch.h>
#include <botan/internal/rotate.h>

namespace Botan {

namespace {

namespace ARIA_F {

alignas(256) const uint8_t S1[256] = {
   0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9,
   0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F,
   0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15, 0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07,
   0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3,
   0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58,
   0xCF, 0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3,
   0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC, 0x5F,
   0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73, 0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
   0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC,
   0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A,
   0xAE, 0x08, 0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A, 0x70,
   0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11,
   0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF, 0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42,
   0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};

alignas(256) const uint8_t S2[256] = {
   0xE2, 0x4E, 0x54, 0xFC, 0x94, 0xC2, 0x4A, 0xCC, 0x62, 0x0D, 0x6A, 0x46, 0x3C, 0x4D, 0x8B, 0xD1, 0x5E, 0xFA, 0x64,
   0xCB, 0xB4, 0x97, 0xBE, 0x2B, 0xBC, 0x77, 0x2E, 0x03, 0xD3, 0x19, 0x59, 0xC1, 0x1D, 0x06, 0x41, 0x6B, 0x55, 0xF0,
   0x99, 0x69, 0xEA, 0x9C, 0x18, 0xAE, 0x63, 0xDF, 0xE7, 0xBB, 0x00, 0x73, 0x66, 0xFB, 0x96, 0x4C, 0x85, 0xE4, 0x3A,
   0x09, 0x45, 0xAA, 0x0F, 0xEE, 0x10, 0xEB, 0x2D, 0x7F, 0xF4, 0x29, 0xAC, 0xCF, 0xAD, 0x91, 0x8D, 0x78, 0xC8, 0x95,
   0xF9, 0x2F, 0xCE, 0xCD, 0x08, 0x7A, 0x88, 0x38, 0x5C, 0x83, 0x2A, 0x28, 0x47, 0xDB, 0xB8, 0xC7, 0x93, 0xA4, 0x12,
   0x53, 0xFF, 0x87, 0x0E, 0x31, 0x36, 0x21, 0x58, 0x48, 0x01, 0x8E, 0x37, 0x74, 0x32, 0xCA, 0xE9, 0xB1, 0xB7, 0xAB,
   0x0C, 0xD7, 0xC4, 0x56, 0x42, 0x26, 0x07, 0x98, 0x60, 0xD9, 0xB6, 0xB9, 0x11, 0x40, 0xEC, 0x20, 0x8C, 0xBD, 0xA0,
   0xC9, 0x84, 0x04, 0x49, 0x23, 0xF1, 0x4F, 0x50, 0x1F, 0x13, 0xDC, 0xD8, 0xC0, 0x9E, 0x57, 0xE3, 0xC3, 0x7B, 0x65,
   0x3B, 0x02, 0x8F, 0x3E, 0xE8, 0x25, 0x92, 0xE5, 0x15, 0xDD, 0xFD, 0x17, 0xA9, 0xBF, 0xD4, 0x9A, 0x7E, 0xC5, 0x39,
   0x67, 0xFE, 0x76, 0x9D, 0x43, 0xA7, 0xE1, 0xD0, 0xF5, 0x68, 0xF2, 0x1B, 0x34, 0x70, 0x05, 0xA3, 0x8A, 0xD5, 0x79,
   0x86, 0xA8, 0x30, 0xC6, 0x51, 0x4B, 0x1E, 0xA6, 0x27, 0xF6, 0x35, 0xD2, 0x6E, 0x24, 0x16, 0x82, 0x5F, 0xDA, 0xE6,
   0x75, 0xA2, 0xEF, 0x2C, 0xB2, 0x1C, 0x9F, 0x5D, 0x6F, 0x80, 0x0A, 0x72, 0x44, 0x9B, 0x6C, 0x90, 0x0B, 0x5B, 0x33,
   0x7D, 0x5A, 0x52, 0xF3, 0x61, 0xA1, 0xF7, 0xB0, 0xD6, 0x3F, 0x7C, 0x6D, 0xED, 0x14, 0xE0, 0xA5, 0x3D, 0x22, 0xB3,
   0xF8, 0x89, 0xDE, 0x71, 0x1A, 0xAF, 0xBA, 0xB5, 0x81};

alignas(256) const uint8_t X1[256] = {
   0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB, 0x7C, 0xE3, 0x39,
   0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB, 0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2,
   0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E, 0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76,
   0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25, 0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC,
   0x5D, 0x65, 0xB6, 0x92, 0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D,
   0x84, 0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06, 0xD0, 0x2C,
   0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B, 0x3A, 0x91, 0x11, 0x41, 0x4F,
   0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73, 0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85,
   0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E, 0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62,
   0x0E, 0xAA, 0x18, 0xBE, 0x1B, 0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD,
   0x5A, 0xF4, 0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F, 0x60,
   0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF, 0xA0, 0xE0, 0x3B, 0x4D,
   0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61, 0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6,
   0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D};

alignas(256) const uint8_t X2[256] = {
   0x30, 0x68, 0x99, 0x1B, 0x87, 0xB9, 0x21, 0x78, 0x50, 0x39, 0xDB, 0xE1, 0x72, 0x09, 0x62, 0x3C, 0x3E, 0x7E, 0x5E,
   0x8E, 0xF1, 0xA0, 0xCC, 0xA3, 0x2A, 0x1D, 0xFB, 0xB6, 0xD6, 0x20, 0xC4, 0x8D, 0x81, 0x65, 0xF5, 0x89, 0xCB, 0x9D,
   0x77, 0xC6, 0x57, 0x43, 0x56, 0x17, 0xD4, 0x40, 0x1A, 0x4D, 0xC0, 0x63, 0x6C, 0xE3, 0xB7, 0xC8, 0x64, 0x6A, 0x53,
   0xAA, 0x38, 0x98, 0x0C, 0xF4, 0x9B, 0xED, 0x7F, 0x22, 0x76, 0xAF, 0xDD, 0x3A, 0x0B, 0x58, 0x67, 0x88, 0x06, 0xC3,
   0x35, 0x0D, 0x01, 0x8B, 0x8C, 0xC2, 0xE6, 0x5F, 0x02, 0x24, 0x75, 0x93, 0x66, 0x1E, 0xE5, 0xE2, 0x54, 0xD8, 0x10,
   0xCE, 0x7A, 0xE8, 0x08, 0x2C, 0x12, 0x97, 0x32, 0xAB, 0xB4, 0x27, 0x0A, 0x23, 0xDF, 0xEF, 0xCA, 0xD9, 0xB8, 0xFA,
   0xDC, 0x31, 0x6B, 0xD1, 0xAD, 0x19, 0x49, 0xBD, 0x51, 0x96, 0xEE, 0xE4, 0xA8, 0x41, 0xDA, 0xFF, 0xCD, 0x55, 0x86,
   0x36, 0xBE, 0x61, 0x52, 0xF8, 0xBB, 0x0E, 0x82, 0x48, 0x69, 0x9A, 0xE0, 0x47, 0x9E, 0x5C, 0x04, 0x4B, 0x34, 0x15,
   0x79, 0x26, 0xA7, 0xDE, 0x29, 0xAE, 0x92, 0xD7, 0x84, 0xE9, 0xD2, 0xBA, 0x5D, 0xF3, 0xC5, 0xB0, 0xBF, 0xA4, 0x3B,
   0x71, 0x44, 0x46, 0x2B, 0xFC, 0xEB, 0x6F, 0xD5, 0xF6, 0x14, 0xFE, 0x7C, 0x70, 0x5A, 0x7D, 0xFD, 0x2F, 0x18, 0x83,
   0x16, 0xA5, 0x91, 0x1F, 0x05, 0x95, 0x74, 0xA9, 0xC1, 0x5B, 0x4A, 0x85, 0x6D, 0x13, 0x07, 0x4F, 0x4E, 0x45, 0xB2,
   0x0F, 0xC9, 0x1C, 0xA6, 0xBC, 0xEC, 0x73, 0x90, 0x7B, 0xCF, 0x59, 0x8F, 0xA1, 0xF9, 0x2D, 0xF2, 0xB1, 0x00, 0x94,
   0x37, 0x9F, 0xD0, 0x2E, 0x9C, 0x6E, 0x28, 0x3F, 0x80, 0xF0, 0x3D, 0xD3, 0x25, 0x8A, 0xB5, 0xE7, 0x42, 0xB3, 0xC7,
   0xEA, 0xF7, 0x4C, 0x11, 0x33, 0x03, 0xA2, 0xAC, 0x60};

inline uint32_t ARIA_F1(uint32_t X) {
   const uint32_t M1 = 0x00010101;
   const uint32_t M2 = 0x01000101;
   const uint32_t M3 = 0x01010001;
   const uint32_t M4 = 0x01010100;

   return (S1[get_byte<0>(X)] * M1) ^ (S2[get_byte<1>(X)] * M2) ^ (X1[get_byte<2>(X)] * M3) ^ (X2[get_byte<3>(X)] * M4);
}

inline uint32_t ARIA_F2(uint32_t X) {
   const uint32_t M1 = 0x00010101;
   const uint32_t M2 = 0x01000101;
   const uint32_t M3 = 0x01010001;
   const uint32_t M4 = 0x01010100;

   return (X1[get_byte<0>(X)] * M3) ^ (X2[get_byte<1>(X)] * M4) ^ (S1[get_byte<2>(X)] * M1) ^ (S2[get_byte<3>(X)] * M2);
}

inline void ARIA_FO(uint32_t& T0, uint32_t& T1, uint32_t& T2, uint32_t& T3) {
   T0 = ARIA_F1(T0);
   T1 = ARIA_F1(T1);
   T2 = ARIA_F1(T2);
   T3 = ARIA_F1(T3);

   T1 ^= T2;
   T2 ^= T3;
   T0 ^= T1;
   T3 ^= T1;
   T2 ^= T0;
   T1 ^= T2;

   T1 = ((T1 << 8) & 0xFF00FF00) | ((T1 >> 8) & 0x00FF00FF);
   T2 = rotr<16>(T2);
   T3 = reverse_bytes(T3);

   T1 ^= T2;
   T2 ^= T3;
   T0 ^= T1;
   T3 ^= T1;
   T2 ^= T0;
   T1 ^= T2;
}

inline void ARIA_FE(uint32_t& T0, uint32_t& T1, uint32_t& T2, uint32_t& T3) {
   T0 = ARIA_F2(T0);
   T1 = ARIA_F2(T1);
   T2 = ARIA_F2(T2);
   T3 = ARIA_F2(T3);

   T1 ^= T2;
   T2 ^= T3;
   T0 ^= T1;
   T3 ^= T1;
   T2 ^= T0;
   T1 ^= T2;

   T3 = ((T3 << 8) & 0xFF00FF00) | ((T3 >> 8) & 0x00FF00FF);
   T0 = rotr<16>(T0);
   T1 = reverse_bytes(T1);

   T1 ^= T2;
   T2 ^= T3;
   T0 ^= T1;
   T3 ^= T1;
   T2 ^= T0;
   T1 ^= T2;
}

/*
* ARIA encryption and decryption
*/
void transform(const uint8_t in[], uint8_t out[], size_t blocks, const secure_vector<uint32_t>& KS) {
   prefetch_arrays(S1, S2, X1, X2);

   const size_t ROUNDS = (KS.size() / 4) - 1;

   for(size_t i = 0; i != blocks; ++i) {
      uint32_t t0, t1, t2, t3;
      load_be(in + 16 * i, t0, t1, t2, t3);

      for(size_t r = 0; r < ROUNDS; r += 2) {
         t0 ^= KS[4 * r];
         t1 ^= KS[4 * r + 1];
         t2 ^= KS[4 * r + 2];
         t3 ^= KS[4 * r + 3];
         ARIA_FO(t0, t1, t2, t3);

         t0 ^= KS[4 * r + 4];
         t1 ^= KS[4 * r + 5];
         t2 ^= KS[4 * r + 6];
         t3 ^= KS[4 * r + 7];

         if(r != ROUNDS - 2)
            ARIA_FE(t0, t1, t2, t3);
      }

      out[16 * i + 0] = X1[get_byte<0>(t0)] ^ get_byte<0>(KS[4 * ROUNDS]);
      out[16 * i + 1] = X2[get_byte<1>(t0)] ^ get_byte<1>(KS[4 * ROUNDS]);
      out[16 * i + 2] = S1[get_byte<2>(t0)] ^ get_byte<2>(KS[4 * ROUNDS]);
      out[16 * i + 3] = S2[get_byte<3>(t0)] ^ get_byte<3>(KS[4 * ROUNDS]);
      out[16 * i + 4] = X1[get_byte<0>(t1)] ^ get_byte<0>(KS[4 * ROUNDS + 1]);
      out[16 * i + 5] = X2[get_byte<1>(t1)] ^ get_byte<1>(KS[4 * ROUNDS + 1]);
      out[16 * i + 6] = S1[get_byte<2>(t1)] ^ get_byte<2>(KS[4 * ROUNDS + 1]);
      out[16 * i + 7] = S2[get_byte<3>(t1)] ^ get_byte<3>(KS[4 * ROUNDS + 1]);
      out[16 * i + 8] = X1[get_byte<0>(t2)] ^ get_byte<0>(KS[4 * ROUNDS + 2]);
      out[16 * i + 9] = X2[get_byte<1>(t2)] ^ get_byte<1>(KS[4 * ROUNDS + 2]);
      out[16 * i + 10] = S1[get_byte<2>(t2)] ^ get_byte<2>(KS[4 * ROUNDS + 2]);
      out[16 * i + 11] = S2[get_byte<3>(t2)] ^ get_byte<3>(KS[4 * ROUNDS + 2]);
      out[16 * i + 12] = X1[get_byte<0>(t3)] ^ get_byte<0>(KS[4 * ROUNDS + 3]);
      out[16 * i + 13] = X2[get_byte<1>(t3)] ^ get_byte<1>(KS[4 * ROUNDS + 3]);
      out[16 * i + 14] = S1[get_byte<2>(t3)] ^ get_byte<2>(KS[4 * ROUNDS + 3]);
      out[16 * i + 15] = S2[get_byte<3>(t3)] ^ get_byte<3>(KS[4 * ROUNDS + 3]);
   }
}

// n-bit right shift of Y XORed to X
template <size_t N>
inline void ARIA_ROL128(const uint32_t X[4], const uint32_t Y[4], uint32_t KS[4]) {
   // MSVC is not generating a "rotate immediate". Constify to help it along.
   static const size_t Q = 4 - (N / 32);
   static const size_t R = N % 32;
   static_assert(R > 0 && R < 32, "Rotation in range for type");
   KS[0] = (X[0]) ^ ((Y[(Q) % 4]) >> R) ^ ((Y[(Q + 3) % 4]) << (32 - R));
   KS[1] = (X[1]) ^ ((Y[(Q + 1) % 4]) >> R) ^ ((Y[(Q) % 4]) << (32 - R));
   KS[2] = (X[2]) ^ ((Y[(Q + 2) % 4]) >> R) ^ ((Y[(Q + 1) % 4]) << (32 - R));
   KS[3] = (X[3]) ^ ((Y[(Q + 3) % 4]) >> R) ^ ((Y[(Q + 2) % 4]) << (32 - R));
}

void aria_ks_dk_transform(uint32_t& K0, uint32_t& K1, uint32_t& K2, uint32_t& K3) {
   K0 = rotr<8>(K0) ^ rotr<16>(K0) ^ rotr<24>(K0);
   K1 = rotr<8>(K1) ^ rotr<16>(K1) ^ rotr<24>(K1);
   K2 = rotr<8>(K2) ^ rotr<16>(K2) ^ rotr<24>(K2);
   K3 = rotr<8>(K3) ^ rotr<16>(K3) ^ rotr<24>(K3);

   K1 ^= K2;
   K2 ^= K3;
   K0 ^= K1;
   K3 ^= K1;
   K2 ^= K0;
   K1 ^= K2;

   K1 = ((K1 << 8) & 0xFF00FF00) | ((K1 >> 8) & 0x00FF00FF);
   K2 = rotr<16>(K2);
   K3 = reverse_bytes(K3);

   K1 ^= K2;
   K2 ^= K3;
   K0 ^= K1;
   K3 ^= K1;
   K2 ^= K0;
   K1 ^= K2;
}

/*
* ARIA Key Schedule
*/
void key_schedule(secure_vector<uint32_t>& ERK, secure_vector<uint32_t>& DRK, const uint8_t key[], size_t length) {
   const uint32_t KRK[3][4] = {{0x517cc1b7, 0x27220a94, 0xfe13abe8, 0xfa9a6ee0},
                               {0x6db14acc, 0x9e21c820, 0xff28b1d5, 0xef5de2b0},
                               {0xdb92371d, 0x2126e970, 0x03249775, 0x04e8c90e}};

   const size_t CK0 = (length / 8) - 2;
   const size_t CK1 = (CK0 + 1) % 3;
   const size_t CK2 = (CK1 + 1) % 3;

   uint32_t w0[4];
   uint32_t w1[4];
   uint32_t w2[4];
   uint32_t w3[4];

   w0[0] = load_be<uint32_t>(key, 0);
   w0[1] = load_be<uint32_t>(key, 1);
   w0[2] = load_be<uint32_t>(key, 2);
   w0[3] = load_be<uint32_t>(key, 3);

   w1[0] = w0[0] ^ KRK[CK0][0];
   w1[1] = w0[1] ^ KRK[CK0][1];
   w1[2] = w0[2] ^ KRK[CK0][2];
   w1[3] = w0[3] ^ KRK[CK0][3];

   ARIA_FO(w1[0], w1[1], w1[2], w1[3]);

   if(length == 24 || length == 32) {
      w1[0] ^= load_be<uint32_t>(key, 4);
      w1[1] ^= load_be<uint32_t>(key, 5);
   }
   if(length == 32) {
      w1[2] ^= load_be<uint32_t>(key, 6);
      w1[3] ^= load_be<uint32_t>(key, 7);
   }

   w2[0] = w1[0] ^ KRK[CK1][0];
   w2[1] = w1[1] ^ KRK[CK1][1];
   w2[2] = w1[2] ^ KRK[CK1][2];
   w2[3] = w1[3] ^ KRK[CK1][3];

   ARIA_FE(w2[0], w2[1], w2[2], w2[3]);

   w2[0] ^= w0[0];
   w2[1] ^= w0[1];
   w2[2] ^= w0[2];
   w2[3] ^= w0[3];

   w3[0] = w2[0] ^ KRK[CK2][0];
   w3[1] = w2[1] ^ KRK[CK2][1];
   w3[2] = w2[2] ^ KRK[CK2][2];
   w3[3] = w2[3] ^ KRK[CK2][3];

   ARIA_FO(w3[0], w3[1], w3[2], w3[3]);

   w3[0] ^= w1[0];
   w3[1] ^= w1[1];
   w3[2] ^= w1[2];
   w3[3] ^= w1[3];

   if(length == 16)
      ERK.resize(4 * 13);
   else if(length == 24)
      ERK.resize(4 * 15);
   else if(length == 32)
      ERK.resize(4 * 17);

   ARIA_ROL128<19>(w0, w1, &ERK[0]);
   ARIA_ROL128<19>(w1, w2, &ERK[4]);
   ARIA_ROL128<19>(w2, w3, &ERK[8]);
   ARIA_ROL128<19>(w3, w0, &ERK[12]);
   ARIA_ROL128<31>(w0, w1, &ERK[16]);
   ARIA_ROL128<31>(w1, w2, &ERK[20]);
   ARIA_ROL128<31>(w2, w3, &ERK[24]);
   ARIA_ROL128<31>(w3, w0, &ERK[28]);
   ARIA_ROL128<67>(w0, w1, &ERK[32]);
   ARIA_ROL128<67>(w1, w2, &ERK[36]);
   ARIA_ROL128<67>(w2, w3, &ERK[40]);
   ARIA_ROL128<67>(w3, w0, &ERK[44]);
   ARIA_ROL128<97>(w0, w1, &ERK[48]);

   if(length == 24 || length == 32) {
      ARIA_ROL128<97>(w1, w2, &ERK[52]);
      ARIA_ROL128<97>(w2, w3, &ERK[56]);

      if(length == 32) {
         ARIA_ROL128<97>(w3, w0, &ERK[60]);
         ARIA_ROL128<109>(w0, w1, &ERK[64]);
      }
   }

   // Now create the decryption key schedule
   DRK.resize(ERK.size());

   for(size_t i = 0; i != DRK.size(); i += 4) {
      DRK[i] = ERK[ERK.size() - 4 - i];
      DRK[i + 1] = ERK[ERK.size() - 3 - i];
      DRK[i + 2] = ERK[ERK.size() - 2 - i];
      DRK[i + 3] = ERK[ERK.size() - 1 - i];
   }

   for(size_t i = 4; i != DRK.size() - 4; i += 4) {
      aria_ks_dk_transform(DRK[i + 0], DRK[i + 1], DRK[i + 2], DRK[i + 3]);
   }
}

}  // namespace ARIA_F

}  // namespace

void ARIA_128::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   assert_key_material_set();
   ARIA_F::transform(in, out, blocks, m_ERK);
}

void ARIA_192::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   assert_key_material_set();
   ARIA_F::transform(in, out, blocks, m_ERK);
}

void ARIA_256::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   assert_key_material_set();
   ARIA_F::transform(in, out, blocks, m_ERK);
}

void ARIA_128::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   assert_key_material_set();
   ARIA_F::transform(in, out, blocks, m_DRK);
}

void ARIA_192::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   assert_key_material_set();
   ARIA_F::transform(in, out, blocks, m_DRK);
}

void ARIA_256::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   assert_key_material_set();
   ARIA_F::transform(in, out, blocks, m_DRK);
}

bool ARIA_128::has_keying_material() const { return !m_ERK.empty(); }

bool ARIA_192::has_keying_material() const { return !m_ERK.empty(); }

bool ARIA_256::has_keying_material() const { return !m_ERK.empty(); }

void ARIA_128::key_schedule(const uint8_t key[], size_t length) { ARIA_F::key_schedule(m_ERK, m_DRK, key, length); }

void ARIA_192::key_schedule(const uint8_t key[], size_t length) { ARIA_F::key_schedule(m_ERK, m_DRK, key, length); }

void ARIA_256::key_schedule(const uint8_t key[], size_t length) { ARIA_F::key_schedule(m_ERK, m_DRK, key, length); }

void ARIA_128::clear() {
   zap(m_ERK);
   zap(m_DRK);
}

void ARIA_192::clear() {
   zap(m_ERK);
   zap(m_DRK);
}

void ARIA_256::clear() {
   zap(m_ERK);
   zap(m_DRK);
}

}  // namespace Botan

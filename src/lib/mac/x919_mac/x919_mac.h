/*
* ANSI X9.19 MAC
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ANSI_X919_MAC_H_
#define BOTAN_ANSI_X919_MAC_H_

#include <botan/block_cipher.h>
#include <botan/mac.h>

namespace Botan {

/**
* DES/3DES-based MAC from ANSI X9.19
*/
class ANSI_X919_MAC final : public MessageAuthenticationCode {
   public:
      void clear() override;
      std::string name() const override;

      size_t output_length() const override { return 8; }

      std::unique_ptr<MessageAuthenticationCode> new_object() const override;

      Key_Length_Specification key_spec() const override { return Key_Length_Specification(8, 16, 8); }

      bool has_keying_material() const override;

      ANSI_X919_MAC();

      ANSI_X919_MAC(const ANSI_X919_MAC&) = delete;
      ANSI_X919_MAC& operator=(const ANSI_X919_MAC&) = delete;

   private:
      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;
      void key_schedule(const uint8_t[], size_t) override;

      std::unique_ptr<BlockCipher> m_des1, m_des2;
      secure_vector<uint8_t> m_state;
      size_t m_position;
};

}  // namespace Botan

#endif

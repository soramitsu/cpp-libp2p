/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_FUNCTIONS_HPP
#define LIBP2P_COMMON_FUNCTIONS_HPP

#include <memory>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::crypto {
  /**
   * Initializes EC_KEY structure with private and public key from private key
   * bytes
   * @param nid - curve group identifier for the key, ex. NID_secp256k1
   * @param private_key - private key bytes
   * @return shared pointer to EC_KEY with overridden destructor
   */
  outcome::result<std::shared_ptr<EC_KEY>> EcKeyFromPrivateKeyBytes(
      int nid, gsl::span<const uint8_t> private_key);

  /**
   * Initializes EVP_PKEY structure from private or public key bytes
   * @tparam ReaderFunc type of restore method. ex. EVP_PKEY_new_raw_private_key
   * or EVP_PKEY_new_raw_public_key
   * @param type - key type (almost the same as nid for ECC). ex.
   * EVP_PKEY_ED25519
   * @param key_bytes - raw representation of the source key
   * @param reader - pointer to a function of type ReaderFunc
   * @return shared pointer to EVP_PKEY with overridden destructor
   */
  template <typename ReaderFunc>
  outcome::result<std::shared_ptr<EVP_PKEY>> NewEvpPkeyFromBytes(
      int type, gsl::span<const uint8_t> key_bytes, ReaderFunc *reader);

}  // namespace libp2p::crypto

#endif  // LIBP2P_COMMON_FUNCTIONS_HPP

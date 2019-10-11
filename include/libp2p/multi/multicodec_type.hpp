/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTICODECTYPE_HPP
#define LIBP2P_MULTICODECTYPE_HPP

#include <string>

namespace libp2p::multi {

  class MulticodecType {
   public:
    /// TODO(Harrm) add more codes
    enum Code {
      IDENTITY = 0x00,
      SHA1 = 0x11,
      SHA2_256 = 0x12,
      SHA2_512 = 0x13,
      SHA3_512 = 0x14,
      SHA3_384 = 0x15,
      SHA3_256 = 0x16,
      SHA3_224 = 0x17,
      DAG_PB = 0x70
    };

    static std::string getName(Code code) {
      switch (code) {
        case Code::IDENTITY:
          return "raw";
        case Code::SHA1:
          return "sha1";
        case Code::SHA2_256:
          return "sha2-256";
        case Code::SHA2_512:
          return "sha2-512";
        case Code::SHA3_224:
          return "sha3-224";
        case Code::SHA3_256:
          return "sha3-256";
        case Code::SHA3_384:
          return "sha3-384";
        case Code::SHA3_512:
          return "sha3-512";
        case Code::DAG_PB:
          return "dag-pb";
      }
    }

  };

}

#endif  // LIBP2P_MULTICODECTYPE_HPP
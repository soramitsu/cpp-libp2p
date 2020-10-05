/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP

#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/x25519_provider.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>
#include <libp2p/security/noise/crypto/state.hpp>
#include <libp2p/security/noise/handshake_message_marshaller.hpp>

namespace libp2p::security::noise {

  std::shared_ptr<CipherSuite> defaultCipherSuite();

  class Handshake {
   public:
    explicit Handshake(crypto::KeyPair local_key,
                       std::shared_ptr<connection::RawConnection> connection,
                       bool is_initiator);

   private:
    const std::string kPayloadPrefix = "noise-libp2p-static-key:";

    outcome::result<std::vector<uint8_t>> generateHandshakePayload(
        const DHKey &keypair);

    outcome::result<void> sendHandshakeMessage(
        gsl::span<const uint8_t> precompiled_out,
        gsl::span<const uint8_t> payload);

    outcome::result<void> runHandshake();

    // constructor params
    const crypto::KeyPair local_key_;  // ???
    std::shared_ptr<connection::RawConnection> conn_;
    bool initiator_;  /// false for incoming connections

    // other params
    std::shared_ptr<crypto::CryptoProvider> crypto_provider_;  // todo init
    std::unique_ptr<security::noise::HandshakeMessageMarshaller>
        noise_marshaller_;  // todo init
    std::unique_ptr<HandshakeState> handshake_state_;
    uint8_t prefix_[2];
  };

}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP

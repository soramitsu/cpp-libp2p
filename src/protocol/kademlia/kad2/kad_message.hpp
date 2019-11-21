/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_MESSAGE_HPP
#define LIBP2P_KAD_MESSAGE_HPP

#include <libp2p/network/connection_manager.hpp>
#include "kad2_common.hpp"

namespace libp2p::kad2 {

  /// Message from wire protocol. Maybe either request or response
  struct Message {
    enum Type {
      kPutValue = 0,
      kGetValue = 1,
      kAddProvider = 2,
      kGetProviders = 3,
      kFindNode = 4,
      kPing = 5,

      kTableSize
    };

    struct Record {
      Key key;
      Value value;
      std::string time_received;
    };

    using Connectedness = network::ConnectionManager::Connectedness;

    struct Peer {
      peer::PeerInfo info;
      Connectedness conn_status = Connectedness::NOT_CONNECTED;
    };
    using Peers = std::vector<Peer>;

    Type type = kPing;
    Key key;
    std::optional<Record> record;
    std::optional<Peers> closer_peers;
    std::optional<Peers> provider_peers;

    void clear();

    // tries to deserialize message from byte array
    bool deserialize(const void* data, size_t sz);

    // serializes varint(message length) + message into buffer
    bool serialize(std::vector<uint8_t>& buffer) const;

    // adds this peer listening address to closer_peers
    void selfAnnounce(peer::PeerInfo self);
  };

  // self is a protocol extension if this is server (i.e. announce)
  Message createFindNodeRequest(const peer::PeerId& node, std::optional<peer::PeerInfo> self_announce);

}
#endif //LIBP2P_KAD_MESSAGE_HPP

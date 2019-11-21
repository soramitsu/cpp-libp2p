/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_MESSAGE_READ_WRITER_HPP
#define LIBP2P_KAD_MESSAGE_READ_WRITER_HPP

#include <libp2p/basic/message_read_writer.hpp>
#include <libp2p/protocol/kademlia/kad_message.hpp>

namespace libp2p::protocol::kademlia {

  /// An interface, which sends actual messages to other peers.
//  struct MessageReadWriter {
//    virtual ~MessageReadWriter() = default;
//
//    using PeerInfos = std::vector<peer::PeerInfo>;
//    using PeerInfosResult = outcome::result<PeerInfos>;
//    using PeerInfosResultFunc = std::function<void(PeerInfosResult)>;
//
//    virtual void findPeerSingle(const Key &p, const peer::PeerId &id,
//                                PeerInfosResultFunc f) = 0;
//  };

 class MessageReadWriter {
  public:
   using ReadResultFn = std::function<void(outcome::result<Message>)>;
   using WriteResultFn = basic::Writer::WriteCallbackFunc;

   MessageReadWriter(
     libp2p::common::Logger log, std::shared_ptr<basic::ReadWriter> conn,
     ReadResultFn rr, WriteResultFn wr
   );

   ~MessageReadWriter() = default;

   void read();

   void write(const Message& msg);

  private:
   void onRead(basic::MessageReadWriter::ReadCallback result);

   libp2p::common::Logger log_;
   std::shared_ptr<basic::MessageReadWriter> mrw_;
   std::vector<uint8_t> buffer_;
   ReadResultFn read_cb_;
   WriteResultFn write_cb_;
 };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_MESSAGE_READ_WRITER_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_MUXER_MULTISELECT_INSTANCE_HPP
#define LIBP2P_PROTOCOL_MUXER_MULTISELECT_INSTANCE_HPP

#include <libp2p/protocol_muxer/multiselect.hpp>
#include "parser.hpp"

namespace soralog {
  class Logger;
}

namespace libp2p::protocol_muxer::multiselect {

  class Multiselect;

  class MultiselectInstance
      : public std::enable_shared_from_this<MultiselectInstance> {
   public:
    explicit MultiselectInstance(Multiselect& owner);

    /// Implements ProtocolMuxer API
    void selectOneOf(gsl::span<const peer::Protocol> protocols,
                     std::shared_ptr<basic::ReadWriter> connection,
                     bool is_initiator, bool negotiate_multiselect,
                     Multiselect::ProtocolHandlerFunc cb);

   private:
    using Protocols = boost::container::small_vector<std::string, 4>;
    using Packet = std::shared_ptr<MsgBuf>;
    using Parser = detail::Parser;
    using MaybeResult = boost::optional<outcome::result<std::string>>;

    void sendOpening();

    /// Sends protocol proposal, returns false when all proposals exhausted
    bool sendProposal();

    /// Sends LS reply message
    void sendLS();

    /// Sends NA reply message
    void sendNA();

    void send(outcome::result<MsgBuf> msg);

    void send(Packet packet);

    void onDataWritten(outcome::result<size_t> res);

    void close(outcome::result<std::string> result);

    void receive();

    void onDataRead(outcome::result<size_t> res);

    MaybeResult processMessages();

    MaybeResult handleProposal(const std::string_view &protocol);

    MaybeResult handleNA();

    /// Owner of this object, needed for reuse
    Multiselect& owner_;

    /// Current round, helps enable Multiselect instance reuse (callbacks won't
    /// be passed to expired destination)
    size_t current_round_ = 0;

    /// List of protocols
    Protocols protocols_;

    /// Connection or stream
    std::shared_ptr<basic::ReadWriter> connection_;

    /// ProtocolMuxer callback
    Multiselect::ProtocolHandlerFunc callback_;

    /// True for client-side instance
    bool is_initiator_ = false;

    /// True if multistream protocol version is negotiated (strict mode)
    bool multistream_negotiated_ = false;

    /// Client specific: true if protocol proposal was sent
    bool wait_for_protocol_reply_ = false;

    /// True if the dialog is closed, no more callbacks
    bool closed_ = false;

    /// Client specific: index of last protocol proposal sent
    size_t current_protocol_ = 0;

    /// Server specific: has value if negotiation was successful and
    /// the instance waits for write callback completion.
    /// Inside is index of protocol chosen
    boost::optional<size_t> wait_for_reply_sent_;

    /// Incoming messages parser
    Parser parser_;

    /// Read buffer
    std::shared_ptr<std::array<uint8_t, kMaxMessageSize>> read_buffer_;

    /// Write queue. Still needed because the underlying ReadWriter may not
    /// support buffered writes
    std::deque<Packet> write_queue_;

    /// True if waiting for write callback
    bool is_writing_ = false;

    /// Cache: serialized LS response
    boost::optional<Packet> ls_response_;

    /// Cache: serialized NA response
    boost::optional<Packet> na_response_;
  };

}  // namespace libp2p::protocol_muxer::multiselect

#endif  // LIBP2P_PROTOCOL_MUXER_MULTISELECT_INSTANCE_HPP

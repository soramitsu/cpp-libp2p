/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL

#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/common/sublogger.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/content_routing_table.hpp>
#include <libp2p/protocol/kademlia/impl/message_observer.hpp>
#include <libp2p/protocol/kademlia/impl/peer_routing_table.hpp>
#include <libp2p/protocol/kademlia/impl/session_host.hpp>
#include <libp2p/protocol/kademlia/impl/storage.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <unordered_map>

namespace libp2p::protocol::kademlia {

  class FindPeerExecutor;
  class FindProvidersExecutor;

  class KademliaImpl final : public Kademlia,
                             public SessionHost,
                             public std::enable_shared_from_this<KademliaImpl> {
   public:
    KademliaImpl(
        const Config &config, std::shared_ptr<Host> host,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentRoutingTable> content_routing_table,
        std::shared_ptr<PeerRoutingTable> peer_routing_table,
        std::shared_ptr<Scheduler> scheduler, std::shared_ptr<event::Bus> bus,
        std::shared_ptr<crypto::random::RandomGenerator> random_generator);

    ~KademliaImpl() override = default;

    /// @see Kademlia::start
    void start() override;

    /// @see Routing::bootstrap
    outcome::result<void> bootstrap() override;

    /// @see ValueStore::putValue
    outcome::result<void> putValue(Key key, Value value) override;

    /// @see ValueStore::getValue
    outcome::result<void> getValue(const Key &key,
                                   FoundValueHandler handler) const override;

    /// @see ContentRouting::provide
    outcome::result<void> provide(const Key &key, bool need_notify) override;

    /// @see ContentRouting::findProviders
    outcome::result<void> findProviders(const Key &key, size_t limit,
                                        FoundProvidersHandler handler) override;

    /// @see PeerRouting::addPeer
    void addPeer(const PeerInfo &peer_info, bool permanent) override;

    /// @see PeerRouting::findPeer
    outcome::result<void> findPeer(const peer::PeerId &peer_id,
                                   FoundPeerInfoHandler handler) override;

    /// @see MessageObserver::onMessage
    void onMessage(const std::shared_ptr<Session> &stream,
                   Message &&msg) override;

    /// @see SessionHost::openSession
    std::shared_ptr<Session> openSession(
        std::shared_ptr<connection::Stream> stream) override;

    /// @see SessionHost::closeSession
    void closeSession(std::shared_ptr<connection::Stream> stream) override;

   private:
    void onPutValue(const std::shared_ptr<Session> &session, Message &&msg);
    void onGetValue(const std::shared_ptr<Session> &session, Message &&msg);
    void onAddProvider(const std::shared_ptr<Session> &session, Message &&msg);
    void onGetProviders(const std::shared_ptr<Session> &session, Message &&msg);
    void onFindNode(const std::shared_ptr<Session> &session, Message &&msg);
    void onPing(const std::shared_ptr<Session> &session, Message &&msg);

    void handleProtocol(protocol::BaseProtocol::StreamResult stream_res);

    std::shared_ptr<FindPeerExecutor> createFindPeerExecutor(
        boost::optional<PeerInfo> self_peer_info, PeerId sought_peer_id,
        std::unordered_set<PeerInfo> nearest_peer_infos,
        FoundPeerInfoHandler handler);

    std::shared_ptr<FindProvidersExecutor> createFindProvidersExecutor(
        boost::optional<PeerInfo> self_peer_info, ContentId sought_key,
        std::unordered_set<PeerInfo> nearest_peer_infos,
        FoundProvidersHandler handler);

    std::vector<PeerId> getNearestPeers(const NodeId &id);

    outcome::result<void> findRandomPeer();
    void randomWalk();

    // --- Primary (Injected) ---

    const Config &config_;
    std::shared_ptr<Host> host_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<ContentRoutingTable> content_routing_table_;
    std::shared_ptr<PeerRoutingTable> peer_routing_table_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<event::Bus> bus_;
    std::shared_ptr<crypto::random::RandomGenerator> random_generator_;

    // --- Secondary ---

    const Protocol &protocol_;
    const PeerId self_id_;

    // --- Auxiliary ---

    // Flag if started early
    bool started_ = false;

    // Subscribtion to new connections
    event::Handle new_connection_subscription_;

    // Subscribtion to new provided key
    event::Handle new_provided_key_subscription_;

    struct StreamPtrComparator {
      bool operator()(const std::shared_ptr<connection::Stream> &lhs,
                      const std::shared_ptr<connection::Stream> &rhs) const {
        return lhs.get() == rhs.get();
      }
    };

    struct StreamPtrHaher {
      size_t operator()(const std::shared_ptr<connection::Stream> &s) const {
        auto r = std::hash<decltype(s.get())>()(s.get());
        return r;
      }
    };

    std::unordered_map<const std::shared_ptr<connection::Stream>,
                       std::shared_ptr<Session>, StreamPtrHaher,
                       StreamPtrComparator>
        sessions_;

    // Random walk's auxiliary data
    struct {
      size_t iteration = 0;
      scheduler::Handle handle{};
    } random_walking_;

    SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_KADEMLIAIMPL

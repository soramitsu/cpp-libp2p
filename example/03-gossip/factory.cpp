/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include <boost/di.hpp>
#include <boost/di/extension/scopes/shared.hpp>

// implementations
#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/muxer/mplex.hpp>
#include <libp2p/muxer/yamux.hpp>
#include <libp2p/network/impl/connection_manager_impl.hpp>
#include <libp2p/network/impl/dialer_impl.hpp>
#include <libp2p/network/impl/listener_manager_impl.hpp>
#include <libp2p/network/impl/network_impl.hpp>
#include <libp2p/network/impl/router_impl.hpp>
#include <libp2p/network/impl/transport_manager_impl.hpp>
#include <libp2p/peer/address_repository/inmem_address_repository.hpp>
#include <libp2p/peer/impl/identity_manager_impl.hpp>
#include <libp2p/peer/impl/peer_repository_impl.hpp>
#include <libp2p/peer/key_repository/inmem_key_repository.hpp>
#include <libp2p/peer/protocol_repository/inmem_protocol_repository.hpp>
#include <libp2p/protocol/gossip/impl/gossip_core.hpp>
#include <libp2p/protocol_muxer/multiselect.hpp>
#include <libp2p/security/plaintext.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller_impl.hpp>
#include <libp2p/transport/impl/upgrader_impl.hpp>
#include <libp2p/transport/tcp.hpp>

#include "factory.hpp"

namespace libp2p::protocol::gossip::example {

  boost::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string &str) {
    auto server_ma_res = libp2p::multi::Multiaddress::create(str);
    if (!server_ma_res) {
      return boost::none;
    }
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      return boost::none;
    }

    auto server_peer_id_res = peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      return boost::none;
    }

    return peer::PeerInfo{server_peer_id_res.value(), {server_ma}};
  }

  namespace {
    template <typename... Ts>
    auto makeInjector(Ts &&... args) {
      using namespace boost;  // NOLINT

      auto csprng = std::make_shared<crypto::random::BoostRandomGenerator>();
      auto ed25519_provider =
          std::make_shared<crypto::ed25519::Ed25519ProviderImpl>();
      auto crypto_provider = std::make_shared<crypto::CryptoProviderImpl>(
          csprng, ed25519_provider);
      auto validator = std::make_shared<crypto::validator::KeyValidatorImpl>(
          crypto_provider);

      // assume no error here. otherwise... just blow up executable
      auto keypair =
          crypto_provider->generateKeys(crypto::Key::Type::Ed25519).value();

      // clang-format off
      return di::make_injector<boost::di::extension::shared_config>(
        di::bind<crypto::CryptoProvider>().to(crypto_provider)[boost::di::override],
        di::bind<crypto::KeyPair>().template to(std::move(keypair)),
        di::bind<crypto::random::CSPRNG>().template to(std::move(csprng)),
        di::bind<crypto::marshaller::KeyMarshaller>().template to<crypto::marshaller::KeyMarshallerImpl>(),
        di::bind<peer::IdentityManager>().template to<peer::IdentityManagerImpl>(),
        di::bind<crypto::validator::KeyValidator>().template to<crypto::validator::KeyValidatorImpl>(),
        di::bind<security::plaintext::ExchangeMessageMarshaller>().template to<security::plaintext::ExchangeMessageMarshallerImpl>(),

        // internal
        di::bind<network::Router>().template to<network::RouterImpl>(),
        di::bind<network::ConnectionManager>().template to<network::ConnectionManagerImpl>(),
        di::bind<network::ListenerManager>().template to<network::ListenerManagerImpl>(),
        di::bind<network::Dialer>().template to<network::DialerImpl>(),
        di::bind<network::Network>().template to<network::NetworkImpl>(),
        di::bind<network::TransportManager>().template to<network::TransportManagerImpl>(),
        di::bind<transport::Upgrader>().template to<transport::UpgraderImpl>(),
        di::bind<protocol_muxer::ProtocolMuxer>().template to<protocol_muxer::Multiselect>(),

        // default adaptors
        di::bind<security::SecurityAdaptor *[]>().template to<security::Plaintext>(),  // NOLINT
        di::bind<muxer::MuxerAdaptor *[]>().template to<muxer::Yamux, muxer::Mplex>(),  // NOLINT
        di::bind<transport::TransportAdaptor *[]>().template to<transport::TcpTransport>(),  // NOLINT

        di::bind<event::Bus>.template to<event::Bus>(),
        di::bind<Host>.template to<host::BasicHost>(),

        di::bind<muxer::MuxedConnectionConfig>.to(muxer::MuxedConnectionConfig()),

        // repositories
        di::bind<peer::PeerRepository>.template to<peer::PeerRepositoryImpl>(),
        di::bind<peer::AddressRepository>.template to<peer::InmemAddressRepository>(),
        di::bind<peer::KeyRepository>.template to<peer::InmemKeyRepository>(),
        di::bind<peer::ProtocolRepository>.template to<peer::InmemProtocolRepository>(),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...
      );
      // clang-format on
    }
  }  // namespace

  std::pair<std::shared_ptr<Host>, std::shared_ptr<Gossip>> createHostAndGossip(
      Config config, std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<boost::asio::io_context> io) {
    std::pair<std::shared_ptr<Host>, std::shared_ptr<Gossip>> p;

    auto injector = makeInjector(
        boost::di::bind<boost::asio::io_context>.to(io)[boost::di::override]);

    p.first = injector.create<std::shared_ptr<Host>>();
    assert(p.first);

    p.second = std::make_shared<GossipCore>(std::move(config),
                                            std::move(scheduler), p.first);

    return p;
  }

}  // namespace libp2p::protocol::gossip::example
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/di.hpp>
#include "boost/di/extension/scopes/shared.hpp"

// implementations
#include <libp2p/crypto/key_generator/key_generator_impl.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/muxer/mplex.hpp>
#include <libp2p/muxer/yamux.hpp>
#include <libp2p/network/impl/connection_manager_impl.hpp>
#include <libp2p/network/impl/dialer_impl.hpp>
#include <libp2p/network/impl/listener_manager_impl.hpp>
#include <libp2p/network/impl/network_impl.hpp>
#include <libp2p/network/impl/router_impl.hpp>
#include <libp2p/network/impl/transport_manager_impl.hpp>
#include <libp2p/peer/impl/identity_manager_impl.hpp>
#include <libp2p/protocol_muxer/multiselect.hpp>
#include <libp2p/security/plaintext.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller_impl.hpp>
#include <libp2p/transport/impl/upgrader_impl.hpp>
#include <libp2p/transport/tcp.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/peer/address_repository/inmem_address_repository.hpp>
#include <libp2p/peer/impl/peer_repository_impl.hpp>
#include <libp2p/peer/key_repository/inmem_key_repository.hpp>
#include <libp2p/peer/protocol_repository/inmem_protocol_repository.hpp>


#include <libp2p/protocol/kademlia/impl/routing_table_impl.hpp>

#include <iostream>

#include "factory.hpp"

namespace libp2p::kad_example {

  std::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string& str) {
    using R = std::optional<libp2p::peer::PeerInfo>;

    auto server_ma_res = libp2p::multi::Multiaddress::create(str);
    if (!server_ma_res) {
      std::cerr << "unable to create server multiaddress: "
                << server_ma_res.error().message() << std::endl;
      return R();
    }
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      std::cerr << "unable to get peer id" << std::endl;
      return R();
    }

    auto server_peer_id_res =
      libp2p::peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      std::cerr << "Unable to decode peer id from base 58: "
                << server_peer_id_res.error().message() << std::endl;
      return R();
    }

    return libp2p::peer::PeerInfo{server_peer_id_res.value(), {server_ma} };
  }

  namespace {
    template <typename... Ts>
    auto makeInjector(Ts &&... args) {
      using namespace boost;  // NOLINT

      auto csprng = std::make_shared<crypto::random::BoostRandomGenerator>();
      auto gen = std::make_shared<crypto::KeyGeneratorImpl>(*csprng);
      auto validator = std::make_shared<crypto::validator::KeyValidatorImpl>(gen);

      // assume no error here. otherwise... just blow up executable
      auto keypair = gen->generateKeys(crypto::Key::Type::Ed25519).value();

      // clang-format off
      return di::make_injector<boost::di::extension::shared_config>(
        di::bind<crypto::KeyPair>().template to(std::move(keypair)),
        di::bind<crypto::random::CSPRNG>().template to(std::move(csprng)),
        di::bind<crypto::KeyGenerator>().template to(std::move(gen)),
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
  }

  std::shared_ptr<libp2p::Host> createHost() {
    return makeInjector(boost::di::bind<boost::asio::io_context>.to(
      createIOContext())[boost::di::override])
      .create<std::shared_ptr<libp2p::Host>>();
  }

  void createHostAndRoutingTable(std::shared_ptr<libp2p::Host>& host,
    std::shared_ptr<libp2p::protocol::kademlia::RoutingTable>& table) {
    auto injector = makeInjector(boost::di::bind<boost::asio::io_context>.to(
      createIOContext())[boost::di::override]);

    host = injector.create<std::shared_ptr<libp2p::Host>>();
    table = injector.create<std::shared_ptr<libp2p::protocol::kademlia::RoutingTableImpl>>();
  }

  std::shared_ptr<boost::asio::io_context> createIOContext() {
    static std::shared_ptr<boost::asio::io_context> c = std::make_shared<boost::asio::io_context>();
    return c;
  }

} //namespace

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_OUTCOME_LOGGER_POLICY_HPP
#define LIBP2P_OUTCOME_LOGGER_POLICY_HPP

#include <boost/outcome/basic_result.hpp>
#include <boost/outcome/detail/trait_std_error_code.hpp>
#include <boost/outcome/detail/trait_std_exception.hpp>
#include <boost/outcome/policy/terminate.hpp>
#include <iostream>
#include <libp2p/log/logger.hpp>

namespace libp2p::outcome::policy {
  namespace bo2 = boost::outcome_v2;
  namespace trait = bo2::trait;
  namespace bpolicy = bo2::policy;
  using bo2::bad_result_access;
  using bpolicy::base;

  template <class T, class EC, class E>
  struct logger_error_code_throw_as_system_error;

  template <class T, class EC>
  struct logger_error_code_throw_as_system_error<T, EC, void> : base {
    template <class Impl>
    static constexpr void wide_value_check(Impl &&self) {
      if (!base::_has_value(std::forward<Impl>(self))) {
        if (base::_has_error(std::forward<Impl>(self))) {
          // ADL discovered
          bpolicy::outcome_throw_as_system_error_with_payload(
              base::_error(std::forward<Impl>(self)));
        }
        BOOST_OUTCOME_THROW_EXCEPTION(bad_result_access("no value"));  // NOLINT
      }
    }
    template <class Impl>
    static constexpr void wide_error_check(Impl &&self) {
      if (!base::_has_error(std::forward<Impl>(self))) {
        std::cerr << "NO ERROR: Value: "
                  << base::_has_value(std::forward<Impl>(self))
                  << ", Error: " << base::_has_error(std::forward<Impl>(self))
                  << ", Exception: "
                  << base::_has_exception(std::forward<Impl>(self))
                  << std::endl;

        BOOST_OUTCOME_THROW_EXCEPTION(bad_result_access("no error"));  // NOLINT
      }
    }
  };

  template <class T, class EC, class E>
  using logger_policy = std::conditional_t<  //
      std::is_void<EC>::value && std::is_void<E>::value,
      bpolicy::terminate,  //
      std::conditional_t<  //
          trait::is_error_code_available<EC>::value,
          logger_error_code_throw_as_system_error<T, EC, E>,  //
          std::conditional_t<                                 //
              trait::is_exception_ptr_available<EC>::value
                  || trait::is_exception_ptr_available<E>::value,
              bpolicy::exception_ptr_rethrow<T, EC, E>,  //
              bpolicy::fail_to_compile_observers         //
              >>>;
}  // namespace libp2p::outcome::policy

#endif  // LIBP2P_OUTCOME_LOGGER_POLICY_HPP

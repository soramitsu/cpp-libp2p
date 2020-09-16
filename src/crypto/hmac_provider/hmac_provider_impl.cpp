/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/hmac_provider/hmac_provider_impl.hpp>

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <gsl/gsl_util>
#include <gsl/span>
#include <libp2p/crypto/error.hpp>

namespace libp2p::crypto::hmac {
  using ByteArray = libp2p::common::ByteArray;

  [[deprecated]] size_t digestSize(common::HashType type) {
    switch (type) {
      case common::HashType::SHA1:
        return 20;
      case common::HashType::SHA256:
        return 32;
      case common::HashType::SHA512:
        return 64;
    }
    return 0;
  }

  [[deprecated]] const evp_md_st *makeHashTraits(common::HashType type) {
    switch (type) {
      case common::HashType::SHA1:
        return EVP_sha1();
      case common::HashType::SHA256:
        return EVP_sha256();
      case common::HashType::SHA512:
        return EVP_sha512();
    }
    return nullptr;
  }

  [[deprecated]] outcome::result<ByteArray> HmacProviderImpl::calculateDigest(
      HashType hash_type, const ByteArray &key,
      gsl::span<const uint8_t> message) const {
    const evp_md_st *evp_md = makeHashTraits(hash_type);
    auto digest_size = ::libp2p::crypto::hmac::digestSize(hash_type);
    if (evp_md == nullptr || digest_size == 0) {
      return HmacProviderError::UNSUPPORTED_HASH_METHOD;
    }

    std::vector<uint8_t> result(digest_size, 0);
    unsigned int len = 0;

    hmac_ctx_st *ctx = HMAC_CTX_new();
    if (nullptr == ctx) {
      return HmacProviderError::FAILED_CREATE_CONTEXT;
    }

    auto clean_ctx_at_exit = gsl::finally([ctx]() { HMAC_CTX_free(ctx); });

    if (1 != HMAC_Init_ex(ctx, key.data(), key.size(), evp_md, nullptr)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }

    if (1 != HMAC_Update(ctx, message.data(), message.size())) {
      return HmacProviderError::FAILED_UPDATE_DIGEST;
    }

    if (1 != HMAC_Final(ctx, result.data(), &len)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }

    if (digest_size != len) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }

    return result;
  }

  [[deprecated]] HmacProviderImpl::HmacProviderImpl() : initialized_{false} {}

  HmacProviderImpl::HmacProviderImpl(HmacProviderImpl::HashType hash_type,
                                     const ByteArray &key)
      : hash_type_{hash_type},
        key_{key},
        hash_st_{hash_type_ == HashType::SHA1
                     ? EVP_sha1()
                     : (hash_type_ == HashType::SHA256 ? EVP_sha256()
                                                       : EVP_sha512())},
        hmac_ctx_{nullptr},
        initialized_{false} {
    switch (hash_type_) {
      case common::HashType::SHA1:
      case common::HashType::SHA256:
      case common::HashType::SHA512:
        initialized_ = true;
        break;
      default:
        return;
    }
    if (nullptr == (hmac_ctx_ = HMAC_CTX_new())) {
      initialized_ = false;
      return;
    }
    initialized_ = 1
        == HMAC_Init_ex(hmac_ctx_, key_.data(), key_.size(), hash_st_, nullptr);
  }

  HmacProviderImpl::~HmacProviderImpl() {
    sinkCtx();
  }

  outcome::result<void> HmacProviderImpl::write(gsl::span<const uint8_t> data) {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    if (1 != HMAC_Update(hmac_ctx_, data.data(), data.size())) {
      return HmacProviderError::FAILED_UPDATE_DIGEST;
    }
    return outcome::success();
  }

  outcome::result<std::vector<uint8_t>> HmacProviderImpl::digest() {
    if (not initialized_) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    HMAC_CTX *ctx_copy{nullptr};
    if (1 != HMAC_CTX_copy(ctx_copy, hmac_ctx_)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    auto free_ctx_copy = gsl::finally([ctx_copy] { HMAC_CTX_free(ctx_copy); });
    std::vector<uint8_t> result;
    result.resize(digestSize());
    unsigned len{0};
    if (1 != HMAC_Final(ctx_copy, result.data(), &len)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }
    if (len != digestSize()) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }
    return result;
  }

  outcome::result<void> HmacProviderImpl::reset() {
    sinkCtx();
    hmac_ctx_ = HMAC_CTX_new();
    if (nullptr == hmac_ctx_
        or 1
            != HMAC_Init_ex(hmac_ctx_, key_.data(), key_.size(), hash_st_,
                            nullptr)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }
    initialized_ = true;
    return outcome::success();
  }

  size_t HmacProviderImpl::digestSize() const {
    if (initialized_) {
      return EVP_MD_size(hash_st_);
    }
    return 0;
  }

  size_t HmacProviderImpl::blockSize() const {
    if (initialized_) {
      return EVP_MD_block_size(hash_st_);
    }
    return 0;
  }

  void HmacProviderImpl::sinkCtx() {
    if (initialized_) {
      initialized_ = false;
      std::vector<uint8_t> data;
      data.resize(digestSize());
      unsigned len{0};
      HMAC_Final(hmac_ctx_, data.data(), &len);
      HMAC_CTX_free(hmac_ctx_);
      hmac_ctx_ = nullptr;
      memset(data.data(), 0, data.size());
    }
  }
}  // namespace libp2p::crypto::hmac

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <istream>

#include "Common.h"

namespace SimpleChessEngine::Nnue::Layers {
#if defined(SCE_NNUE_AVX2)
[[nodiscard]] inline std::int32_t LoadInt32(const std::uint8_t* input,
                                            const IndexType index) {
  std::int32_t value;
  std::memcpy(&value, input + index * sizeof(std::int32_t), sizeof(value));
  return value;
}

[[maybe_unused]] inline int M256HorizontalAdd(const __m256i sum,
                                              const int bias) {
  __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum),
                                 _mm256_extracti128_si256(sum, 1));
  sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_BADC));
  sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_CDAB));
  return _mm_cvtsi128_si32(sum128) + bias;
}

[[maybe_unused]] inline void M256AddDpbusdEpi32(__m256i& acc, const __m256i a,
                                                const __m256i b) {
#if defined(SCE_NNUE_AVXVNNI)
  acc = _mm256_dpbusd_avx_epi32(acc, a, b);
#else
  __m256i product = _mm256_maddubs_epi16(a, b);
  product = _mm256_madd_epi16(product, _mm256_set1_epi16(1));
  acc = _mm256_add_epi32(acc, product);
#endif
}

#else

template <IndexType kInputDimensions, IndexType kPaddedInputDimensions,
          IndexType kOutputDimensions>
void AffineTransformScalar(std::int32_t* output, const std::int8_t* weights,
                           const std::int32_t* biases,
                           const std::uint8_t* input) {
  std::memcpy(output, biases, sizeof(std::int32_t) * kOutputDimensions);
  for (IndexType i = 0; i < kInputDimensions; ++i) {
    if (input[i]) {
      const std::int8_t* w = &weights[i];
      const int in = input[i];
      for (IndexType j = 0; j < kOutputDimensions; ++j) {
        output[j] += w[j * kPaddedInputDimensions] * in;
      }
    }
  }
}
#endif

template <IndexType kInDims, IndexType kOutDims>
class AffineTransform {
 public:
  using InputType = std::uint8_t;
  using OutputType = std::int32_t;

  static constexpr IndexType kInputDimensions = kInDims;
  static constexpr IndexType kOutputDimensions = kOutDims;
  static constexpr IndexType kPaddedInputDimensions =
      CeilToMultiple<IndexType>(kInputDimensions, kMaxSimdWidth);
  static constexpr IndexType kPaddedOutputDimensions =
      CeilToMultiple<IndexType>(kOutputDimensions, kMaxSimdWidth);

  using OutputBuffer = OutputType[kPaddedOutputDimensions];

  static constexpr std::uint32_t GetHashValue(const std::uint32_t prev_hash) {
    std::uint32_t hash_value = 0xCC03DAE4u;
    hash_value += kOutputDimensions;
    hash_value ^= prev_hash >> 1;
    hash_value ^= prev_hash << 31;
    return hash_value;
  }

  static constexpr IndexType GetWeightIndex(const IndexType i) {
#if defined(SCE_NNUE_AVX2)
    return (i / 4) % (kPaddedInputDimensions / 4) * kOutputDimensions * 4 +
           i / kPaddedInputDimensions * 4 + i % 4;
#else
    return i;
#endif
  }

  bool ReadParameters(std::istream& stream) {
    ReadLittleEndian<BiasType>(stream, biases_, kOutputDimensions);
    for (IndexType i = 0; i < kOutputDimensions * kPaddedInputDimensions; ++i) {
      weights_[GetWeightIndex(i)] = ReadLittleEndian<WeightType>(stream);
    }
    return !stream.fail();
  }

  void Propagate(const InputType* input, OutputType* output) const {
#if defined(SCE_NNUE_AVX2)
    if constexpr (kOutputDimensions > 1) {
      using vec_t = __m256i;
      constexpr IndexType kOutputSimdWidth = sizeof(vec_t) / sizeof(OutputType);
      static_assert(kOutputDimensions % kOutputSimdWidth == 0);
      constexpr IndexType kNumChunks =
          CeilToMultiple<IndexType>(kInputDimensions, 8) / 4;
      constexpr IndexType kNumRegs = kOutputDimensions / kOutputSimdWidth;

      const auto bias_vec = reinterpret_cast<const vec_t*>(biases_);
      vec_t acc[kNumRegs];
      for (IndexType k = 0; k < kNumRegs; ++k) acc[k] = bias_vec[k];

      for (IndexType i = 0; i < kNumChunks; ++i) {
        const vec_t in0 = _mm256_set1_epi32(LoadInt32(input, i));
        const auto col0 = reinterpret_cast<const vec_t*>(
            &weights_[i * kOutputDimensions * 4]);
        for (IndexType k = 0; k < kNumRegs; ++k) {
          M256AddDpbusdEpi32(acc[k], in0, col0[k]);
        }
      }

      const auto out = reinterpret_cast<vec_t*>(output);
      for (IndexType k = 0; k < kNumRegs; ++k) out[k] = acc[k];
    } else {
      using vec_t = __m256i;
      const auto input_vector = reinterpret_cast<const vec_t*>(input);
      constexpr IndexType kInputSimdWidth = sizeof(vec_t) / sizeof(InputType);
      static_assert(kPaddedInputDimensions % kInputSimdWidth == 0);
      constexpr IndexType kNumChunks = kPaddedInputDimensions / kInputSimdWidth;
      vec_t sum = _mm256_setzero_si256();
      const auto row = reinterpret_cast<const vec_t*>(&weights_[0]);
      for (IndexType j = 0; j < kNumChunks; ++j) {
        M256AddDpbusdEpi32(sum, input_vector[j], row[j]);
      }
      output[0] = M256HorizontalAdd(sum, biases_[0]);
    }
#else
    AffineTransformScalar<kInputDimensions, kPaddedInputDimensions,
                          kOutputDimensions>(output, weights_, biases_, input);
#endif
  }

 private:
  using BiasType = OutputType;
  using WeightType = std::int8_t;

  alignas(kCacheLineSize) BiasType biases_[kOutputDimensions];
  alignas(kCacheLineSize)
      WeightType weights_[kOutputDimensions * kPaddedInputDimensions];
};

#if defined(SCE_NNUE_AVX2)
struct OffsetIndices {
  std::uint8_t offset_indices[256][8];

  constexpr OffsetIndices() : offset_indices() {
    for (int i = 0; i < 256; ++i) {
      std::uint64_t j = i;
      int k = 0;
      while (j) {
        offset_indices[i][k++] = static_cast<std::uint8_t>(std::countr_zero(j));
        j &= j - 1;
      }
      while (k < 8) offset_indices[i][k++] = 0;
    }
  }
};

alignas(kCacheLineSize) inline constexpr OffsetIndices kLookup{};

inline void AppendNonZero(const __m256i chunk, const __m128i base,
                          std::uint16_t* out, IndexType& count) {
  const unsigned lookup = static_cast<unsigned>(_mm256_movemask_ps(
      _mm256_castsi256_ps(_mm256_cmpgt_epi32(chunk, _mm256_setzero_si256()))));
  const __m128i offsets = _mm_cvtepu8_epi16(_mm_loadl_epi64(
      reinterpret_cast<const __m128i*>(&kLookup.offset_indices[lookup])));
  _mm_storeu_si128(reinterpret_cast<__m128i*>(out + count),
                   _mm_add_epi16(base, offsets));
  count += std::popcount(lookup);
}


#endif

template <IndexType kInDims, IndexType kOutDims>
class AffineTransformSparseInput {
 public:
  using InputType = std::uint8_t;
  using OutputType = std::int32_t;

  static constexpr IndexType kInputDimensions = kInDims;
  static constexpr IndexType kOutputDimensions = kOutDims;
  static_assert(kOutputDimensions % 16 == 0);

  static constexpr IndexType kPaddedInputDimensions =
      CeilToMultiple<IndexType>(kInputDimensions, kMaxSimdWidth);
  static constexpr IndexType kPaddedOutputDimensions =
      CeilToMultiple<IndexType>(kOutputDimensions, kMaxSimdWidth);

#if defined(SCE_NNUE_AVX2)
  static constexpr IndexType kChunkSize = 4;
#else
  static constexpr IndexType kChunkSize = 1;
#endif

  using OutputBuffer = OutputType[kPaddedOutputDimensions];

  static constexpr std::uint32_t GetHashValue(const std::uint32_t prev_hash) {
    std::uint32_t hash_value = 0xCC03DAE4u;
    hash_value += kOutputDimensions;
    hash_value ^= prev_hash >> 1;
    hash_value ^= prev_hash << 31;
    return hash_value;
  }

  static constexpr IndexType GetWeightIndex(const IndexType i) {
#if defined(SCE_NNUE_AVX2)
    return (i / kChunkSize) % (kPaddedInputDimensions / kChunkSize) *
               kOutputDimensions * kChunkSize +
           i / kPaddedInputDimensions * kChunkSize + i % kChunkSize;
#else
    return i;
#endif
  }

  bool ReadParameters(std::istream& stream) {
    ReadLittleEndian<BiasType>(stream, biases_, kOutputDimensions);
    for (IndexType i = 0; i < kOutputDimensions * kPaddedInputDimensions; ++i) {
      weights_[GetWeightIndex(i)] = ReadLittleEndian<WeightType>(stream);
    }
    return !stream.fail();
  }

  void Propagate(const InputType* input, OutputType* output,
                 [[maybe_unused]] const std::uint16_t* nnz,
                 [[maybe_unused]] const IndexType count) const {
#if defined(SCE_NNUE_AVX2)
    using vec_t = __m256i;
    constexpr IndexType kOutputSimdWidth = sizeof(vec_t) / sizeof(OutputType);
    constexpr IndexType kNumRegs = kOutputDimensions / kOutputSimdWidth;
    const auto bias_vec = reinterpret_cast<const vec_t*>(biases_);
    vec_t acc[kNumRegs];
    for (IndexType k = 0; k < kNumRegs; ++k) acc[k] = bias_vec[k];

    for (IndexType j = 0; j < count; ++j) {
      const auto i = nnz[j];
      const vec_t in = _mm256_set1_epi32(LoadInt32(input, i));
      const auto col = reinterpret_cast<const vec_t*>(
          &weights_[i * kOutputDimensions * kChunkSize]);
      for (IndexType k = 0; k < kNumRegs; ++k) {
        M256AddDpbusdEpi32(acc[k], in, col[k]);
      }
    }

    const auto out = reinterpret_cast<vec_t*>(output);
    for (IndexType k = 0; k < kNumRegs; ++k) out[k] = acc[k];
#else
    AffineTransformScalar<kInputDimensions, kPaddedInputDimensions,
                          kOutputDimensions>(output, weights_, biases_, input);
#endif
  }

 private:
  using BiasType = OutputType;
  using WeightType = std::int8_t;

  alignas(kCacheLineSize) BiasType biases_[kOutputDimensions];
  alignas(kCacheLineSize)
      WeightType weights_[kOutputDimensions * kPaddedInputDimensions];
};

template <IndexType kInDims>
class ClippedReLU {
 public:
  using InputType = std::int32_t;
  using OutputType = std::uint8_t;

  static constexpr IndexType kInputDimensions = kInDims;
  static constexpr IndexType kOutputDimensions = kInputDimensions;
  static constexpr IndexType kPaddedOutputDimensions =
      CeilToMultiple<IndexType>(kOutputDimensions, 32);

  using OutputBuffer = OutputType[kPaddedOutputDimensions];

  static constexpr std::uint32_t GetHashValue(const std::uint32_t prev_hash) {
    return 0x538D24C7u + prev_hash;
  }

  bool ReadParameters(std::istream&) { return true; }

  void Propagate(const InputType* input, OutputType* output) const {
#if defined(SCE_NNUE_AVX2)
    if constexpr (kInputDimensions % kSimdWidth == 0) {
      constexpr IndexType kNumChunks = kInputDimensions / kSimdWidth;
      const __m256i offsets = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
      const auto in = reinterpret_cast<const __m256i*>(input);
      const auto out = reinterpret_cast<__m256i*>(output);
      for (IndexType i = 0; i < kNumChunks; ++i) {
        const __m256i words0 = _mm256_srli_epi16(
            _mm256_packus_epi32(_mm256_load_si256(&in[i * 4 + 0]),
                                _mm256_load_si256(&in[i * 4 + 1])),
            kWeightScaleBits);
        const __m256i words1 = _mm256_srli_epi16(
            _mm256_packus_epi32(_mm256_load_si256(&in[i * 4 + 2]),
                                _mm256_load_si256(&in[i * 4 + 3])),
            kWeightScaleBits);
        _mm256_store_si256(&out[i],
                           _mm256_permutevar8x32_epi32(
                               _mm256_packs_epi16(words0, words1), offsets));
      }
    } else {
      constexpr IndexType kNumChunks = kInputDimensions / (kSimdWidth / 2);
      const auto in = reinterpret_cast<const __m128i*>(input);
      const auto out = reinterpret_cast<__m128i*>(output);
      for (IndexType i = 0; i < kNumChunks; ++i) {
        const __m128i words0 =
            _mm_srli_epi16(_mm_packus_epi32(_mm_load_si128(&in[i * 4 + 0]),
                                            _mm_load_si128(&in[i * 4 + 1])),
                           kWeightScaleBits);
        const __m128i words1 =
            _mm_srli_epi16(_mm_packus_epi32(_mm_load_si128(&in[i * 4 + 2]),
                                            _mm_load_si128(&in[i * 4 + 3])),
                           kWeightScaleBits);
        _mm_store_si128(&out[i], _mm_packs_epi16(words0, words1));
      }
    }
    constexpr IndexType kStart =
        kInputDimensions % kSimdWidth == 0
            ? kInputDimensions / kSimdWidth * kSimdWidth
            : kInputDimensions / (kSimdWidth / 2) * (kSimdWidth / 2);
#else
    constexpr IndexType kStart = 0;
#endif
    for (IndexType i = kStart; i < kInputDimensions; ++i) {
      output[i] = static_cast<OutputType>(
          std::clamp(input[i] >> kWeightScaleBits, 0, 127));
    }
  }
};

template <IndexType kInDims>
class SqrClippedReLU {
 public:
  using InputType = std::int32_t;
  using OutputType = std::uint8_t;

  static constexpr IndexType kInputDimensions = kInDims;
  static constexpr IndexType kOutputDimensions = kInputDimensions;
  static constexpr IndexType kPaddedOutputDimensions =
      CeilToMultiple<IndexType>(kOutputDimensions, 32);

  using OutputBuffer = OutputType[kPaddedOutputDimensions];

  static constexpr std::uint32_t GetHashValue(const std::uint32_t prev_hash) {
    return 0x538D24C7u + prev_hash;
  }

  bool ReadParameters(std::istream&) { return true; }

  void Propagate(const InputType* input, OutputType* output) const {
#if defined(SCE_NNUE_AVX2)
    constexpr IndexType kNumChunks = kInputDimensions / 16;
    static_assert(kWeightScaleBits == 6);
    const auto in = reinterpret_cast<const __m128i*>(input);
    const auto out = reinterpret_cast<__m128i*>(output);
    for (IndexType i = 0; i < kNumChunks; ++i) {
      __m128i words0 = _mm_packs_epi32(_mm_load_si128(&in[i * 4 + 0]),
                                       _mm_load_si128(&in[i * 4 + 1]));
      __m128i words1 = _mm_packs_epi32(_mm_load_si128(&in[i * 4 + 2]),
                                       _mm_load_si128(&in[i * 4 + 3]));
      words0 = _mm_srli_epi16(_mm_mulhi_epi16(words0, words0), 3);
      words1 = _mm_srli_epi16(_mm_mulhi_epi16(words1, words1), 3);
      _mm_store_si128(&out[i], _mm_packs_epi16(words0, words1));
    }
    constexpr IndexType kStart = kNumChunks * 16;
#else
    constexpr IndexType kStart = 0;
#endif
    for (IndexType i = kStart; i < kInputDimensions; ++i) {
      output[i] = static_cast<OutputType>(
          std::min(127ll, (static_cast<long long>(input[i]) * input[i]) >>
                              (2 * kWeightScaleBits + 7)));
    }
  }
};
}  // namespace SimpleChessEngine::Nnue::Layers

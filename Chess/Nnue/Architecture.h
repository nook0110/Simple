#pragma once

#include <cstdint>
#include <cstring>
#include <istream>

#include "Common.h"
#include "Layers.h"

namespace SimpleChessEngine::Nnue {
template <IndexType kL1, int kL2, int kL3>
struct NetworkArchitecture {
  static constexpr IndexType kTransformedFeatureDimensions = kL1;
  static constexpr int kFc0Outputs = kL2;
  static constexpr int kFc1Outputs = kL3;

  Layers::AffineTransformSparseInput<kTransformedFeatureDimensions,
                                     kFc0Outputs + 1>
      fc_0;
  Layers::SqrClippedReLU<kFc0Outputs + 1> ac_sqr_0;
  Layers::ClippedReLU<kFc0Outputs + 1> ac_0;
  Layers::AffineTransform<kFc0Outputs * 2, kFc1Outputs> fc_1;
  Layers::ClippedReLU<kFc1Outputs> ac_1;
  Layers::AffineTransform<kFc1Outputs, 1> fc_2;

  static constexpr std::uint32_t GetHashValue() {
    std::uint32_t hash_value = 0xEC42E90Du;
    hash_value ^= kTransformedFeatureDimensions * 2;
    hash_value = decltype(fc_0)::GetHashValue(hash_value);
    hash_value = decltype(ac_0)::GetHashValue(hash_value);
    hash_value = decltype(fc_1)::GetHashValue(hash_value);
    hash_value = decltype(ac_1)::GetHashValue(hash_value);
    hash_value = decltype(fc_2)::GetHashValue(hash_value);
    return hash_value;
  }

  bool ReadParameters(std::istream& stream) {
    return fc_0.ReadParameters(stream) && ac_0.ReadParameters(stream) &&
           fc_1.ReadParameters(stream) && ac_1.ReadParameters(stream) &&
           fc_2.ReadParameters(stream);
  }

  std::int32_t Propagate(
      const TransformedFeatureType* transformed_features,
      const std::uint16_t* nonzero, const IndexType nonzero_count) const {
    struct alignas(kCacheLineSize) Buffer {
      alignas(kCacheLineSize) typename decltype(fc_0)::OutputBuffer fc_0_out;
      alignas(kCacheLineSize) typename decltype(ac_sqr_0)::OutputType
          ac_sqr_0_out[CeilToMultiple<IndexType>(kFc0Outputs * 2, 32)];
      alignas(kCacheLineSize) typename decltype(ac_0)::OutputBuffer ac_0_out;
      alignas(kCacheLineSize) typename decltype(fc_1)::OutputBuffer fc_1_out;
      alignas(kCacheLineSize) typename decltype(ac_1)::OutputBuffer ac_1_out;
      alignas(kCacheLineSize) typename decltype(fc_2)::OutputBuffer fc_2_out;
    };

    alignas(kCacheLineSize) static thread_local Buffer buffer{};

    fc_0.Propagate(transformed_features, buffer.fc_0_out, nonzero,
                    nonzero_count);
    ac_sqr_0.Propagate(buffer.fc_0_out, buffer.ac_sqr_0_out);
    ac_0.Propagate(buffer.fc_0_out, buffer.ac_0_out);
    std::memcpy(buffer.ac_sqr_0_out + kFc0Outputs, buffer.ac_0_out,
                kFc0Outputs * sizeof(typename decltype(ac_0)::OutputType));
    fc_1.Propagate(buffer.ac_sqr_0_out, buffer.fc_1_out);
    ac_1.Propagate(buffer.fc_1_out, buffer.ac_1_out);
    fc_2.Propagate(buffer.ac_1_out, buffer.fc_2_out);

    const std::int32_t forward_output = buffer.fc_0_out[kFc0Outputs] *
                                        (600 * kOutputScale) /
                                        (127 * (1 << kWeightScaleBits));
    return buffer.fc_2_out[0] + forward_output;
  }
};
}  // namespace SimpleChessEngine::Nnue

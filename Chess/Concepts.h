#pragma once
#include <concepts>

template <class T>
concept StopSearchCondition = requires(const T& condition) {
  { condition.IsTimeToExit() } -> std::convertible_to<bool>;
};

#pragma once
#include <concepts>

template <class T>
concept StopSearchCondition = requires(const T& condition) {
  { condition.IsTimeToExit() } -> std::convertible_to<bool>;
};

template <class T>
concept CountsNodes = requires(const T& condition) { condition.CountNode(); };

template <class T>
concept ReportsNodes = requires { typename T::NodeReportingTag; };

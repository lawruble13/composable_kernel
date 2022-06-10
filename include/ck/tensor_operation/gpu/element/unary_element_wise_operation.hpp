#pragma once
#include "data_type.hpp"
#include "math_v2.hpp"

namespace ck {
namespace tensor_operation {
namespace element_wise {

struct PassThrough
{
    template <typename T>
    __host__ __device__ void operator()(T& y, const T& x) const
    {
        static_assert(is_same<T, float>::value || is_same<T, double>::value ||
                          is_same<T, half_t>::value || is_same<T, bhalf_t>::value ||
                          is_same<T, int32_t>::value || is_same<T, int8_t>::value,
                      "Data type is not supported by this operation!");

        y = x;
    };
};

struct UnaryDivide
{
    __host__ __device__ UnaryDivide(const int32_t divider = 1) : divider_(divider){};

    template <typename T>
    __host__ __device__ void operator()(T& y, const T& x) const;

    template <>
    __host__ __device__ void operator()<float>(float& y, const float& x) const
    {
        y = x / type_convert<float>(divider_);
    };

    template <>
    __host__ __device__ void operator()<double>(double& y, const double& x) const
    {
        y = x / type_convert<double>(divider_);
    };

    template <>
    __host__ __device__ void operator()<int32_t>(int32_t& y, const int32_t& x) const
    {
        y = x / type_convert<int32_t>(divider_);
    };

    int32_t divider_ = 1;
};

struct UnarySquare
{
    template <typename T>
    __host__ __device__ void operator()(T& y, const T& x) const
    {
        static_assert(is_same<T, float>::value || is_same<T, double>::value,
                      "Data type is not supported by this operation!");

        y = x * x;
    };
};

struct UnaryAbs
{
    template <typename T>
    __host__ __device__ void operator()(T& y, const T& x) const
    {
        static_assert(is_same<T, float>::value || is_same<T, double>::value ||
                          is_same<T, half_t>::value || is_same<T, int32_t>::value ||
                          is_same<T, int8_t>::value,
                      "Data type is not supported by this operation!");

        y = ck::math::abs(x);
    };
};

struct UnarySqrt
{
    template <typename T>
    __host__ __device__ void operator()(T& y, const T& x) const
    {
        static_assert(is_same<T, float>::value || is_same<T, double>::value,
                      "Data type is not supported by this operation!");

        y = ck::math::sqrt(x);
    };
};

} // namespace element_wise
} // namespace tensor_operation
} // namespace ck

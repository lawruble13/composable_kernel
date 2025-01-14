#include <tuple>
#include <vector>
#include "gtest/gtest.h"

#include "ck/library/utility/conv_util.hpp"
#include "config.hpp"
#include "conv_util.hpp"
#include "data_type.hpp"
#include "element_wise_operation.hpp"
#include "fill.hpp"

namespace {

class Conv2dFwdNHWCInstances : public ::testing::Test
{
    public:
    template <typename T>
    bool test_conv2d_nhwc_instances(const std::vector<test::conv::DeviceConvFwdNoOpPtr>& conv_ptrs,
                                    const ck::utils::conv::ConvParams& params)
    {
        using namespace std::placeholders;
        using namespace ck::utils;

        conv::ConvFwdOpInstance<T,
                                T,
                                T,
                                ck::tensor_layout::convolution::NHWC,
                                ck::tensor_layout::convolution::KYXC,
                                ck::tensor_layout::convolution::NHWK,
                                ck::tensor_operation::element_wise::PassThrough,
                                ck::tensor_operation::element_wise::PassThrough,
                                ck::tensor_operation::element_wise::PassThrough,
                                FillUniformDistributionIntegerValue<T>,
                                FillUniformDistributionIntegerValue<T>>
            conv_instance(params,
                          true,
                          FillUniformDistributionIntegerValue<T>{},
                          FillUniformDistributionIntegerValue<T>{});
        auto reference_conv_fwd_fun =
            std::bind(conv::run_reference_convolution_forward<2, T, T, T>, params, _1, _2, _3);
        OpInstanceRunEngine<T, T, T> run_engine(conv_instance, reference_conv_fwd_fun);
        run_engine.SetAtol(atol_);
        run_engine.SetRtol(rtol_);
        return run_engine.Test(conv_ptrs);
    }

    template <typename T>
    bool test_default(bool use_convnd = false)
    {
        if(use_convnd)
        {
            return test_conv2d_nhwc_instances<T>(
                test::conv::ConvolutionNDFwdInstances<T, T, T>::Get(2), params_default_);
        }
        else
        {
            return test_conv2d_nhwc_instances<T>(
                ck::utils::conv::ConvolutionFwdInstances<T, T, T>::template Get<2>(),
                params_default_);
        }
    }

    template <typename T>
    bool test_filter1x1_stride1_pad0(bool use_convnd = false)
    {
        if(use_convnd)
        {
            return test_conv2d_nhwc_instances<T>(
                test::conv::ConvolutionNDFwdInstances<T, T, T>::Get(2),
                params_filter1x1_stride1_pad0_);
        }
        else
        {
            return test_conv2d_nhwc_instances<T>(
                ck::utils::conv::ConvolutionFwdInstances<T, T, T>::template Get<2>(),
                params_filter1x1_stride1_pad0_);
        }
    }

    template <typename T>
    bool test_filter1x1_pad0(bool use_convnd = false)
    {
        if(use_convnd)
        {
            return test_conv2d_nhwc_instances<T>(
                test::conv::ConvolutionNDFwdInstances<T, T, T>::Get(2), params_filter1x1_pad0_);
        }
        else
        {
            return test_conv2d_nhwc_instances<T>(
                ck::utils::conv::ConvolutionFwdInstances<T, T, T>::template Get<2>(),
                params_filter1x1_pad0_);
        }
    }

    template <typename T>
    bool test_oddC()
    {
        return test_conv2d_nhwc_instances<T>(
            ck::utils::conv::ConvolutionFwdInstances<T, T, T>::template Get<2>(), params_oddC_);
    }

    static inline ck::utils::conv::ConvParams params_default_{
        2, 4, 256, 64, {3, 3}, {36, 36}, {2, 2}, {2, 2}, {2, 2}, {2, 2}};
    static inline ck::utils::conv::ConvParams params_filter1x1_stride1_pad0_{
        2, 4, 256, 64, {1, 1}, {28, 28}, {1, 1}, {1, 1}, {0, 0}, {0, 0}};
    static inline ck::utils::conv::ConvParams params_filter1x1_pad0_{
        2, 4, 256, 64, {1, 1}, {28, 28}, {2, 2}, {1, 1}, {0, 0}, {0, 0}};
    static inline ck::utils::conv::ConvParams params_oddC_{
        2, 4, 256, 3, {3, 3}, {28, 28}, {1, 1}, {1, 1}, {0, 0}, {0, 0}};

    private:
    double atol_{1e-5};
    double rtol_{1e-4};
};

} // anonymous namespace

TEST(Conv2DFwdNHWC, IntegerValues)
{
    using namespace std::placeholders;
    using namespace ck::utils;
    using T = float;

    ck::utils::conv::ConvParams params{
        2, 4, 256, 64, {3, 3}, {36, 36}, {1, 1}, {2, 2}, {2, 2}, {2, 2}};

    std::vector<test::conv::DeviceConvFwdNoOpPtr> conv_ptrs;
    test::conv::get_test_convolution_fwd_instance<2, T, T, T, T>(conv_ptrs);
    conv::ConvFwdOpInstance<T,
                            T,
                            T,
                            ck::tensor_layout::convolution::NHWC,
                            ck::tensor_layout::convolution::KYXC,
                            ck::tensor_layout::convolution::NHWK,
                            ck::tensor_operation::element_wise::PassThrough,
                            ck::tensor_operation::element_wise::PassThrough,
                            ck::tensor_operation::element_wise::PassThrough,
                            FillUniformDistributionIntegerValue<T>,
                            FillUniformDistributionIntegerValue<T>>
        conv_instance(params,
                      true,
                      FillUniformDistributionIntegerValue<T>{},
                      FillUniformDistributionIntegerValue<T>{});

    auto reference_conv_fwd_fun =
        std::bind(conv::run_reference_convolution_forward<2, T, T, T>, params, _1, _2, _3);
    OpInstanceRunEngine<T, T, T> run_engine(conv_instance, reference_conv_fwd_fun);
    run_engine.SetAtol(1e-5);
    run_engine.SetRtol(1e-4);
    EXPECT_TRUE(run_engine.Test(conv_ptrs));
}

TEST(Conv2DFwdNHWC, FloatingPointValues)
{
    using namespace std::placeholders;
    using namespace ck::utils;
    using T = ck::half_t;

    ck::utils::conv::ConvParams params{
        2, 4, 256, 64, {3, 3}, {36, 36}, {2, 2}, {2, 2}, {2, 2}, {2, 2}};

    std::vector<test::conv::DeviceConvFwdNoOpPtr> conv_ptrs;
    test::conv::get_test_convolution_fwd_instance<2, T, T, T, float>(conv_ptrs);
    conv::ConvFwdOpInstance<T,
                            T,
                            T,
                            ck::tensor_layout::convolution::NHWC,
                            ck::tensor_layout::convolution::KYXC,
                            ck::tensor_layout::convolution::NHWK,
                            ck::tensor_operation::element_wise::PassThrough,
                            ck::tensor_operation::element_wise::PassThrough,
                            ck::tensor_operation::element_wise::PassThrough,
                            FillUniformDistribution<T>,
                            FillUniformDistribution<T>>
        conv_instance(params, true, FillUniformDistribution<T>{}, FillUniformDistribution<T>{});

    auto reference_conv_fwd_fun =
        std::bind(conv::run_reference_convolution_forward<2, T, T, T>, params, _1, _2, _3);
    OpInstanceRunEngine<T, T, T> run_engine(conv_instance, reference_conv_fwd_fun);
    run_engine.SetAtol(2e-4);
    run_engine.SetRtol(1e-3);
    EXPECT_TRUE(run_engine.Test(conv_ptrs));
}

TEST_F(Conv2dFwdNHWCInstances, BF16_default) { EXPECT_TRUE(this->test_default<ck::bhalf_t>()); }
TEST_F(Conv2dFwdNHWCInstances, BF16_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<ck::bhalf_t>());
}
TEST_F(Conv2dFwdNHWCInstances, BF16_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<ck::bhalf_t>());
}
TEST_F(Conv2dFwdNHWCInstances, F16_default) { EXPECT_TRUE(this->test_default<ck::half_t>()); }
TEST_F(Conv2dFwdNHWCInstances, F16_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<ck::half_t>());
}
TEST_F(Conv2dFwdNHWCInstances, F16_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<ck::half_t>());
}
TEST_F(Conv2dFwdNHWCInstances, F16_oddC) { EXPECT_TRUE(this->test_oddC<ck::half_t>()); }
TEST_F(Conv2dFwdNHWCInstances, F32_default) { EXPECT_TRUE(this->test_default<float>()); }
TEST_F(Conv2dFwdNHWCInstances, F32_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<float>());
}
TEST_F(Conv2dFwdNHWCInstances, F32_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<float>());
}
TEST_F(Conv2dFwdNHWCInstances, I8_default) { EXPECT_TRUE(this->test_default<int8_t>()); }
TEST_F(Conv2dFwdNHWCInstances, I8_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<int8_t>());
}
TEST_F(Conv2dFwdNHWCInstances, I8_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<int8_t>());
}

TEST_F(Conv2dFwdNHWCInstances, ND_BF16_default)
{
    EXPECT_TRUE(this->test_default<ck::bhalf_t>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_BF16_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<ck::bhalf_t>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_BF16_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<ck::bhalf_t>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_F16_default)
{
    EXPECT_TRUE(this->test_default<ck::half_t>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_F16_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<ck::half_t>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_F16_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<ck::half_t>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_F32_default) { EXPECT_TRUE(this->test_default<float>(true)); }
TEST_F(Conv2dFwdNHWCInstances, ND_F32_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<float>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_F32_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<float>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_I8_default) { EXPECT_TRUE(this->test_default<int8_t>(true)); }
TEST_F(Conv2dFwdNHWCInstances, ND_I8_filter1x1_stride1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_stride1_pad0<int8_t>(true));
}
TEST_F(Conv2dFwdNHWCInstances, ND_I8_filter1x1_pad0)
{
    EXPECT_TRUE(this->test_filter1x1_pad0<int8_t>(true));
}

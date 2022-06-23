#include <limits>
#include <iostream>
#include <getopt.h>
#include "check_err.hpp"
#include "config.hpp"
#include "print.hpp"
#include "device.hpp"
#include "host_tensor.hpp"
#include "host_tensor_generator.hpp"
#include "host_common_util.hpp"
#include "device_tensor.hpp"
#include "batchnorm_infer_impl.hpp"
#include "reference_batchnorm_infer_nhwc_c.hpp"

template <typename InOutDataType, typename AccDataType>
using ReferenceBatchNormInferInstance =
    ck::tensor_operation::host::ReferenceBatchNormInfer_Input_N_H_W_C_Output_C<InOutDataType,
                                                                               AccDataType>;

static struct option long_options[] = {{"inOutLengths", required_argument, nullptr, 'D'},
                                       {"verify", required_argument, nullptr, 'v'},
                                       {"help", no_argument, nullptr, '?'},
                                       {nullptr, 0, nullptr, 0}};

class BatchNormInferArg
{
    private:
    int option_index = 0;

    public:
    std::vector<size_t> inOutLengths;

    bool do_verification = false;

    int init_method  = 2;
    bool time_kernel = false;

    public:
    void show_usage(const char* cmd)
    {
        std::cout << "Usage of " << cmd << std::endl;
        std::cout << "--inOutLengths or -D, comma separated list of input tensor dimension "
                     "lengths, must have 4 integers for nhwc"
                  << std::endl;
        std::cout << "--verify or -v, 1/0 to indicate whether to verify the batch-normalization "
                     "result by "
                     "comparing with the host-based batch-normalization"
                  << std::endl;
        std::cout << "Arg1 -- init method used for bnScale and bnBias (0=no init, 1=single integer "
                     "value, 2=scope integer "
                     "value, 3=decimal value)"
                  << std::endl;
        std::cout << "Arg2 -- time kernel (0=no, 1=yes)" << std::endl;
    };

    int processArgs(int argc, char* argv[])
    {
        using ck::host_common::getTypeValuesFromString;

        int ch;

        while(1)
        {
            ch = getopt_long(argc, argv, "D:v:", long_options, &option_index);
            if(ch == -1)
                break;
            switch(ch)
            {
            case 'D':
                if(!optarg)
                    throw std::runtime_error("Invalid option format!");

                inOutLengths = getTypeValuesFromString<size_t>(optarg);

                if(inOutLengths.size() != 4)
                    throw std::runtime_error(
                        "NHWC tensor layout should have 4 length values specified!");
                break;
            case 'v':
                if(!optarg)
                    throw std::runtime_error("Invalid option format!");

                do_verification = static_cast<bool>(std::atoi(optarg));
                break;
            case '?':
                if(std::string(long_options[option_index].name) == "help")
                {
                    show_usage(argv[0]);
                    return (-1);
                };
                break;
            default: show_usage(argv[0]); return (-1);
            };
        };

        if(optind + 2 > argc)
            throw std::runtime_error("Invalid cmd-line arguments, more argumetns are needed!");

        init_method = std::atoi(argv[optind++]);
        time_kernel = static_cast<bool>(std::atoi(argv[optind]));

        return (0);
    };
};

using namespace ck;

template <typename InOutDataType, typename AccDataType>
bool bnorm_infer_nhwc_test(bool do_verification,
                           int init_method,
                           bool time_kernel,
                           const std::vector<size_t> inOutLengths,
                           double epsilon)
{
    // for NHWC BatchNorm calculation of mean and meansquare
    constexpr int Rank         = 4;
    constexpr int NumReduceDim = 3;

    const std::vector<size_t> scaleBiasMeanVarLengths = {inOutLengths[3]};

    // input data of the batchnorm forward algorithm
    Tensor<InOutDataType> x(inOutLengths);
    Tensor<AccDataType> bnScale(scaleBiasMeanVarLengths);
    Tensor<AccDataType> bnBias(scaleBiasMeanVarLengths);

    // output data of the batchnorm forward algorithm
    Tensor<InOutDataType> y_ref(inOutLengths);
    Tensor<InOutDataType> y(inOutLengths);

    Tensor<AccDataType> estimatedMean(scaleBiasMeanVarLengths);
    Tensor<AccDataType> estimatedVariance(scaleBiasMeanVarLengths);

    auto inOutStrides            = x.mDesc.GetStrides();
    auto scaleBiasMeanVarStrides = bnScale.mDesc.GetStrides();

    std::size_t num_thread = std::thread::hardware_concurrency();

    const float x_mean       = 0.0f;
    const float x_stddev     = 1.0f;
    const float noise_stddev = 0.0001f;

    // input data in normal distribution
    x.GenerateTensorValue(GeneratorTensor_4<InOutDataType>{x_mean, x_stddev}, num_thread);

    // initialize the savedMean to be values with tiny variation to the mean of the x values
    estimatedMean.GenerateTensorValue(GeneratorTensor_4<AccDataType>{x_mean, noise_stddev},
                                      num_thread);

    // initialize the variance to be values with tiny variation to the variance of the x values
    estimatedVariance.GenerateTensorValue(
        GeneratorTensor_4<AccDataType>{x_stddev * x_stddev, noise_stddev}, num_thread);

    if(do_verification)
    {
        switch(init_method)
        {
        case 0:
            bnScale.GenerateTensorValue(GeneratorTensor_0<AccDataType>{}, num_thread);
            bnBias.GenerateTensorValue(GeneratorTensor_0<AccDataType>{}, num_thread);
            break;
        case 1:
            bnScale.GenerateTensorValue(GeneratorTensor_1<AccDataType>{1}, num_thread);
            bnBias.GenerateTensorValue(GeneratorTensor_1<AccDataType>{0}, num_thread);
            break;
        case 2:
            bnScale.GenerateTensorValue(GeneratorTensor_2<AccDataType>{-5, 5}, num_thread);
            bnBias.GenerateTensorValue(GeneratorTensor_2<AccDataType>{-5, 5}, num_thread);
            break;
        default:
            bnScale.GenerateTensorValue(GeneratorTensor_3<AccDataType>{-5.0f, 5.0f}, num_thread);
            bnBias.GenerateTensorValue(GeneratorTensor_3<AccDataType>{-5.0f, 5.0f}, num_thread);
        }
    };

    // these buffers are usually provided by the user application
    DeviceMem x_dev(sizeof(InOutDataType) * x.mDesc.GetElementSpace());
    DeviceMem y_dev(sizeof(InOutDataType) * y.mDesc.GetElementSpace());
    DeviceMem bnScale_dev(sizeof(AccDataType) * bnScale.mDesc.GetElementSpace());
    DeviceMem bnBias_dev(sizeof(AccDataType) * bnBias.mDesc.GetElementSpace());

    // mean_dev or resultSaveMean_dev
    DeviceMem estimatedMean_dev(sizeof(AccDataType) * estimatedMean.mDesc.GetElementSpace());
    // meansquare_dev or resultSaveInvVariance_dev
    DeviceMem estimatedVariance_dev(sizeof(AccDataType) *
                                    estimatedVariance.mDesc.GetElementSpace());

    x_dev.ToDevice(x.mData.data());
    bnScale_dev.ToDevice(bnScale.mData.data());
    bnBias_dev.ToDevice(bnBias.mData.data());
    estimatedMean_dev.ToDevice(estimatedMean.mData.data());
    estimatedVariance_dev.ToDevice(estimatedVariance.mData.data());

    std::vector<ck::index_t> i_inOutLengths;
    std::vector<ck::index_t> i_inOutStrides;
    std::vector<ck::index_t> i_scaleBiasMeanVarLengths;
    std::vector<ck::index_t> i_scaleBiasMeanVarStrides;

    i_inOutLengths.assign(inOutLengths.begin(), inOutLengths.end());
    i_inOutStrides.assign(inOutStrides.begin(), inOutStrides.end());
    i_scaleBiasMeanVarLengths.assign(scaleBiasMeanVarLengths.begin(),
                                     scaleBiasMeanVarLengths.end());
    i_scaleBiasMeanVarStrides.assign(scaleBiasMeanVarStrides.begin(),
                                     scaleBiasMeanVarStrides.end());

    int result = 0;

    result = batchnorm::bnorm_infer<InOutDataType, AccDataType, Rank, NumReduceDim, false>(
        time_kernel,
        {0, 1, 2},
        i_inOutLengths,
        i_inOutStrides,
        i_inOutStrides,
        i_scaleBiasMeanVarLengths,
        i_scaleBiasMeanVarStrides,
        x_dev.GetDeviceBuffer(),
        bnScale_dev.GetDeviceBuffer(),
        bnBias_dev.GetDeviceBuffer(),
        epsilon,
        estimatedMean_dev.GetDeviceBuffer(),
        estimatedVariance_dev.GetDeviceBuffer(),
        y_dev.GetDeviceBuffer());

    if(result < 0)
        return (false);

    bool pass = true;

    if(do_verification)
    {
        auto batchNormInfer_ref = ReferenceBatchNormInferInstance<InOutDataType, AccDataType>{};

        auto argument_ptr_ref =
            batchNormInfer_ref.MakeArgumentPointer(i_inOutLengths,
                                                   i_inOutStrides,
                                                   i_inOutStrides,
                                                   i_scaleBiasMeanVarLengths,
                                                   i_scaleBiasMeanVarStrides,
                                                   x.mData.data(),
                                                   bnScale.mData.data(),
                                                   bnBias.mData.data(),
                                                   epsilon,
                                                   estimatedMean.mData.data(),
                                                   estimatedVariance.mData.data(),
                                                   y_ref.mData.data());

        if(!batchNormInfer_ref.IsSupportedArgument(argument_ptr_ref.get()))
        {
            std::cout
                << "The runtime parameters seems not supported by the BatchNorm instance, exiting!"
                << std::endl;
            return (-2);
        };

        auto invoker_ptr_ref = batchNormInfer_ref.MakeInvokerPointer();

        (void)invoker_ptr_ref->Run(argument_ptr_ref.get());

        y_dev.FromDevice(y.mData.data());
        pass = pass && ck::utils::check_err(y.mData, y_ref.mData);
    };

    return (pass);
};

using InOutDataType = ck::half_t;
using AccDataType   = float;

static const double epsilon = std::numeric_limits<AccDataType>::epsilon();

int main(int argc, char* argv[])
{
    bool pass = true;

    if(argc > 1)
    {
        BatchNormInferArg arg;

        if(arg.processArgs(argc, argv) < 0)
            return (-1);

        pass = bnorm_infer_nhwc_test<InOutDataType, AccDataType>(
            arg.do_verification, arg.init_method, arg.time_kernel, arg.inOutLengths, epsilon);
    }
    else
    {
        pass = bnorm_infer_nhwc_test<InOutDataType, AccDataType>(true,
                                                                 2,
                                                                 false, // don't time kernel
                                                                 {128, 16, 16, 1024},
                                                                 epsilon);
    };

    return (pass ? 0 : 1);
}
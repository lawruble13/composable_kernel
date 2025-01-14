#pragma once

#include <iomanip>

#include "check_err.hpp"
#include "config.hpp"
#include "device.hpp"
#include "host_tensor.hpp"
#include "host_tensor_generator.hpp"
#include "host_conv.hpp"
#include "tensor_layout.hpp"
#include "device_tensor.hpp"
#include "element_wise_operation.hpp"
#include "reference_gemm.hpp"
#include "device_gemm_multiple_d.hpp"

namespace ck {
namespace tensor_operation {
namespace device {
namespace device_gemm_instance {

using DeviceGemmAddAddFastGeluPtr = ck::tensor_operation::device::DeviceGemmMultipleDPtr<
    2,
    ck::tensor_operation::element_wise::PassThrough,
    ck::tensor_operation::element_wise::PassThrough,
    ck::tensor_operation::element_wise::AddAddFastGelu>;

void add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_mk_kn_mn_instances(
    std::vector<DeviceGemmAddAddFastGeluPtr>&);
void add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_mk_nk_mn_instances(
    std::vector<DeviceGemmAddAddFastGeluPtr>&);
void add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_km_kn_mn_instances(
    std::vector<DeviceGemmAddAddFastGeluPtr>&);
void add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_km_nk_mn_instances(
    std::vector<DeviceGemmAddAddFastGeluPtr>&);

} // namespace device_gemm_instance
} // namespace device
} // namespace tensor_operation
} // namespace ck

namespace ck {
namespace profiler {

template <typename ADataType,
          typename BDataType,
          typename AccDataType,
          typename D0DataType,
          typename D1DataType,
          typename EDataType,
          typename ALayout,
          typename BLayout,
          typename D0Layout,
          typename D1Layout,
          typename ELayout>
int profile_gemm_add_add_fastgelu_impl(int do_verification,
                                       int init_method,
                                       bool /*do_log*/,
                                       bool time_kernel,
                                       int M,
                                       int N,
                                       int K,
                                       int StrideA,
                                       int StrideB,
                                       int StrideD0,
                                       int StrideD1,
                                       int StrideE)
{
    auto f_host_tensor_descriptor =
        [](std::size_t row, std::size_t col, std::size_t stride, auto layout) {
            if(is_same<decltype(layout), tensor_layout::gemm::RowMajor>::value)
            {
                return HostTensorDescriptor(std::vector<std::size_t>({row, col}),
                                            std::vector<std::size_t>({stride, 1}));
            }
            else
            {
                return HostTensorDescriptor(std::vector<std::size_t>({row, col}),
                                            std::vector<std::size_t>({1, stride}));
            }
        };

    Tensor<ADataType> a_m_k(f_host_tensor_descriptor(M, K, StrideA, ALayout{}));
    Tensor<BDataType> b_k_n(f_host_tensor_descriptor(K, N, StrideB, BLayout{}));
    Tensor<D0DataType> d0_m_n(f_host_tensor_descriptor(M, N, StrideD0, D0Layout{}));
    Tensor<D1DataType> d1_m_n(f_host_tensor_descriptor(M, N, StrideD1, D1Layout{}));
    Tensor<EDataType> e_m_n_device_result(f_host_tensor_descriptor(M, N, StrideE, ELayout{}));
    Tensor<EDataType> e_m_n_host_result(f_host_tensor_descriptor(M, N, StrideE, ELayout{}));

    std::cout << "a_m_k: " << a_m_k.mDesc << std::endl;
    std::cout << "b_k_n: " << b_k_n.mDesc << std::endl;
    std::cout << "d0_m_n: " << d0_m_n.mDesc << std::endl;
    std::cout << "d1_m_n: " << d1_m_n.mDesc << std::endl;
    std::cout << "e_m_n: " << e_m_n_device_result.mDesc << std::endl;

    switch(init_method)
    {
    case 0: break;
    case 1:
        a_m_k.GenerateTensorValue(GeneratorTensor_2<ADataType>{-5, 5});
        b_k_n.GenerateTensorValue(GeneratorTensor_2<BDataType>{-5, 5});
        d0_m_n.GenerateTensorValue(GeneratorTensor_2<D0DataType>{-5, 5});
        d1_m_n.GenerateTensorValue(GeneratorTensor_2<D1DataType>{-5, 5});
        break;
    default:
        a_m_k.GenerateTensorValue(GeneratorTensor_3<ADataType>{0.0, 1.0});
        b_k_n.GenerateTensorValue(GeneratorTensor_3<BDataType>{-0.5, 0.5});
        d0_m_n.GenerateTensorValue(GeneratorTensor_3<D0DataType>{0.0, 1.0});
        d1_m_n.GenerateTensorValue(GeneratorTensor_3<D1DataType>{0.0, 1.0});
    }

    using PassThrough    = ck::tensor_operation::element_wise::PassThrough;
    using AddAddFastGelu = ck::tensor_operation::element_wise::AddAddFastGelu;

    using AElementOp   = PassThrough;
    using BElementOp   = PassThrough;
    using CDEElementOp = AddAddFastGelu;

    const auto a_element_op   = AElementOp{};
    const auto b_element_op   = BElementOp{};
    const auto cde_element_op = CDEElementOp{};

    // add device GEMM instances
    std::vector<ck::tensor_operation::device::device_gemm_instance::DeviceGemmAddAddFastGeluPtr>
        device_op_ptrs;

    if constexpr(is_same_v<ADataType, half_t> && is_same_v<BDataType, half_t> &&
                 is_same_v<EDataType, half_t>)
    {
        if constexpr(is_same_v<ALayout, tensor_layout::gemm::RowMajor> &&
                     is_same_v<BLayout, tensor_layout::gemm::RowMajor> &&
                     is_same_v<ELayout, tensor_layout::gemm::RowMajor>)
        {
            ck::tensor_operation::device::device_gemm_instance::
                add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_mk_kn_mn_instances(
                    device_op_ptrs);
        }
        else if constexpr(is_same_v<ALayout, tensor_layout::gemm::RowMajor> &&
                          is_same_v<BLayout, tensor_layout::gemm::ColumnMajor> &&
                          is_same_v<ELayout, tensor_layout::gemm::RowMajor>)
        {
            ck::tensor_operation::device::device_gemm_instance::
                add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_mk_nk_mn_instances(
                    device_op_ptrs);
        }
        else if constexpr(is_same_v<ALayout, tensor_layout::gemm::ColumnMajor> &&
                          is_same_v<BLayout, tensor_layout::gemm::RowMajor> &&
                          is_same_v<ELayout, tensor_layout::gemm::RowMajor>)
        {
            ck::tensor_operation::device::device_gemm_instance::
                add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_km_kn_mn_instances(
                    device_op_ptrs);
        }
        else if constexpr(is_same_v<ALayout, tensor_layout::gemm::ColumnMajor> &&
                          is_same_v<BLayout, tensor_layout::gemm::ColumnMajor> &&
                          is_same_v<ELayout, tensor_layout::gemm::RowMajor>)
        {
            ck::tensor_operation::device::device_gemm_instance::
                add_device_gemm_add_add_fastgelu_xdl_c_shuffle_f16_f16_f16_km_nk_mn_instances(
                    device_op_ptrs);
        }
    }

    std::cout << "found " << device_op_ptrs.size() << " instances" << std::endl;

    // run reference
    if(do_verification)
    {
        Tensor<AccDataType> c_m_n(HostTensorDescriptor(
            std::vector<std::size_t>{static_cast<std::size_t>(M), static_cast<std::size_t>(N)}));

        using ReferenceGemmInstance = ck::tensor_operation::host::ReferenceGemm<ADataType,
                                                                                BDataType,
                                                                                AccDataType,
                                                                                AccDataType,
                                                                                AElementOp,
                                                                                BElementOp,
                                                                                PassThrough>;

        auto ref_gemm    = ReferenceGemmInstance{};
        auto ref_invoker = ref_gemm.MakeInvoker();

        auto ref_argument =
            ref_gemm.MakeArgument(a_m_k, b_k_n, c_m_n, a_element_op, b_element_op, PassThrough{});

        ref_invoker.Run(ref_argument);

        for(int m = 0; m < M; ++m)
        {
            for(int n = 0; n < N; ++n)
            {
                cde_element_op(e_m_n_host_result(m, n), c_m_n(m, n), d0_m_n(m, n), d1_m_n(m, n));
            }
        }
    }

    DeviceMem a_device_buf(sizeof(ADataType) * a_m_k.mDesc.GetElementSpace());
    DeviceMem b_device_buf(sizeof(BDataType) * b_k_n.mDesc.GetElementSpace());
    DeviceMem d0_m_n_device_buf(sizeof(D0DataType) * d0_m_n.mDesc.GetElementSpace());
    DeviceMem d1_m_n_device_buf(sizeof(D1DataType) * d1_m_n.mDesc.GetElementSpace());
    DeviceMem e_device_buf(sizeof(EDataType) * e_m_n_device_result.mDesc.GetElementSpace());

    a_device_buf.ToDevice(a_m_k.mData.data());
    b_device_buf.ToDevice(b_k_n.mData.data());
    d0_m_n_device_buf.ToDevice(d0_m_n.mData.data());
    d1_m_n_device_buf.ToDevice(d1_m_n.mData.data());

    std::string best_device_op_name;
    float best_ave_time   = 0;
    float best_tflops     = 0;
    float best_gb_per_sec = 0;

    bool pass = true;

    // profile device operation instances
    for(auto& device_op_ptr : device_op_ptrs)
    {
        auto argument_ptr = device_op_ptr->MakeArgumentPointer(
            a_device_buf.GetDeviceBuffer(),
            b_device_buf.GetDeviceBuffer(),
            std::array<const void*, 2>{d0_m_n_device_buf.GetDeviceBuffer(),
                                       d1_m_n_device_buf.GetDeviceBuffer()},
            static_cast<EDataType*>(e_device_buf.GetDeviceBuffer()),
            M,
            N,
            K,
            StrideA,
            StrideB,
            std::array<ck::index_t, 2>{StrideD0, StrideD1},
            StrideE,
            a_element_op,
            b_element_op,
            cde_element_op);

        auto invoker_ptr = device_op_ptr->MakeInvokerPointer();

        std::string device_op_name = device_op_ptr->GetTypeString();

        if(device_op_ptr->IsSupportedArgument(argument_ptr.get()))
        {
            // re-init E to zero before profiling a kernel
            e_device_buf.SetZero();

            float ave_time =
                invoker_ptr->Run(argument_ptr.get(), StreamConfig{nullptr, time_kernel});

            std::size_t flop = std::size_t(2) * M * N * K;

            std::size_t num_btype =
                sizeof(ADataType) * M * K + sizeof(BDataType) * K * N + sizeof(EDataType) * M * N;

            float tflops = static_cast<float>(flop) / 1.E9 / ave_time;

            float gb_per_sec = num_btype / 1.E6 / ave_time;

            std::cout << "Perf: " << std::setw(10) << ave_time << " ms, " << tflops << " TFlops, "
                      << gb_per_sec << " GB/s, " << device_op_name << std::endl;

            if(tflops > best_tflops)
            {
                best_device_op_name = device_op_name;
                best_tflops         = tflops;
                best_ave_time       = ave_time;
                best_gb_per_sec     = gb_per_sec;
            }

            if(do_verification)
            {
                e_device_buf.FromDevice(e_m_n_device_result.mData.data());

                pass = pass &&
                       ck::utils::check_err(e_m_n_device_result.mData, e_m_n_host_result.mData);
            }
        }
        else
        {
            std::cout << device_op_name << " does not support this problem" << std::endl;
        }
    }

    std::cout << "Best Perf: " << best_ave_time << " ms, " << best_tflops << " TFlops, "
              << best_gb_per_sec << " GB/s, " << best_device_op_name << std::endl;

    return pass ? 0 : 1;
}

} // namespace profiler
} // namespace ck

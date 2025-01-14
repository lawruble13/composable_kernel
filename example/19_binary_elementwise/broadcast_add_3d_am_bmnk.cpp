#include <iostream>
#include <cstdlib>
#include "check_err.hpp"
#include "config.hpp"
#include "device.hpp"
#include "host_tensor.hpp"
#include "host_tensor_generator.hpp"

#include "device_tensor.hpp"
#include "binary_element_wise_operation.hpp"
#include "device_binary_elementwise.hpp"

using F16 = ck::half_t;
using F32 = float;

using ABDataType             = F16;
using CDataType              = F16;
using EltwiseComputeDataType = F32;

using Add = ck::tensor_operation::element_wise::Add;

using DeviceElementwiseAddInstance =
    ck::tensor_operation::device::DeviceBinaryElementwise<ABDataType,
                                                          ABDataType,
                                                          CDataType,
                                                          EltwiseComputeDataType,
                                                          Add,
                                                          3,
                                                          8,
                                                          1,
                                                          8,
                                                          8>;

template <typename HostTensorA,
          typename HostTensorB,
          typename HostTensorC,
          typename ComputeDataType,
          typename Functor>
void host_broadcast3D_am_bmnk(HostTensorC& C,
                              const HostTensorA& A,
                              const HostTensorB& B,
                              const std::vector<std::size_t>& shape,
                              Functor functor)
{
    using ctype = ck::remove_reference_t<decltype(C(0, 0))>;

    for(std::size_t m = 0; m < shape[0]; ++m)
        for(std::size_t n = 0; n < shape[1]; ++n)
            for(std::size_t k = 0; k < shape[2]; ++k)
            {
                ComputeDataType a_val = ck::type_convert<ComputeDataType>(A(m));
                ComputeDataType b_val = ck::type_convert<ComputeDataType>(B(m, n, k));
                ComputeDataType c_val = 0;
                functor(c_val, a_val, b_val);
                C(m, n, k) = ck::type_convert<ctype>(c_val);
            }
}

int main()
{
    bool do_verification = true;
    bool time_kernel     = false;

    std::vector<std::size_t> mnk = {4, 16, 32};
    ck::index_t M                = mnk[0];

    Tensor<ABDataType> a_m({M});
    Tensor<ABDataType> b_m_n_k(mnk);
    Tensor<CDataType> c_m_n_k(mnk);

    a_m.GenerateTensorValue(GeneratorTensor_3<ABDataType>{0.0, 1.0});
    b_m_n_k.GenerateTensorValue(GeneratorTensor_3<ABDataType>{0.0, 1.0});

    DeviceMem a_m_device_buf(sizeof(ABDataType) * a_m.mDesc.GetElementSpace());
    DeviceMem b_m_n_k_device_buf(sizeof(ABDataType) * b_m_n_k.mDesc.GetElementSpace());
    DeviceMem c_m_n_k_device_buf(sizeof(CDataType) * c_m_n_k.mDesc.GetElementSpace());

    a_m_device_buf.ToDevice(a_m.mData.data());
    b_m_n_k_device_buf.ToDevice(b_m_n_k.mData.data());

    auto broadcastAdd = DeviceElementwiseAddInstance{};
    auto argument     = broadcastAdd.MakeArgumentPointer(
        a_m_device_buf.GetDeviceBuffer(),
        b_m_n_k_device_buf.GetDeviceBuffer(),
        c_m_n_k_device_buf.GetDeviceBuffer(),
        std::vector<ck::index_t>{mnk.begin(), mnk.end()},
        {1, 0, 0}, // broadcast A on second and third dimension
        std::vector<ck::index_t>{b_m_n_k.mDesc.GetStrides().begin(),
                                 b_m_n_k.mDesc.GetStrides().end()},
        std::vector<ck::index_t>{c_m_n_k.mDesc.GetStrides().begin(),
                                 c_m_n_k.mDesc.GetStrides().end()},
        Add{});

    if(!broadcastAdd.IsSupportedArgument(argument.get()))
    {
        throw std::runtime_error("The runtime parameters seems not supported by the "
                                 "DeviceBinaryElementwise instance, exiting!");
    };

    auto broadcastAdd_invoker_ptr = broadcastAdd.MakeInvokerPointer();
    float ave_time =
        broadcastAdd_invoker_ptr->Run(argument.get(), StreamConfig{nullptr, time_kernel});

    std::cout << "Perf: " << ave_time << " ms" << std::endl;

    bool pass = true;
    if(do_verification)
    {
        c_m_n_k_device_buf.FromDevice(c_m_n_k.mData.data());
        Tensor<CDataType> host_c_m_n_k(mnk);

        host_broadcast3D_am_bmnk<Tensor<ABDataType>,
                                 Tensor<ABDataType>,
                                 Tensor<CDataType>,
                                 EltwiseComputeDataType,
                                 Add>(host_c_m_n_k, a_m, b_m_n_k, mnk, Add{});

        pass &= ck::utils::check_err(
            c_m_n_k.mData, host_c_m_n_k.mData, "Error: Incorrect results c", 1e-3, 1e-3);
    }

    return pass ? 0 : 1;
}

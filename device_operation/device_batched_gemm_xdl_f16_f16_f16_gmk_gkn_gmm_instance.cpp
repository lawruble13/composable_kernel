#include <stdlib.h>
#include "config.hpp"
#include "device_batched_gemm_xdl.hpp"
#include "element_wise_operation.hpp"
#include "device_operation_instance.hpp"

namespace ck {
namespace tensor_operation {
namespace device {
namespace device_batched_gemm_instance {

using F16 = ck::half_t;
using F32 = float;

using Row = ck::tensor_layout::gemm::RowMajor;
using Col = ck::tensor_layout::gemm::ColumnMajor;

template <ck::index_t... Is>
using S = ck::Sequence<Is...>;

using PassThrough = ck::tensor_operation::element_wise::PassThrough;

// Compilation parameters for a[m, k] * b[k, n] = c[m, n]
using device_batched_gemm_xdl_f16_f16_f16_gmk_gkn_gmn_instances =
    std::tuple<
        // clang-format off
        //##########| AData| BData| CData| AccData| ALayout| BLayout| CLayout|           A|           B|           C| Block|  MPer|  NPer| K0Per| K1| MPer| NPer| MXdl| NXdl|  ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockLds|  BBlockTransfer| BBlockTransfer| BBlockTransfer| BlockTransfer| BBlockTransfer| BBlockTransfer| BBlockLds| CThreadTransfer| CThreadTransfer|
        //##########|  Type|  Type|  Type|    Type|        |        |        | Elementwise| Elementwise| Elementwise|  Size| Block| Block| Block|   |  XDL|  XDL|  Per|  Per|   ThreadCluster|  ThreadCluster| SrcAccessOrder|   SrcVectorDim|      SrcScalar|      DstScalar| AddExtraM|   ThreadCluster|  ThreadCluster| SrcAccessOrder|  SrcVectorDim|      SrcScalar|      DstScalar| AddExtraN| SrcDstVectorDim|       DstScalar|
        //##########|      |      |      |        |        |        |        |   Operation|   Operation|   Operation|      |      |      |      |   |     |     | Wave| Wave| Lengths_K0_M_K1|   ArrangeOrder|               |               |      PerVector|   PerVector_K1|          | Lengths_K0_N_K1|   ArrangeOrder|               |              |      PerVector|   PerVector_K1|          |                |       PerVector|
        //##########|      |      |      |        |        |        |        |            |            |            |      |      |      |      |   |     |     |     |     |                |               |               |               |               |               |          |                |               |               |              |               |               |          |                |                |
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   256,   256,   128,     4,  8,   32,   32,    4,    2,     S<1, 4, 64, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 64, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              2,              8,      true,               8,               1>,
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   256,   128,   256,     4,  8,   32,   32,    2,    4,     S<1, 4, 64, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 64, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              4,              8,      true,               8,               1>,
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   128,   128,   128,     4,  8,   32,   32,    4,    2,     S<1, 4, 32, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 32, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              4,              8,      true,               8,               1>,
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   256,   128,   128,     4,  8,   32,   32,    2,    2,     S<1, 4, 64, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 64, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              2,              8,      true,               8,               1>,
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   128,   128,    64,     4,  8,   32,   32,    2,    2,     S<1, 4, 32, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 32, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              2,              8,      true,               8,               1>,
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   128,    64,   128,     4,  8,   32,   32,    2,    2,     S<1, 4, 32, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 32, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              4,              8,      true,               8,               1>,
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   256,   128,    64,     4,  8,   32,   32,    2,    1,     S<1, 4, 64, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 64, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              1,              8,      true,               8,               1>,
      //DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,   256,    64,   128,     4,  8,   32,   32,    1,    2,     S<1, 4, 64, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 4, 64, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              2,              8,      true,               8,               1>,
        DeviceBatchedGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row, PassThrough, PassThrough, PassThrough,    64,    16,    16,     4,  8,   16,   16,    1,    1,     S<1, 4, 16, 1>,     S<0, 2, 1, 3>,     S<0, 2, 1, 3>,              3,              8,              8,      true,     S<1, 1, 16, 1>,     S<0, 1, 3, 2>,     S<0, 1, 3, 2>,             2,              1,              1,      true,               8,               1>
        // clang-format on
        >;

void add_device_batched_gemm_xdl_f16_f16_f16_gmk_gkn_gmn_instances(
    std::vector<DeviceGemmPtr<PassThrough, PassThrough, PassThrough>>& instances)
{
    add_device_operation_instances(instances,
                                   device_batched_gemm_xdl_f16_f16_f16_gmk_gkn_gmn_instances{});
}

} // namespace device_batched_gemm_instance
} // namespace device
} // namespace tensor_operation
} // namespace ck

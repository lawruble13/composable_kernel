#include <stdlib.h>
#include "config.hpp"
#include "device_cgemm_4gemm_xdl_cshuffle.hpp"
#include "element_wise_operation.hpp"
#include "device_operation_instance.hpp"

namespace ck {
namespace tensor_operation {
namespace device {
namespace device_cgemm_instance {

using F16 = ck::half_t;
using F32 = float;

using Row = ck::tensor_layout::gemm::RowMajor;
using Col = ck::tensor_layout::gemm::ColumnMajor;

template <ck::index_t... Is>
using S = ck::Sequence<Is...>;

using PassThrough = ck::tensor_operation::element_wise::PassThrough;

static constexpr auto GemmDefault = ck::tensor_operation::device::GemmSpecialization::Default;

// Compilation parameters for a[k, m] * b[n, k] = c[m, n]
using device_cgemm_4gemm_xdl_c_shuffle_f16_f16_f16_km_nk_mn_instances = std::tuple<
    // clang-format off
        //#####################| ALayout| BLayout| CLayout| AData| BData| CData| AccData| CShuffle|           A|           B|           C|           GEMM| NumGemmK| Block|  MPer|  NPer|  KPer| AK1| BK1| MPer| NPer| MXdl| NXdl|  ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockLds|  BBlockTransfer| BBlockTransfer| BBlockTransfer| BlockTransfer| BBlockTransfer| BBlockTransfer| BBlockLds|    CShuffle|    CShuffle| CBlockTransferClusterLengths|  CBlockTransfer|
        //#####################|        |        |        |  Type|  Type|  Type|    Type| DataType| Elementwise| Elementwise| Elementwise| Spacialization| Prefetch|  Size| Block| Block| Block|    |    |  XDL|  XDL|  Per|  Per|   ThreadCluster|  ThreadCluster| SrcAccessOrder|   SrcVectorDim|      SrcScalar|      DstScalar| AddExtraM|   ThreadCluster|  ThreadCluster| SrcAccessOrder|  SrcVectorDim|      SrcScalar|      DstScalar| AddExtraN| MXdlPerWave| NXdlPerWave|         _MBlock_MWaveMPerXdl| ScalarPerVector|
        //#####################|        |        |        |      |      |      |        |         |   Operation|   Operation|   Operation|               |    Stage|      |      |      |      |    |    |     |     | Wave| Wave| Lengths_K0_M_K1|   ArrangeOrder|               |               |      PerVector|   PerVector_K1|          | Lengths_K0_N_K1|   ArrangeOrder|               |              |      PerVector|   PerVector_K1|          |  PerShuffle|  PerShuffle|         _NBlock_NWaveNPerXdl|   _NWaveNPerXdl|
        //#####################|        |        |        |      |      |      |        |         |            |            |            |               |         |      |      |      |      |    |    |     |     |     |     |                |               |               |               |               |               |          |                |               |               |              |               |               |          |            |            |                             |                |
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   256,   128,    32,   2,   8,   32,   32,    4,    2,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   256,   128,    32,   8,   8,   32,   32,    4,    2,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              8,         1,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   128,   256,    32,   2,   8,   32,   32,    2,    4,     S<8, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   128,   256,    32,   8,   8,   32,   32,    2,    4,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,         1,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   128,   128,   128,    32,   2,   8,   32,   32,    4,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 16, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   128,   128,   128,    32,   8,   8,   32,   32,    4,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              8,         1,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 16, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   128,   128,    32,   2,   8,   32,   32,    2,    2,     S<8, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   128,   128,    32,   8,   8,   32,   32,    2,    2,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,         1,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   128,   128,    64,    32,   2,   8,   32,   32,    2,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 4>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   128,   128,    64,    32,   8,   8,   32,   32,    2,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              8,         1,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 4>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   128,    64,   128,    32,   2,   8,   32,   32,    2,    2,     S<8, 16, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 16, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   128,    64,   128,    32,   8,   8,   32,   32,    2,    2,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,         1,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 16, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   128,    64,    32,   2,   8,   32,   32,    2,    1,     S<8, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,   128,    64,    32,   8,   8,   32,   32,    2,    1,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              2,              8,         1,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,    64,   128,    32,   2,   8,   32,   32,    1,    2,     S<16,16, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              4,              2,         0,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>,
        DeviceCGemm_4Gemm_Xdl_CShuffle<     Col,      Col,    Row,   F16,   F16,   F16,     F32,      F16, PassThrough, PassThrough, PassThrough,    GemmDefault,        1,   256,    64,   128,    32,   8,   8,   32,   32,    1,    2,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,              1,              1,              8,         1,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,             2,              8,              8,         1,           1,           1,               S<1, 32, 1, 8>,              8>
    // clang-format on
    >;

void add_device_cgemm_4gemm_xdl_c_shuffle_f16_f16_f16_km_nk_mn_instances(
    std::vector<DeviceCGemmPtr<PassThrough, PassThrough, PassThrough>>& instances)
{
    add_device_operation_instances(
        instances, device_cgemm_4gemm_xdl_c_shuffle_f16_f16_f16_km_nk_mn_instances{});
}

} // namespace device_cgemm_instance
} // namespace device
} // namespace tensor_operation
} // namespace ck
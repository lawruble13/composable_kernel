#ifndef CK_DRIVER_DYNAMIC_CONTRACTION_V1R2_HPP
#define CK_DRIVER_DYNAMIC_CONTRACTION_V1R2_HPP

#include "common_header.hpp"
#include "dynamic_tensor_descriptor.hpp"
#include "dynamic_tensor_descriptor_helper.hpp"
#include "gridwise_dynamic_contraction_v1r2.hpp"

namespace ck {

template <index_t BlockSize,
          typename FloatAB,
          typename FloatAcc,
          typename FloatC,
          InMemoryDataOperation CGlobalMemoryDataOperation,
          typename AGridDesc_GK0_GM0_GM1_GK1,
          typename BGridDesc_GK0_GN0_GN1_GK1,
          typename CGridDesc_GM0_GM1_GN0_GN1,
          index_t GM1PerBlockGM11,
          index_t GN1PerBlockGN11,
          index_t GK0PerBlock,
          index_t BM1PerThreadBM11,
          index_t BN1PerThreadBN11,
          index_t BK0PerThread,
          index_t BM10BN10ThreadClusterBM100,
          index_t BM10BN10ThreadClusterBN100,
          index_t BM10BN10ThreadClusterBM101,
          index_t BM10BN10ThreadClusterBN101,
          typename ABlockTransferThreadSliceLengths_GK0_GM0_GM10_GM11_GK1,
          typename ABlockTransferThreadClusterLengths_GK0_GM0_GM10_GM11_GK1,
          typename ABlockTransferThreadClusterArrangeOrder,
          typename ABlockTransferSrcAccessOrder,
          typename ABlockTransferSrcVectorTensorLengths_GK0_GM0_GM10_GM11_GK1,
          typename ABlockTransferDstVectorTensorLengths_GK0_GM0_GM10_GM11_GK1,
          typename ABlockTransferSrcVectorTensorContiguousDimOrder,
          typename BBlockTransferThreadSliceLengths_GK0_GN0_GN10_GN11_GK1,
          typename BBlockTransferThreadClusterLengths_GK0_GN0_GN10_GN11_GK1,
          typename BBlockTransferThreadClusterArrangeOrder,
          typename BBlockTransferSrcAccessOrder,
          typename BBlockTransferSrcVectorTensorLengths_GK0_GN0_GN10_GN11_GK1,
          typename BBlockTransferDstVectorTensorLengths_GK0_GN0_GN10_GN11_GK1,
          typename BBlockTransferSrcVectorTensorContiguousDimOrder,
          typename CThreadTransferSrcDstAccessOrder,
          index_t CThreadTransferSrcDstVectorDim,
          index_t CThreadTransferDstScalarPerVector,
          typename AGridIteratorHacks,
          typename BGridIteratorHacks,
          typename CGridIteratorHacks,
          typename AGridMoveSliceWindowIteratorHacks,
          typename BGridMoveSliceWindowIteratorHacks>
__host__ float
driver_dynamic_contraction_v1r2(const FloatAB* p_a_grid,
                                const FloatAB* p_b_grid,
                                FloatC* p_c_grid,
                                const AGridDesc_GK0_GM0_GM1_GK1& a_grid_desc_gk0_gm0_gm1_gk1,
                                const BGridDesc_GK0_GN0_GN1_GK1& b_grid_desc_gk0_gn0_gn1_gk1,
                                const CGridDesc_GM0_GM1_GN0_GN1& c_grid_desc_gm0_gm1_gn0_gn1,
                                AGridIteratorHacks,
                                BGridIteratorHacks,
                                CGridIteratorHacks,
                                AGridMoveSliceWindowIteratorHacks,
                                BGridMoveSliceWindowIteratorHacks,
                                index_t nrepeat)

{
    constexpr auto I0 = Number<0>{};
    constexpr auto I1 = Number<1>{};
    constexpr auto I2 = Number<2>{};
    constexpr auto I3 = Number<3>{};
    constexpr auto I4 = Number<4>{};
    constexpr auto I5 = Number<5>{};

    // GEMM
    using GridwiseContraction =
        GridwiseDynamicContraction_A_GK0_GM0_GM1_GK1_B_GK0_GN0_GN1_GK1_C_GM0_GM1_GN0_GN1<
            BlockSize,
            FloatAB,
            FloatAcc,
            FloatC,
            CGlobalMemoryDataOperation,
            AGridDesc_GK0_GM0_GM1_GK1,
            BGridDesc_GK0_GN0_GN1_GK1,
            CGridDesc_GM0_GM1_GN0_GN1,
            GM1PerBlockGM11,
            GN1PerBlockGN11,
            GK0PerBlock,
            BM1PerThreadBM11,
            BN1PerThreadBN11,
            BK0PerThread,
            BM10BN10ThreadClusterBM100,
            BM10BN10ThreadClusterBN100,
            BM10BN10ThreadClusterBM101,
            BM10BN10ThreadClusterBN101,
            ABlockTransferThreadSliceLengths_GK0_GM0_GM10_GM11_GK1,
            ABlockTransferThreadClusterLengths_GK0_GM0_GM10_GM11_GK1,
            ABlockTransferThreadClusterArrangeOrder,
            ABlockTransferSrcAccessOrder,
            ABlockTransferSrcVectorTensorLengths_GK0_GM0_GM10_GM11_GK1,
            ABlockTransferDstVectorTensorLengths_GK0_GM0_GM10_GM11_GK1,
            ABlockTransferSrcVectorTensorContiguousDimOrder,
            BBlockTransferThreadSliceLengths_GK0_GN0_GN10_GN11_GK1,
            BBlockTransferThreadClusterLengths_GK0_GN0_GN10_GN11_GK1,
            BBlockTransferThreadClusterArrangeOrder,
            BBlockTransferSrcAccessOrder,
            BBlockTransferSrcVectorTensorLengths_GK0_GN0_GN10_GN11_GK1,
            BBlockTransferDstVectorTensorLengths_GK0_GN0_GN10_GN11_GK1,
            BBlockTransferSrcVectorTensorContiguousDimOrder,
            CThreadTransferSrcDstAccessOrder,
            CThreadTransferSrcDstVectorDim,
            CThreadTransferDstScalarPerVector,
            AGridIteratorHacks,
            BGridIteratorHacks,
            CGridIteratorHacks,
            AGridMoveSliceWindowIteratorHacks,
            BGridMoveSliceWindowIteratorHacks>;

    const auto GK0 = a_grid_desc_gk0_gm0_gm1_gk1.GetLength(I0);

    if(!GridwiseContraction::CheckValidity(
           a_grid_desc_gk0_gm0_gm1_gk1, b_grid_desc_gk0_gn0_gn1_gk1, c_grid_desc_gm0_gm1_gn0_gn1))
    {
        throw std::runtime_error(
            "wrong! GridwiseDynamicContraction_km_kn0n1_mn0n1_v1r1 has invalid setting");
    }

    const auto a_grid_desc_gk0_gm0_gm10_gm11_gk1 =
        GridwiseContraction::MakeAGridDescriptor_GK0_GM0_GM10_GM11_GK1(a_grid_desc_gk0_gm0_gm1_gk1);
    const auto b_grid_desc_gk0_gn0_gn10_gn11_gk1 =
        GridwiseContraction::MakeBGridDescriptor_GK0_GN0_GN10_GN11_GK1(b_grid_desc_gk0_gn0_gn1_gk1);

    using AGridDesc_GK0_GM0_GM10_GM11_GK1 = decltype(a_grid_desc_gk0_gm0_gm10_gm11_gk1);
    using BGridDesc_GK0_GN0_GN10_GN11_GK1 = decltype(b_grid_desc_gk0_gn0_gn10_gn11_gk1);

    // c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1
    const auto c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1 =
        GridwiseContraction::MakeCGridDescriptor_GM10_BM0_BM1_GN10_BN0_BN1(
            c_grid_desc_gm0_gm1_gn0_gn1);

    using CGridDesc_GM10_BM0_BM1_GN10_BN0_BN1 = decltype(c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1);

    // c_grid_block_cluster_blockid_to_gm10_gn10
    const auto c_grid_block_cluster_blockid_to_gm10_gn10 =
        GridwiseContraction::MakeCGridBlockCluster_BlockId_To_GM10_GN10(
            c_grid_desc_gm0_gm1_gn0_gn1);

    using CGridBlockCluster_BlockId_To_GM10_GN10 =
        decltype(c_grid_block_cluster_blockid_to_gm10_gn10);

    const index_t grid_size = GridwiseContraction::CalculateGridSize(c_grid_desc_gm0_gm1_gn0_gn1);

    const bool has_main_k_block_loop = GridwiseContraction::CalculateHasMainKBlockLoop(GK0);

    const bool has_double_tail_k_block_loop =
        GridwiseContraction::CalculateHasDoubleTailKBlockLoop(GK0);

    {
        std::cout << "a_grid_desc_gk0_gm0_gm10_gm11_gk1{"
                  << a_grid_desc_gk0_gm0_gm10_gm11_gk1.GetLength(I0) << ", "
                  << a_grid_desc_gk0_gm0_gm10_gm11_gk1.GetLength(I1) << ", "
                  << a_grid_desc_gk0_gm0_gm10_gm11_gk1.GetLength(I2) << ", "
                  << a_grid_desc_gk0_gm0_gm10_gm11_gk1.GetLength(I3) << ", "
                  << a_grid_desc_gk0_gm0_gm10_gm11_gk1.GetLength(I4) << "}" << std::endl;

        std::cout << "b_grid_desc_gk0_gn0_gn10_gn11_gk1{"
                  << b_grid_desc_gk0_gn0_gn10_gn11_gk1.GetLength(I0) << ", "
                  << b_grid_desc_gk0_gn0_gn10_gn11_gk1.GetLength(I1) << ", "
                  << b_grid_desc_gk0_gn0_gn10_gn11_gk1.GetLength(I2) << ", "
                  << b_grid_desc_gk0_gn0_gn10_gn11_gk1.GetLength(I3) << ", "
                  << b_grid_desc_gk0_gn0_gn10_gn11_gk1.GetLength(I4) << "}" << std::endl;

        std::cout << "c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1{ "
                  << c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1.GetLength(I0) << ", "
                  << c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1.GetLength(I1) << ", "
                  << c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1.GetLength(I2) << ", "
                  << c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1.GetLength(I3) << ", "
                  << c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1.GetLength(I4) << ", "
                  << c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1.GetLength(I5) << "}" << std::endl;
    }

    float ave_time = 0;

    if(has_main_k_block_loop && has_double_tail_k_block_loop)
    {
        const auto kernel = kernel_dynamic_contraction_v1r1<
            GridwiseContraction,
            FloatAB,
            FloatC,
            remove_reference_t<AGridDesc_GK0_GM0_GM10_GM11_GK1>,
            remove_reference_t<BGridDesc_GK0_GN0_GN10_GN11_GK1>,
            remove_reference_t<CGridDesc_GM10_BM0_BM1_GN10_BN0_BN1>,
            remove_reference_t<CGridBlockCluster_BlockId_To_GM10_GN10>,
            true,
            true>;

        ave_time = launch_and_time_kernel(kernel,
                                          nrepeat,
                                          dim3(grid_size),
                                          dim3(BlockSize),
                                          0,
                                          0,
                                          p_a_grid,
                                          p_b_grid,
                                          p_c_grid,
                                          a_grid_desc_gk0_gm0_gm10_gm11_gk1,
                                          b_grid_desc_gk0_gn0_gn10_gn11_gk1,
                                          c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1,
                                          c_grid_block_cluster_blockid_to_gm10_gn10);
    }
    else if(has_main_k_block_loop && !has_double_tail_k_block_loop)
    {
        const auto kernel = kernel_dynamic_contraction_v1r1<
            GridwiseContraction,
            FloatAB,
            FloatC,
            remove_reference_t<AGridDesc_GK0_GM0_GM10_GM11_GK1>,
            remove_reference_t<BGridDesc_GK0_GN0_GN10_GN11_GK1>,
            remove_reference_t<CGridDesc_GM10_BM0_BM1_GN10_BN0_BN1>,
            remove_reference_t<CGridBlockCluster_BlockId_To_GM10_GN10>,
            true,
            false>;

        ave_time = launch_and_time_kernel(kernel,
                                          nrepeat,
                                          dim3(grid_size),
                                          dim3(BlockSize),
                                          0,
                                          0,
                                          p_a_grid,
                                          p_b_grid,
                                          p_c_grid,
                                          a_grid_desc_gk0_gm0_gm10_gm11_gk1,
                                          b_grid_desc_gk0_gn0_gn10_gn11_gk1,
                                          c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1,
                                          c_grid_block_cluster_blockid_to_gm10_gn10);
    }
    else if(!has_main_k_block_loop && has_double_tail_k_block_loop)
    {
        const auto kernel = kernel_dynamic_contraction_v1r1<
            GridwiseContraction,
            FloatAB,
            FloatC,
            remove_reference_t<AGridDesc_GK0_GM0_GM10_GM11_GK1>,
            remove_reference_t<BGridDesc_GK0_GN0_GN10_GN11_GK1>,
            remove_reference_t<CGridDesc_GM10_BM0_BM1_GN10_BN0_BN1>,
            remove_reference_t<CGridBlockCluster_BlockId_To_GM10_GN10>,
            false,
            true>;

        ave_time = launch_and_time_kernel(kernel,
                                          nrepeat,
                                          dim3(grid_size),
                                          dim3(BlockSize),
                                          0,
                                          0,
                                          p_a_grid,
                                          p_b_grid,
                                          p_c_grid,
                                          a_grid_desc_gk0_gm0_gm10_gm11_gk1,
                                          b_grid_desc_gk0_gn0_gn10_gn11_gk1,
                                          c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1,
                                          c_grid_block_cluster_blockid_to_gm10_gn10);
    }
    else
    {
        const auto kernel = kernel_dynamic_contraction_v1r1<
            GridwiseContraction,
            FloatAB,
            FloatC,
            remove_reference_t<AGridDesc_GK0_GM0_GM10_GM11_GK1>,
            remove_reference_t<BGridDesc_GK0_GN0_GN10_GN11_GK1>,
            remove_reference_t<CGridDesc_GM10_BM0_BM1_GN10_BN0_BN1>,
            remove_reference_t<CGridBlockCluster_BlockId_To_GM10_GN10>,
            false,
            false>;

        ave_time = launch_and_time_kernel(kernel,
                                          nrepeat,
                                          dim3(grid_size),
                                          dim3(BlockSize),
                                          0,
                                          0,
                                          p_a_grid,
                                          p_b_grid,
                                          p_c_grid,
                                          a_grid_desc_gk0_gm0_gm10_gm11_gk1,
                                          b_grid_desc_gk0_gn0_gn10_gn11_gk1,
                                          c_grid_desc_gm10_bm0_bm1_gn10_bn0_bn1,
                                          c_grid_block_cluster_blockid_to_gm10_gn10);
    }

    return ave_time;
}

} // namespace ck
#endif

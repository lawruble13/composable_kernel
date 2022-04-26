#ifndef CK_GRIDWISE_GEMM_XDLOPS_V2R4R2_HPP
#define CK_GRIDWISE_GEMM_XDLOPS_V2R4R2_HPP

#include "common_header.hpp"
#include "multi_index_transform_helper.hpp"
#include "merge_transform_for_wrw.hpp"
#include "tensor_descriptor.hpp"
#include "tensor_descriptor_helper.hpp"
#include "blockwise_gemm_xdlops.hpp"
#include "blockwise_tensor_slice_transfer_v4r1.hpp"
#include "blockwise_tensor_slice_transfer_v6r1.hpp"
#include "threadwise_tensor_slice_transfer.hpp"

#define A_BLOCK_BANK_CONFLICT_FREE_WRW 1
#define B_BLOCK_BANK_CONFLICT_FREE_WRW 1

namespace ck {

template <typename GridwiseGemm,
          typename FloatAB,
          typename FloatC,
          typename AGridDesc_B_K0_M_K1,
          typename BGridDesc_B_K0_N_K1,
          typename CGridDesc_MBlock_MPerBlock_NBlock_NPerBlock,
          typename AElementwiseOperation,
          typename BElementwiseOperation,
          typename CElementwiseOperation,
          typename CBlockClusterAdaptor,
          bool HasMainKBlockLoop>
__global__ void
#if CK_USE_LAUNCH_BOUNDS
    __launch_bounds__(CK_MAX_THREAD_PER_BLOCK, CK_MIN_BLOCK_PER_CU)
#endif
        kernel_gemm_xdlops_v2r4r2(const FloatAB* __restrict__ p_a_grid,
                                  const FloatAB* __restrict__ p_b_grid,
                                  FloatC* __restrict__ p_c_grid,
                                  const AGridDesc_B_K0_M_K1 a_b_k0_m_k1_grid_desc,
                                  const BGridDesc_B_K0_N_K1 b_b_k0_n_k1_grid_desc,
                                  const CGridDesc_MBlock_MPerBlock_NBlock_NPerBlock
                                      c_grid_desc_mblock_mperblock_nblock_nperblock,
                                  const AElementwiseOperation a_element_op,
                                  const BElementwiseOperation b_element_op,
                                  const CElementwiseOperation c_element_op,
                                  const CBlockClusterAdaptor c_block_cluster_adaptor)
{
#if(!defined(__HIP_DEVICE_COMPILE__) || defined(__gfx908__) || defined(__gfx90a__))
    constexpr index_t shared_block_size =
        GridwiseGemm::GetSharedMemoryNumberOfByte() / sizeof(FloatAB);

    __shared__ FloatAB p_shared_block[shared_block_size];

    GridwiseGemm::template Run<HasMainKBlockLoop>(p_a_grid,
                                                  p_b_grid,
                                                  p_c_grid,
                                                  p_shared_block,
                                                  a_b_k0_m_k1_grid_desc,
                                                  b_b_k0_n_k1_grid_desc,
                                                  c_grid_desc_mblock_mperblock_nblock_nperblock,
                                                  a_element_op,
                                                  b_element_op,
                                                  c_element_op,
                                                  c_block_cluster_adaptor);
#else
    ignore = p_a_grid;
    ignore = p_b_grid;
    ignore = p_c_grid;
    ignore = a_b_k0_m_k1_grid_desc;
    ignore = b_b_k0_n_k1_grid_desc;
    ignore = c_grid_desc_mblock_mperblock_nblock_nperblock;
    ignore = a_element_op;
    ignore = b_element_op;
    ignore = c_element_op;
    ignore = c_block_cluster_adaptor;
#endif // end of if (defined(__gfx908__) || defined(__gfx90a__))
}

template <index_t BlockSize,
          typename FloatAB,
          typename FloatAcc,
          typename FloatC,
          InMemoryDataOperationEnum CGlobalMemoryDataOperation,
          typename AGridDesc_B_K0_M_K1,
          typename BGridDesc_B_K0_N_K1,
          typename CMNGridDesc,
          typename AElementwiseOperation,
          typename BElementwiseOperation,
          typename CElementwiseOperation,
          index_t MPerBlock,
          index_t NPerBlock,
          index_t K0PerBlock,
          index_t MPerXDL,
          index_t NPerXDL,
          index_t K1Value,
          index_t MRepeat,
          index_t NRepeat,
          typename ABlockTransferThreadClusterLengths_K0_M_K1,
          typename ABlockTransferThreadClusterArrangeOrder,
          typename ABlockTransferSrcAccessOrder,
          index_t ABlockTransferSrcVectorDim,
          index_t ABlockTransferSrcScalarPerVector,
          index_t ABlockTransferDstScalarPerVector_K1,
          bool AThreadTransferSrcResetCoordinateAfterRun,
          bool ABlockLdsExtraM,
          typename BBlockTransferThreadClusterLengths_K0_N_K1,
          typename BBlockTransferThreadClusterArrangeOrder,
          typename BBlockTransferSrcAccessOrder,
          index_t BBlockTransferSrcVectorDim,
          index_t BBlockTransferSrcScalarPerVector,
          index_t BBlockTransferDstScalarPerVector_K1,
          bool BThreadTransferSrcResetCoordinateAfterRun,
          bool BBlockLdsExtraN,
          index_t CShuffleMRepeatPerShuffle,
          index_t CShuffleNRepeatPerShuffle,
          index_t CBlockTransferScalarPerVector_NWaveNPerXDL,
          typename CBlockTransferClusterLengths_MBlock_MPerBlock_NBlock_NPerBlock>
struct GridwiseGemm_bk0mk1_bk0nk1_mn_xdlops_v2r4r2
{
    static constexpr auto I0 = Number<0>{};
    static constexpr auto I1 = Number<1>{};
    static constexpr auto I2 = Number<2>{};
    static constexpr auto I3 = Number<3>{};
    static constexpr auto I4 = Number<4>{};
    static constexpr auto I5 = Number<5>{};
    static constexpr auto I6 = Number<6>{};
    static constexpr auto I7 = Number<7>{};

    // K1 should be Number<...>
    static constexpr auto K1 = Number<K1Value>{};

    // M1 & N1
    static constexpr auto ElePerBank = Number<64>{};
    static constexpr auto M1PerBlock = Number<ElePerBank / K1Value>{};
    static constexpr auto N1PerBlock = Number<ElePerBank / K1Value>{};
    // M0 & N0
    static constexpr auto M0PerBlock = Number<MPerBlock / M1PerBlock>{};
    static constexpr auto N0PerBlock = Number<NPerBlock / M1PerBlock>{};

    // M1 padding num
    static constexpr auto M1Padding = Number<4>{};
    static constexpr auto N1Padding = M1Padding;

    __host__ __device__ static constexpr auto GetABlockDescriptor_K0PerBlock_MPerBlock_K1()
    {
        constexpr auto max_lds_align = K1;

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto a_block_desc_k0_m_k1 = [&]() {
            if constexpr(ABlockLdsExtraM)
            {
#if A_BLOCK_BANK_CONFLICT_FREE_WRW
                constexpr auto a_block_desc_k0_m0_m1_k1 = make_naive_tensor_descriptor(
                    make_tuple(Number<K0PerBlock>{}, Number<M0PerBlock>{}, Number<M1PerBlock>{}, K1),
                    make_tuple(Number<M0PerBlock>{} * (Number<M1PerBlock>{} * K1 + M1Padding), Number<M1PerBlock>{} * K1 + M1Padding, K1, I1));

                constexpr auto a_block_desc_k0_m_k1_tmp = transform_tensor_descriptor(
                    a_block_desc_k0_m0_m1_k1,
                    make_tuple(make_pass_through_transform(Number<K0PerBlock>{}),
                               make_merge_transform_v3_division_mod(make_tuple(Number<M0PerBlock>{}, Number<M1PerBlock>{})),
                               make_pass_through_transform(K1)),
                    make_tuple(Sequence<0>{}, Sequence<1, 2>{}, Sequence<3>{}),
                    make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{})
                );

                return a_block_desc_k0_m_k1_tmp;
#else
                return make_naive_tensor_descriptor(
                    make_tuple(Number<K0PerBlock>{}, Number<MPerBlock>{}, K1),
                    make_tuple(Number<MPerBlock + 1>{} * K1, K1, I1));
#endif
            }
            else
            {
                return make_naive_tensor_descriptor_aligned(
                    make_tuple(Number<K0PerBlock>{}, Number<MPerBlock>{}, K1), max_lds_align);
            }
        }();

        return a_block_desc_k0_m_k1;
    }

    __host__ __device__ static constexpr auto GetABlockDescriptor_Batch_K0PerBlock_MPerBlock_K1()
    {
        constexpr auto max_lds_align = K1;

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto a_block_desc_b_k0_m_k1 = [&]() {
            if constexpr(ABlockLdsExtraM)
            {
#if A_BLOCK_BANK_CONFLICT_FREE_WRW
                constexpr auto a_block_desc_b_k0_m0_m1_k1 = make_naive_tensor_descriptor(
                    make_tuple(Number<1>{}, Number<K0PerBlock>{}, Number<M0PerBlock>{}, Number<M1PerBlock>{}, K1),
                    make_tuple(Number<K0PerBlock>{} * Number<M0PerBlock>{} * (Number<M1PerBlock>{} * K1 + M1Padding), Number<M0PerBlock>{} * (Number<M1PerBlock>{} * K1 + M1Padding), Number<M1PerBlock>{} * K1 + M1Padding, K1, I1));

                constexpr auto a_block_desc_b_k0_m_k1_tmp = transform_tensor_descriptor(
                    a_block_desc_b_k0_m0_m1_k1,
                    make_tuple(make_pass_through_transform(Number<1>{}),
                               make_pass_through_transform(Number<K0PerBlock>{}),
                               make_merge_transform_v3_division_mod_for_wrw(make_tuple(Number<M0PerBlock>{}, Number<M1PerBlock>{})),
                               make_pass_through_transform(K1)),
                    make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2, 3>{}, Sequence<4>{}),
                    make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}, Sequence<3>{})
                );

                return a_block_desc_b_k0_m_k1_tmp;
#else
                return make_naive_tensor_descriptor(
                    make_tuple(Number<1>{}, Number<K0PerBlock>{}, Number<MPerBlock>{}, K1),
                    make_tuple(Number<K0PerBlock>{} * Number<MPerBlock + 1>{} * K1, Number<MPerBlock + 1>{} * K1, K1, I1));
#endif
            }
            else
            {
                return make_naive_tensor_descriptor_aligned(
                    make_tuple(Number<1>{}, Number<K0PerBlock>{}, Number<MPerBlock>{}, K1), max_lds_align);
            }
        }();

        return a_block_desc_b_k0_m_k1;
    }

    __host__ __device__ static constexpr auto GetBBlockDescriptor_K0PerBlock_NPerBlock_K1()
    {
        constexpr auto max_lds_align = K1;

        // B matrix in LDS memory, dst of blockwise copy
        constexpr auto b_block_desc_k0_n_k1 = [&]() {
            if constexpr(BBlockLdsExtraN)
            {
#if B_BLOCK_BANK_CONFLICT_FREE_WRW
                constexpr auto b_block_desc_k0_n0_n1_k1 = make_naive_tensor_descriptor(
                    make_tuple(Number<K0PerBlock>{}, Number<N0PerBlock>{}, Number<N1PerBlock>{}, K1),
                    make_tuple(Number<N0PerBlock>{} * (Number<N1PerBlock>{} * K1 + N1Padding), Number<N1PerBlock>{} * K1 + N1Padding, K1, I1));

                constexpr auto b_block_desc_k0_n_k1_tmp = transform_tensor_descriptor(
                    b_block_desc_k0_n0_n1_k1,
                    make_tuple(make_pass_through_transform(Number<K0PerBlock>{}),
                               make_merge_transform_v3_division_mod(make_tuple(Number<N0PerBlock>{}, Number<N1PerBlock>{})),
                               make_pass_through_transform(K1)),
                    make_tuple(Sequence<0>{}, Sequence<1, 2>{}, Sequence<3>{}),
                    make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{})
                );

                return b_block_desc_k0_n_k1_tmp;
#else

                return make_naive_tensor_descriptor(
                    make_tuple(Number<K0PerBlock>{}, Number<NPerBlock>{}, K1),
                    make_tuple(Number<NPerBlock + 1>{} * K1, K1, I1));
#endif
            }
            else
            {
                return make_naive_tensor_descriptor_aligned(
                    make_tuple(Number<K0PerBlock>{}, Number<NPerBlock>{}, K1), max_lds_align);
            }
        }();

        return b_block_desc_k0_n_k1;
    }

    __host__ __device__ static constexpr auto GetBBlockDescriptor_Batch_K0PerBlock_NPerBlock_K1()
    {
        constexpr auto max_lds_align = K1;

        // B matrix in LDS memory, dst of blockwise copy
        constexpr auto b_block_desc_b_k0_n_k1 = [&]() {
            if constexpr(BBlockLdsExtraN)
            {
#if B_BLOCK_BANK_CONFLICT_FREE_WRW
                constexpr auto b_block_desc_b_k0_n0_n1_k1 = make_naive_tensor_descriptor(
                    make_tuple(Number<1>{}, Number<K0PerBlock>{}, Number<N0PerBlock>{}, Number<N1PerBlock>{}, K1),
                    make_tuple(Number<K0PerBlock>{} * Number<N0PerBlock>{} * (Number<N1PerBlock>{} * K1 + N1Padding), Number<N0PerBlock>{} * (Number<N1PerBlock>{} * K1 + N1Padding), Number<N1PerBlock>{} * K1 + N1Padding, K1, I1));

                constexpr auto b_block_desc_b_k0_n_k1_tmp = transform_tensor_descriptor(
                    b_block_desc_b_k0_n0_n1_k1,
                    make_tuple(make_pass_through_transform(Number<1>{}),
                               make_pass_through_transform(Number<K0PerBlock>{}),
                               make_merge_transform_v3_division_mod_for_wrw(make_tuple(Number<N0PerBlock>{}, Number<N1PerBlock>{})),
                               make_pass_through_transform(K1)),
                    make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2, 3>{}, Sequence<4>{}),
                    make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}, Sequence<3>{})
                );

                return b_block_desc_b_k0_n_k1_tmp;
#else
                return make_naive_tensor_descriptor(
                    make_tuple(Number<1>{}, Number<K0PerBlock>{}, Number<NPerBlock>{}, K1),
                    make_tuple(Number<K0PerBlock>{} * Number<NPerBlock + 1>{} * K1, Number<NPerBlock + 1>{} * K1, K1, I1));
#endif
            }
            else
            {
                return make_naive_tensor_descriptor_aligned(
                    make_tuple(Number<1>{}, Number<K0PerBlock>{}, Number<NPerBlock>{}, K1), max_lds_align);
            }
        }();

        return b_block_desc_b_k0_n_k1;
    }

    __host__ __device__ static constexpr index_t GetSharedMemoryNumberOfByte()
    {
        constexpr auto max_lds_align = K1;

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto a_b_k0_m_k1_block_desc = GetABlockDescriptor_Batch_K0PerBlock_MPerBlock_K1();

        // B matrix in LDS memory, dst of blockwise copy
        constexpr auto b_b_k0_n_k1_block_desc = GetBBlockDescriptor_Batch_K0PerBlock_NPerBlock_K1();

        // LDS allocation for A and B: be careful of alignment
        constexpr auto a_block_space_size =
            math::integer_least_multiple(a_b_k0_m_k1_block_desc.GetElementSpaceSize(), max_lds_align);

        constexpr auto b_block_space_size =
            math::integer_least_multiple(b_b_k0_n_k1_block_desc.GetElementSpaceSize(), max_lds_align);

        constexpr auto c_block_size =
            GetCBlockDescriptor_MBlock_MPerBlock_NBlock_NPerBlock().GetElementSpaceSize();

        return math::max((a_block_space_size + b_block_space_size) * sizeof(FloatAB),
                         c_block_size * sizeof(FloatC));
    }

    // block_id to matrix tile idx (m0, n0) mapping are controlled by {M01, N01}
    __host__ __device__ static constexpr bool
    CheckValidity(const AGridDesc_B_K0_M_K1& a_b_k0_m_k1_grid_desc,
                  const BGridDesc_B_K0_N_K1& b_b_k0_n_k1_grid_desc,
                  const CMNGridDesc& c_m_n_grid_desc,
                  index_t M01,
                  index_t N01)
    {
        static_assert(is_known_at_compile_time<remove_cv_t<decltype(K1)>>::value,
                      "wrong! K1 need to be known at compile-time");

        static_assert((MPerBlock % (MPerXDL * MRepeat) == 0) &&
                          (NPerBlock % (NRepeat * NPerXDL)) == 0,
                      "Invalid tuning param!");

        const auto M      = a_b_k0_m_k1_grid_desc.GetLength(I2);
        const auto N      = b_b_k0_n_k1_grid_desc.GetLength(I2);
        const auto K0     = a_b_k0_m_k1_grid_desc.GetLength(I1);
        const auto KBatch = a_b_k0_m_k1_grid_desc.GetLength(I0);

        if(!(M == c_m_n_grid_desc.GetLength(I0) && N == c_m_n_grid_desc.GetLength(I1) &&
             K0 == b_b_k0_n_k1_grid_desc.GetLength(I1) &&
             K1 == a_b_k0_m_k1_grid_desc.GetLength(I3) &&
             K1 == b_b_k0_n_k1_grid_desc.GetLength(I3) &&
             KBatch == b_b_k0_n_k1_grid_desc.GetLength(I0)))
            return false;

        if(!(M % MPerBlock == 0 && N % NPerBlock == 0 && K0 % K0PerBlock == 0))
            return false;

        // check M01, N01
        constexpr auto M1 = Number<MPerBlock>{};
        constexpr auto N1 = Number<NPerBlock>{};

        const auto M0 = M / M1;
        const auto N0 = N / N1;

        if(!(M0 % M01 == 0 && N0 % N01 == 0))
            return false;

        // TODO: also check validity of all components (blockwise-copy, threadwise-copy, etc)
        return true;
    }

    __host__ __device__ static constexpr index_t
    CalculateGridSize(const CMNGridDesc& c_m_n_grid_desc, index_t KBatch)
    {
        const auto M = c_m_n_grid_desc.GetLength(I0);
        const auto N = c_m_n_grid_desc.GetLength(I1);

        const index_t grid_size = (M / MPerBlock) * (N / NPerBlock) * KBatch;

        return grid_size;
    }

    __host__ __device__ static constexpr bool CalculateHasMainK0BlockLoop(index_t K0)
    {
        const bool has_main_k0_block_loop = K0 > K0PerBlock;

        return has_main_k0_block_loop;
    }

    __host__ __device__ static constexpr auto
    MakeCGridDesc_MBlock_MPerBlock_NBlock_NPerBlock(const CMNGridDesc& c_m_n_grid_desc)
    {
        const auto M = c_m_n_grid_desc.GetLength(I0);
        const auto N = c_m_n_grid_desc.GetLength(I1);

        const auto MBlock = M / MPerBlock;
        const auto NBlock = N / NPerBlock;

        return transform_tensor_descriptor(
            c_m_n_grid_desc,
            make_tuple(make_unmerge_transform(make_tuple(MBlock, Number<MPerBlock>{})),
                       make_unmerge_transform(make_tuple(NBlock, Number<NPerBlock>{}))),
            make_tuple(Sequence<0>{}, Sequence<1>{}),
            make_tuple(Sequence<0, 1>{}, Sequence<2, 3>{}));
    }

    // return block_id to C matrix tile idx (m0, n0) mapping
    __host__ __device__ static constexpr auto MakeCBlockClusterAdaptor(
        const CMNGridDesc& c_m_n_grid_desc, index_t M01, index_t N01, index_t KBatch)
    {
        const auto M = c_m_n_grid_desc.GetLength(I0);
        const auto N = c_m_n_grid_desc.GetLength(I1);

        constexpr auto M1 = Number<MPerBlock>{};
        constexpr auto N1 = Number<NPerBlock>{};

        const auto M0 = M / M1;
        const auto N0 = N / N1;

        const auto M00 = M0 / M01;
        const auto N00 = N0 / N01;

        const auto kbatch_m00_m01_n00_n01_to_m0_n0_block_cluster_adaptor =
            make_single_stage_tensor_adaptor(
                make_tuple(make_pass_through_transform(KBatch),
                           make_unmerge_transform(make_tuple(M00, M01)),
                           make_unmerge_transform(make_tuple(N00, N01))),
                make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}),
                make_tuple(Sequence<0>{}, Sequence<1, 3>{}, Sequence<2, 4>{}));

        const auto c_blockid_to_kbatch_m00_m01_n00_n01_block_cluster_adaptor =
            make_single_stage_tensor_adaptor(
                make_tuple(make_merge_transform(make_tuple(KBatch, M00, N00, M01, N01))),
                make_tuple(Sequence<0, 1, 2, 3, 4>{}),
                make_tuple(Sequence<0>{}));

        const auto c_blockid_to_kbatch_m0_n0_block_cluster_adaptor =
            chain_tensor_adaptors(kbatch_m00_m01_n00_n01_to_m0_n0_block_cluster_adaptor,
                                  c_blockid_to_kbatch_m00_m01_n00_n01_block_cluster_adaptor);

        return c_blockid_to_kbatch_m0_n0_block_cluster_adaptor;
    }

    __host__ __device__ static constexpr auto
    GetCBlockDescriptor_MBlock_MPerBlock_NBlock_NPerBlock()
    {
        constexpr index_t MWave = MPerBlock / (MRepeat * MPerXDL);
        constexpr index_t NWave = NPerBlock / (NRepeat * NPerXDL);

        return make_naive_tensor_descriptor_packed(
            make_tuple(I1,
                       Number<CShuffleMRepeatPerShuffle * MWave * MPerXDL>{},
                       I1,
                       Number<CShuffleNRepeatPerShuffle * NWave * NPerXDL>{}));
    }

    using CGridDesc_MBlock_MPerBlock_NBlock_NPerBlock =
        decltype(MakeCGridDesc_MBlock_MPerBlock_NBlock_NPerBlock(CMNGridDesc{}));
    using CBlockClusterAdaptor = decltype(MakeCBlockClusterAdaptor(CMNGridDesc{}, 1, 1, 1));

    template <bool HasMainKBlockLoop>
    __device__ static void Run(const FloatAB* __restrict__ p_a_grid,
                               const FloatAB* __restrict__ p_b_grid,
                               FloatC* __restrict__ p_c_grid,
                               FloatAB* __restrict__ p_shared_block,
                               const AGridDesc_B_K0_M_K1& a_b_k0_m_k1_grid_desc,
                               const BGridDesc_B_K0_N_K1& b_b_k0_n_k1_grid_desc,
                               const CGridDesc_MBlock_MPerBlock_NBlock_NPerBlock&
                                   c_grid_desc_mblock_mperblock_nblock_nperblock,
                               const AElementwiseOperation& a_element_op,
                               const BElementwiseOperation& b_element_op,
                               const CElementwiseOperation& c_element_op,
                               const CBlockClusterAdaptor& c_block_cluster_adaptor)
    {
        const auto a_grid_buf = make_dynamic_buffer<AddressSpaceEnum::Global>(
            p_a_grid, a_b_k0_m_k1_grid_desc.GetElementSpaceSize());
        const auto b_grid_buf = make_dynamic_buffer<AddressSpaceEnum::Global>(
            p_b_grid, b_b_k0_n_k1_grid_desc.GetElementSpaceSize());
        auto c_grid_buf = make_dynamic_buffer<AddressSpaceEnum::Global>(
            p_c_grid, c_grid_desc_mblock_mperblock_nblock_nperblock.GetElementSpaceSize());

        const auto K0 = a_b_k0_m_k1_grid_desc.GetLength(I1);

        // divide block work by [M, N]
        const auto block_work_idx =
            c_block_cluster_adaptor.CalculateBottomIndex(make_multi_index(get_block_1d_id()));

        const index_t k_batch_id = block_work_idx[I0];

        // HACK: this force m/n_block_data_idx_on_grid into SGPR
        const index_t m_block_data_idx_on_grid =
            __builtin_amdgcn_readfirstlane(block_work_idx[I1] * MPerBlock);

        const index_t n_block_data_idx_on_grid =
            __builtin_amdgcn_readfirstlane(block_work_idx[I2] * NPerBlock);

        // lds max alignment
        constexpr auto max_lds_align = K1;

        // A matrix in LDS memory, dst of blockwise copy
        constexpr auto a_k0_m_k1_block_desc = GetABlockDescriptor_K0PerBlock_MPerBlock_K1();

        constexpr auto a_b_k0_m_k1_block_desc = GetABlockDescriptor_Batch_K0PerBlock_MPerBlock_K1();
        // B matrix in LDS memory, dst of blockwise copy
        constexpr auto b_k0_n_k1_block_desc = GetBBlockDescriptor_K0PerBlock_NPerBlock_K1();

        constexpr auto b_b_k0_n_k1_block_desc = GetBBlockDescriptor_Batch_K0PerBlock_NPerBlock_K1();
        // A matrix blockwise copy
        auto a_blockwise_copy =
            BlockwiseTensorSliceTransfer_v4r1<BlockSize,
                                              AElementwiseOperation,
                                              ck::tensor_operation::element_wise::PassThrough,
                                              InMemoryDataOperationEnum::Set,
                                              Sequence<1, K0PerBlock, MPerBlock, K1>,
                                              ABlockTransferThreadClusterLengths_K0_M_K1,
                                              ABlockTransferThreadClusterArrangeOrder,
                                              FloatAB,
                                              FloatAB,
                                              decltype(a_b_k0_m_k1_grid_desc),
                                              decltype(a_b_k0_m_k1_block_desc),
                                              ABlockTransferSrcAccessOrder,
                                              Sequence<0, 2, 1, 3>,
                                              ABlockTransferSrcVectorDim,
                                              3,
                                              ABlockTransferSrcScalarPerVector,
                                              ABlockTransferDstScalarPerVector_K1,
                                              1,
                                              1,
                                              AThreadTransferSrcResetCoordinateAfterRun,
                                              true>(
                a_b_k0_m_k1_grid_desc,
                make_multi_index(k_batch_id, 0, m_block_data_idx_on_grid, 0),
                a_element_op,
                a_b_k0_m_k1_block_desc,
                make_multi_index(0, 0, 0, 0),
                ck::tensor_operation::element_wise::PassThrough{});

        // B matrix blockwise copy
        auto b_blockwise_copy =
            BlockwiseTensorSliceTransfer_v4r1<BlockSize,
                                              BElementwiseOperation,
                                              ck::tensor_operation::element_wise::PassThrough,
                                              InMemoryDataOperationEnum::Set,
                                              Sequence<1, K0PerBlock, NPerBlock, K1>,
                                              BBlockTransferThreadClusterLengths_K0_N_K1,
                                              BBlockTransferThreadClusterArrangeOrder,
                                              FloatAB,
                                              FloatAB,
                                              decltype(b_b_k0_n_k1_grid_desc),
                                              decltype(b_b_k0_n_k1_block_desc),
                                              BBlockTransferSrcAccessOrder,
                                              Sequence<0, 2, 1, 3>,
                                              BBlockTransferSrcVectorDim,
                                              3,
                                              BBlockTransferSrcScalarPerVector,
                                              BBlockTransferDstScalarPerVector_K1,
                                              1,
                                              1,
                                              BThreadTransferSrcResetCoordinateAfterRun,
                                              true>(
                b_b_k0_n_k1_grid_desc,
                make_multi_index(k_batch_id, 0, n_block_data_idx_on_grid, 0),
                b_element_op,
                b_b_k0_n_k1_block_desc,
                make_multi_index(0, 0, 0, 0),
                ck::tensor_operation::element_wise::PassThrough{});

        // GEMM definition
        //   c_mtx += transpose(a_mtx) * b_mtx
        //     a_mtx[K0PerBlock, MPerBlock] is in LDS
        //     b_mtx[K0PerBlock, NPerBlock] is in LDS
        //     c_mtx[MPerBlock, NPerBlock] is distributed among threads, and saved in
        //       register
        // sanity check

        constexpr index_t KPack = math::max(
            K1, MfmaSelector<FloatAB, MPerXDL, NPerXDL>::selected_mfma.k_per_blk);

        auto blockwise_gemm =
            BlockwiseGemmXdlops_k0mk1_k0nk1_m0n0m1n1m2m3m4n2_v1<BlockSize,
                                                                FloatAB,
                                                                FloatAcc,
                                                                decltype(a_k0_m_k1_block_desc),
                                                                decltype(b_k0_n_k1_block_desc),
                                                                MPerXDL,
                                                                NPerXDL,
                                                                MRepeat,
                                                                NRepeat,
                                                                KPack>{};

        auto c_thread_buf = blockwise_gemm.GetCThreadBuffer();

        // LDS allocation for A and B: be careful of alignment
        constexpr auto a_block_space_size =
            math::integer_least_multiple(a_k0_m_k1_block_desc.GetElementSpaceSize(), max_lds_align);

        FloatAB* p_a_block = p_shared_block;
        FloatAB* p_b_block = p_shared_block + a_block_space_size;

        constexpr auto a_block_slice_copy_step = make_multi_index(0, K0PerBlock, 0, 0);
        constexpr auto b_block_slice_copy_step = make_multi_index(0, K0PerBlock, 0, 0);

        auto a_block_buf = make_dynamic_buffer<AddressSpaceEnum::Lds>(
            p_a_block, a_k0_m_k1_block_desc.GetElementSpaceSize());
        auto b_block_buf = make_dynamic_buffer<AddressSpaceEnum::Lds>(
            p_b_block, b_k0_n_k1_block_desc.GetElementSpaceSize());

        // preload data into LDS
        {
            a_blockwise_copy.RunRead(a_b_k0_m_k1_grid_desc, a_grid_buf);
            b_blockwise_copy.RunRead(b_b_k0_n_k1_grid_desc, b_grid_buf);

            a_blockwise_copy.RunWrite(a_b_k0_m_k1_block_desc, a_block_buf);
            b_blockwise_copy.RunWrite(b_b_k0_n_k1_block_desc, b_block_buf);
        }

        // Initialize C
        c_thread_buf.Clear();

        // main body
        if constexpr(HasMainKBlockLoop)
        {
            index_t k0_block_data_begin = 0;

            do
            {
                a_blockwise_copy.MoveSrcSliceWindow(a_b_k0_m_k1_grid_desc, a_block_slice_copy_step);
                b_blockwise_copy.MoveSrcSliceWindow(b_b_k0_n_k1_grid_desc, b_block_slice_copy_step);

                a_blockwise_copy.RunRead(a_b_k0_m_k1_grid_desc, a_grid_buf);

                block_sync_lds();

                b_blockwise_copy.RunRead(b_b_k0_n_k1_grid_desc, b_grid_buf);

                blockwise_gemm.Run(a_block_buf, b_block_buf, c_thread_buf);

                block_sync_lds();

                a_blockwise_copy.RunWrite(a_b_k0_m_k1_block_desc, a_block_buf);
                b_blockwise_copy.RunWrite(b_b_k0_n_k1_block_desc, b_block_buf);

                k0_block_data_begin += K0PerBlock;
            } while(k0_block_data_begin < (K0 - K0PerBlock));
        }

        // tail
        {
            block_sync_lds();

            blockwise_gemm.Run(a_block_buf, b_block_buf, c_thread_buf);
        }

        // output: register to global memory
        {
            constexpr index_t MWave = MPerBlock / (MRepeat * MPerXDL);
            constexpr index_t NWave = NPerBlock / (NRepeat * NPerXDL);

            constexpr auto c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc =
                blockwise_gemm.GetCBlockDescriptor_M0_N0_M1_N1_M2_M3_M4_N2();

            constexpr auto c_m0_n0_m1_n1_m2_m3_m4_n2_thread_desc =
                blockwise_gemm.GetCThreadDescriptor_M0_N0_M1_N1_M2_M3_M4_N2();

            constexpr auto M0 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I0);
            constexpr auto N0 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I1);
            constexpr auto M1 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I2);
            constexpr auto N1 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I3);
            constexpr auto M2 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I4);
            constexpr auto M3 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I5);
            constexpr auto M4 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I6);
            constexpr auto N2 = c_m0_n0_m1_n1_m2_m3_m4_n2_block_desc.GetLength(I7);

            constexpr auto c_block_desc_mblock_mperblock_nblock_nperblock =
                GetCBlockDescriptor_MBlock_MPerBlock_NBlock_NPerBlock();

            auto c_block_buf = make_dynamic_buffer<AddressSpaceEnum::Lds>(
                static_cast<FloatC*>(p_shared_block),
                c_block_desc_mblock_mperblock_nblock_nperblock.GetElementSpaceSize());

            static_assert(M1 == MWave, "");
            static_assert(N1 == NWave, "");
            static_assert(M2 * M3 * M4 == MPerXDL, "");
            static_assert(N2 == NPerXDL, "");

            constexpr auto c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2 = transform_tensor_descriptor(
                c_block_desc_mblock_mperblock_nblock_nperblock,
                make_tuple(
                    make_freeze_transform(I0), // freeze mblock
                    make_unmerge_transform(make_tuple(CShuffleMRepeatPerShuffle,
                                                      M1,
                                                      M2,
                                                      M3,
                                                      M4)), // M1 = MWave, M2 * M3 * M4 = MPerXDL
                    make_freeze_transform(I0),              // freeze nblock
                    make_unmerge_transform(make_tuple(CShuffleNRepeatPerShuffle,
                                                      N1,
                                                      N2))), // M1 = MWave, M2 * M3 * M4 = MPerXDL
                make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}, Sequence<3>{}),
                make_tuple(
                    Sequence<>{}, Sequence<0, 2, 4, 5, 6>{}, Sequence<>{}, Sequence<1, 3, 7>{}));

            // calculate origin of thread output tensor on global memory
            //     blockwise GEMM c matrix starting index
            const auto c_thread_mtx_on_block =
                blockwise_gemm.CalculateCThreadOriginDataIndex(I0, I0, I0, I0);

            const index_t m_thread_data_on_block = c_thread_mtx_on_block[I0];
            const index_t n_thread_data_on_block = c_thread_mtx_on_block[I1];

            const auto m_thread_data_on_block_to_m0_m1_m2_m3_m4_adaptor =
                make_single_stage_tensor_adaptor(
                    make_tuple(make_merge_transform(make_tuple(M0, M1, M2, M3, M4))),
                    make_tuple(Sequence<0, 1, 2, 3, 4>{}),
                    make_tuple(Sequence<0>{}));

            const auto m_thread_data_on_block_idx =
                m_thread_data_on_block_to_m0_m1_m2_m3_m4_adaptor.CalculateBottomIndex(
                    make_multi_index(m_thread_data_on_block));

            const auto n_thread_data_on_block_to_n0_n1_n2_adaptor =
                make_single_stage_tensor_adaptor(
                    make_tuple(make_merge_transform(make_tuple(N0, N1, N2))),
                    make_tuple(Sequence<0, 1, 2>{}),
                    make_tuple(Sequence<0>{}));

            const auto n_thread_data_on_block_idx =
                n_thread_data_on_block_to_n0_n1_n2_adaptor.CalculateBottomIndex(
                    make_multi_index(n_thread_data_on_block));

            // VGPR to LDS
            auto c_thread_copy_vgpr_to_lds =
                ThreadwiseTensorSliceTransfer_v1r3<FloatAcc,
                                                   FloatC,
                                                   decltype(c_m0_n0_m1_n1_m2_m3_m4_n2_thread_desc),
                                                   decltype(c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2),
                                                   ck::tensor_operation::element_wise::PassThrough,
                                                   Sequence<CShuffleMRepeatPerShuffle,
                                                            CShuffleNRepeatPerShuffle,
                                                            I1,
                                                            I1,
                                                            M2,
                                                            I1,
                                                            M4,
                                                            I1>,
                                                   Sequence<0, 1, 2, 3, 4, 5, 6, 7>,
                                                   7,
                                                   1,
                                                   InMemoryDataOperationEnum::Set,
                                                   1,
                                                   true>{
                    c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2,
                    make_multi_index(0,
                                     0,
                                     m_thread_data_on_block_idx[I1],
                                     n_thread_data_on_block_idx[I1],
                                     m_thread_data_on_block_idx[I2],
                                     m_thread_data_on_block_idx[I3],
                                     m_thread_data_on_block_idx[I4],
                                     n_thread_data_on_block_idx[I2]),
                    ck::tensor_operation::element_wise::PassThrough{}};

            // LDS to global
            auto c_block_copy_lds_to_global = BlockwiseTensorSliceTransfer_v6r1<
                BlockSize,                  // index_t BlockSize,
                CElementwiseOperation,      // ElementwiseOperation,
                CGlobalMemoryDataOperation, // DstInMemOp,
                Sequence<1,
                         CShuffleMRepeatPerShuffle * MWave * MPerXDL,
                         1,
                         CShuffleNRepeatPerShuffle * NWave * NPerXDL>, // BlockSliceLengths,
                CBlockTransferClusterLengths_MBlock_MPerBlock_NBlock_NPerBlock,
                Sequence<0, 1, 2, 3>, // typename ThreadClusterArrangeOrder,
                FloatC,               // typename SrcData,
                FloatC,               // typename DstData,
                decltype(c_block_desc_mblock_mperblock_nblock_nperblock),
                decltype(c_grid_desc_mblock_mperblock_nblock_nperblock),
                Sequence<0, 1, 2, 3>,                       // typename DimAccessOrder,
                3,                                          // index_t VectorDim,
                CBlockTransferScalarPerVector_NWaveNPerXDL, // index_t ScalarPerVector,
                true,  // bool ThreadTransferSrcResetCoordinateAfterRun,
                false> // bool ThreadTransferDstResetCoordinateAfterRun
                {c_block_desc_mblock_mperblock_nblock_nperblock,
                 make_multi_index(0, 0, 0, 0),
                 c_grid_desc_mblock_mperblock_nblock_nperblock,
                 make_multi_index(block_work_idx[I1], 0, block_work_idx[I2], 0),
                 c_element_op};

            constexpr auto mxdlperwave_forward_step =
                make_multi_index(0, CShuffleMRepeatPerShuffle * MWave * MPerXDL, 0, 0);
            constexpr auto nxdlperwave_forward_step =
                make_multi_index(0, 0, 0, CShuffleNRepeatPerShuffle * NWave * NPerXDL);
            constexpr auto nxdlperwave_backward_step =
                make_multi_index(0, 0, 0, -CShuffleNRepeatPerShuffle * NWave * NPerXDL);

            static_for<0, MRepeat, CShuffleMRepeatPerShuffle>{}([&](auto mxdlperwave_iter) {
                constexpr auto mxdlperwave = mxdlperwave_iter;

                static_for<0, NRepeat, CShuffleNRepeatPerShuffle>{}([&](auto nxdlperwave_iter) {
                    constexpr bool nxdlperwave_forward_sweep =
                        (mxdlperwave % (2 * CShuffleMRepeatPerShuffle) == 0);

                    constexpr index_t nxdlperwave_value =
                        nxdlperwave_forward_sweep
                            ? nxdlperwave_iter
                            : (NRepeat - nxdlperwave_iter - CShuffleNRepeatPerShuffle);

                    constexpr auto nxdlperwave = Number<nxdlperwave_value>{};

                    // make sure it's safe to do ds_write
                    block_sync_lds();

                    // VGPR to LDS
                    c_thread_copy_vgpr_to_lds.Run(
                        c_m0_n0_m1_n1_m2_m3_m4_n2_thread_desc,
                        make_tuple(mxdlperwave, nxdlperwave, I0, I0, I0, I0, I0, I0),
                        c_thread_buf,
                        c_block_desc_m0_n0_m1_n1_m2_m3_m4_n2,
                        c_block_buf);

                    // make sure it's safe to do ds_read
                    block_sync_lds();

                    // LDS to global
                    c_block_copy_lds_to_global.Run(c_block_desc_mblock_mperblock_nblock_nperblock,
                                                   c_block_buf,
                                                   c_grid_desc_mblock_mperblock_nblock_nperblock,
                                                   c_grid_buf);

                    // move on nxdlperwave dimension
                    if constexpr(nxdlperwave_forward_sweep &&
                                 (nxdlperwave < NRepeat - CShuffleNRepeatPerShuffle))
                    {
                        c_block_copy_lds_to_global.MoveDstSliceWindow(
                            c_grid_desc_mblock_mperblock_nblock_nperblock,
                            nxdlperwave_forward_step);
                    }
                    else if constexpr((!nxdlperwave_forward_sweep) && (nxdlperwave > 0))
                    {
                        c_block_copy_lds_to_global.MoveDstSliceWindow(
                            c_grid_desc_mblock_mperblock_nblock_nperblock,
                            nxdlperwave_backward_step);
                    }
                });

                // move on mxdlperwave dimension
                if constexpr(mxdlperwave < MRepeat - CShuffleMRepeatPerShuffle)
                {
                    c_block_copy_lds_to_global.MoveDstSliceWindow(
                        c_grid_desc_mblock_mperblock_nblock_nperblock, mxdlperwave_forward_step);
                }
            });
        }
    }
}; // namespace ck

} // namespace ck
#endif

#pragma once

#include "common_header.hpp"
#include "tensor_descriptor.hpp"
#include "tensor_descriptor_helper.hpp"
#include "tensor_space_filling_curve.hpp"

namespace ck {

// Do following things to avoid "alloca" in LLVM-IR, which would cause scratch memory
// and sometimes useless instructions:
//   1. Don't save a reference to tensor descriptor in class, pass in tensor descriptor as argument
//   instead
//   2. Don't construct a new tensor coordinate everytime when using it, update and reuse the same
//   tensor coordinate instead
//   3. Don't use a pointer to VGPR buffer, use vector instead

// Assume:
//   1. src_desc and dst_desc are not known at compile-time
//   2. SrcBuffer and DstBuffer are DynamicBuffer
//   3. src_slice_origin and dst_slice_origin are not known at compile-time,
template <typename SrcDatas,
          typename DstDatas,
          typename SrcDescs,
          typename DstDescs,
          typename ElementwiseOperation,
          typename DstInMemOps, // Sequence<InMemoryDataOperationEnum ...>
          typename SliceLengths,
          typename DimAccessOrder,
          index_t VectorDim,
          index_t ScalarPerVector,
          typename SrcResetCoordinateAfterRunFlags, // Sequence<bool ...>
          typename DstResetCoordinateAfterRunFlags> // Sequence<bool ...>
struct ThreadwiseTensorSliceTransfer_v7
{
    static constexpr auto I0 = Number<0>{};
    static constexpr auto I1 = Number<1>{};
    static constexpr auto I2 = Number<2>{};

    static constexpr index_t nDim = SliceLengths::Size();

    static constexpr index_t nSrc = SrcDescs::Size();
    static constexpr index_t nDst = DstDescs::Size();

    using Index = MultiIndex<nDim>;

    // return a tuple of coordiantes for a tuple of tensor
    template <typename Descs,
              typename Indices,
              enable_if_t<Descs::Size() == Indices::Size(), bool> = false>
    static constexpr auto MakeCoordinates(const Descs& descs, const Indices& indices)
    {
        return generate_tuple([&](auto i) { return make_tensor_coordinate(descs[i], indices[i]); },
                              Number<Descs::Size()>{});
    }

    using SrcCoords = decltype(MakeCoordinates(SrcDescs{}, StaticallyIndexedArray<Index, nSrc>{}));
    using DstCoords = decltype(MakeCoordinates(DstDescs{}, StaticallyIndexedArray<Index, nDst>{}));

    // scalar per access on each dim
    // FIXME: don't use lambda_scalar_per_access
    static constexpr auto scalar_per_access = generate_sequence(
        detail::lambda_scalar_per_access<VectorDim, ScalarPerVector>{}, Number<nDim>{});

    using SpaceFillingCurve =
        SpaceFillingCurve<SliceLengths, DimAccessOrder, remove_cv_t<decltype(scalar_per_access)>>;

    __device__ constexpr ThreadwiseTensorSliceTransfer_v7(
        const SrcDescs& src_descs,
        const StaticallyIndexedArray<Index, nSrc>& src_slice_origins,
        const DstDescs& dst_descs,
        const StaticallyIndexedArray<Index, nDst>& dst_slice_origins,
        const ElementwiseOperation& element_op)
        : src_coords_(MakeCoordinates(src_descs, src_slice_origins)),
          dst_coords_(MakeCoordinates(dst_descs, dst_slice_origins)),
          element_op_(element_op)
    {
        static_assert(SliceLengths::At(Number<VectorDim>{}) % ScalarPerVector == 0,
                      "wrong! cannot evenly divide");
    }

    template <typename Indices, enable_if_t<SrcDescs::Size() == Indices::Size(), bool> = false>
    __device__ void SetSrcSliceOrigins(const SrcDescs& src_descs,
                                       const Indices& src_slice_origin_idxs)
    {
        static_for<0, nSrc, 1>{}([&](auto i) {
            src_coords_(i) = make_tensor_coordinate(src_descs[i], src_slice_origin_idxs[i]);
        });
    }

    template <typename Indices, enable_if_t<DstDescs::Size() == Indices::Size(), bool> = false>
    __device__ void SetDstSliceOrigins(const DstDescs& dst_descs,
                                       const Indices& dst_slice_origin_idxs)
    {
        static_for<0, nDst, 1>{}([&](auto i) {
            dst_coords_(i) = make_tensor_coordinate(dst_descs[i], dst_slice_origin_idxs[i]);
        });
    }

    // SrcDescs: Tuple<const SrcDesc0&, const SrcDesc1&, ...>
    // SrcBuffers: Tuple<const SrcBuffer0&, const SrcBuffer1&, ...>
    // DstDescs: Tuple<const DstDesc0&, const DstDesc1&, ...>
    // DstBuffers: Tuple<const DstBuffer0&, const DstBuffer1&, ...>
    template <typename SrcBuffers,
              typename DstBuffers,
              enable_if_t<SrcDescs::Size() == SrcBuffers::Size() &&
                              DstDescs::Size() == DstBuffers::Size(),
                          bool> = false>
    __device__ void Run(const SrcDescs& src_descs,
                        const SrcBuffers& src_bufs,
                        const DstDescs& dst_descs,
                        DstBuffers dst_bufs)
    {
        auto generate_vectors = [&](auto data_types) {
            constexpr index_t num = data_types.Size();

            return generate_tuple(
                [&](auto i) {
                    using DataType = remove_cvref_t<decltype(data_types[i])>;

                    return vector_type_maker_t<DataType, ScalarPerVector>{};
                },
                Number<num>{});
        };

        constexpr auto num_access = SpaceFillingCurve::GetNumOfAccess();

        // loop over space-filling curve
        static_for<0, num_access, 1>{}([&](auto iAccess) {
            auto src_vectors = generate_vectors(SrcDatas{});
            auto dst_vectors = generate_vectors(DstDatas{});

            // copy data from src_bufs into src_vectors
            static_for<0, nSrc, 1>{}([&](auto i) {
                using src_vector_t = typename remove_cvref_t<decltype(src_vectors[i])>::type;

                const bool is_src_valid =
                    coordinate_has_valid_offset_assuming_visible_index_is_valid(src_descs[i],
                                                                                src_coords_[i]);

                src_vectors(i) = src_bufs[i].template Get<src_vector_t>(src_coords_[i].GetOffset(),
                                                                        is_src_valid);
            });

            // apply pointwise function
            // FIXME: support tuple of arbitary size
            static_for<0, ScalarPerVector, 1>{}([&](auto i) {
                using SrcData0 = remove_cvref_t<decltype(SrcDatas{}[I0])>;
                using SrcData1 = remove_cvref_t<decltype(SrcDatas{}[I1])>;
                using SrcData2 = remove_cvref_t<decltype(SrcDatas{}[I2])>;

                using DstData0 = remove_cvref_t<decltype(DstDatas{}[I0])>;

                element_op_(dst_vectors(I0).template AsType<DstData0>()(i),
                            src_vectors[I0].template AsType<SrcData0>()[i],
                            src_vectors[I1].template AsType<SrcData1>()[i],
                            src_vectors[I2].template AsType<SrcData2>()[i]);
            });

            // copy data from buf_vectors into dst_bufs
            static_for<0, nDst, 1>{}([&](auto i) {
                using dst_vector_t = typename remove_cvref_t<decltype(dst_vectors[i])>::type;

                const bool is_dst_valid =
                    coordinate_has_valid_offset_assuming_visible_index_is_valid(dst_descs[i],
                                                                                dst_coords_[i]);

                constexpr InMemoryDataOperationEnum DstInMemOp =
                    static_cast<InMemoryDataOperationEnum>(DstInMemOps::At(i.value));

                dst_bufs(i).template Update<DstInMemOp, dst_vector_t>(
                    dst_coords_[i].GetOffset(),
                    is_dst_valid,
                    dst_vectors[i].template AsType<dst_vector_t>()[I0]);
            });

            // move coordinate
            if constexpr(iAccess.value != num_access - 1)
            {
                constexpr auto forward_step = SpaceFillingCurve::GetForwardStep(iAccess);

                static_for<0, nSrc, 1>{}([&](auto i) {
                    move_tensor_coordinate(src_descs[i],
                                           src_coords_(i),
                                           make_tensor_coordinate_step(src_descs[i], forward_step));
                });

                static_for<0, nDst, 1>{}([&](auto i) {
                    move_tensor_coordinate(dst_descs[i],
                                           dst_coords_(i),
                                           make_tensor_coordinate_step(dst_descs[i], forward_step));
                });
            }
        });

        // move coordinate back to slice origin (or not)
        static_for<0, nSrc, 1>{}([&](auto i) {
            if constexpr(SrcResetCoordinateAfterRunFlags::At(i))
            {
                const auto src_reset_step =
                    make_tensor_coordinate_step(src_descs[i], GetCoordinateResetStep());

                move_tensor_coordinate(src_descs[i], src_coords_(i), src_reset_step);
            }
        });

        static_for<0, nDst, 1>{}([&](auto i) {
            if constexpr(DstResetCoordinateAfterRunFlags::At(i))
            {
                const auto dst_reset_step =
                    make_tensor_coordinate_step(dst_descs[i], GetCoordinateResetStep());

                move_tensor_coordinate(dst_descs[i], dst_coords_(i), dst_reset_step);
            }
        });
    }

    __device__ static constexpr auto GetCoordinateResetStep()
    {
        constexpr auto num_access = SpaceFillingCurve::GetNumOfAccess();

        if constexpr(num_access == 0)
        {
            return typename SpaceFillingCurve::Index{};
        }
        else
        {
            constexpr auto reset_step =
                SpaceFillingCurve::GetStepBetween(Number<num_access - 1>{}, Number<0>{});

            return reset_step;
        }
    }

    // src_slice_origin_step_idx need to be known at compile-time, for performance reason
    template <index_t ISrc>
    __device__ void MoveSrcSliceWindow(const SrcDescs& src_descs,
                                       Number<ISrc> iSrc,
                                       const Index& src_slice_origin_step_idx)
    {
        // if src coord was not reset by RunRead(), then need to adjust the step here
        const auto adjusted_step_idx = SrcResetCoordinateAfterRunFlags::At(iSrc)
                                           ? src_slice_origin_step_idx
                                           : src_slice_origin_step_idx + GetCoordinateResetStep();

        // is it OK to construct a new step every time?
        const auto adjusted_step = make_tensor_coordinate_step(src_descs[iSrc], adjusted_step_idx);

        move_tensor_coordinate(src_descs[iSrc], src_coords_(iSrc), adjusted_step);
    }

    // dst_slice_origin_step_idx need to be known at compile-time, for performance reason
    template <index_t IDst>
    __device__ void MoveDstSliceWindow(const DstDescs& dst_descs,
                                       Number<IDst> iDst,
                                       const Index& dst_slice_origin_step_idx)
    {
        // if dst coord was not reset by Run(), then need to adjust the step here
        const auto adjusted_step_idx = DstResetCoordinateAfterRunFlags::At(iDst)
                                           ? dst_slice_origin_step_idx
                                           : dst_slice_origin_step_idx + GetCoordinateResetStep();

        // is it OK to construct a new step every time?
        const auto adjusted_step = make_tensor_coordinate_step(dst_descs[iDst], adjusted_step_idx);

        move_tensor_coordinate(dst_descs[iDst], dst_coords_(iDst), adjusted_step);
    }

    private:
    SrcCoords src_coords_;
    DstCoords dst_coords_;
    const ElementwiseOperation element_op_;
};

} // namespace ck
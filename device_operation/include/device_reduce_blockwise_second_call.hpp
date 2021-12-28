#ifndef DEVICE_REDUCE_BLOCKWISE_SECOND_CALL_HPP
#define DEVICE_REDUCE_BLOCKWISE_SECOND_CALL_HPP

#include <iostream>
#include "device.hpp"
#include "device_reduce.hpp"
#include "device_reduce_common.hpp"
#include "gridwise_2d_reduction_blockwise.hpp"

namespace ck {
namespace tensor_operation {
namespace device {

template <typename inType, typename compType, typename outType, int rank, typename toReduceDims, 
	  ReduceTensorOp_t reduceOp, NanPropagation_t nanOpt, ReduceTensorIndices_t indicesOpt,
	  int blockSize, int dim0_thread_cluster_size, int dim1_thread_cluster_size, 
	  int dim0_max_vector_size, int dim1_max_vector_size, int dim0_thread_slice_size, int dim1_thread_slice_size>
struct DeviceReduceBlockWiseSecondCall : public DeviceReduce<inType, compType, outType, rank, toReduceDims, reduceOp, nanOpt, indicesOpt> 
{
     static_assert(std::is_same<inType, compType>::value, "inType and compType should be the same to use DEviceReduceBlockWiseSecondCall!"); 

     using invariantDims = decltype(get_invariantDims<rank, toReduceDims>());

     static constexpr index_t dstDims = (invariantDims::Size() == 0)? 1 : invariantDims::Size();

     static constexpr bool need_indices = (reduceOp == ReduceTensorOp_t::MIN || reduceOp == ReduceTensorOp_t::MAX || reduceOp == ReduceTensorOp_t::AMAX) 
	                                            && (indicesOpt != ReduceTensorIndices_t::NO_INDICES); 

     static constexpr int dim0_tile_size = dim0_thread_cluster_size * dim0_thread_slice_size;
     static constexpr int dim1_tile_size = dim1_thread_cluster_size * dim1_thread_slice_size;

     size_t getWorkspaceSize(const std::vector<int> & inLengths) override
     {
         (void) inLengths; 
         return(0);
     };

     static auto MakeSrc2dDescriptor(const std::vector<int> & inLengths, const std::vector<int> & inStrides, size_t gridSize)
     {
         const auto tupleSrcLengths = make_tuple_from_array(inLengths, Number<2>{});
         const auto tupleSrcStrides = make_tuple_from_array(inStrides, Number<2>{});

         const auto src2dDesc = make_naive_tensor_descriptor(tupleSrcLengths, tupleSrcStrides);

         const auto invariantLen = src2dDesc.GetLength(Number<0>{});
         const auto toReduceLen  = src2dDesc.GetLength(Number<1>{});

         const auto srcPad1 = gridSize * dim0_tile_size - invariantLen;
         const auto srcPad2 = ((toReduceLen + dim1_tile_size - 1) / dim1_tile_size) * dim1_tile_size - toReduceLen;

         auto src2dDesc_2 = transform_tensor_descriptor(src2dDesc,
                                            make_tuple(make_pad_transform(invariantLen, 0, srcPad1),
                                                       make_pad_transform(toReduceLen, 0, srcPad2)),
                                            make_tuple(Sequence<0>{}, Sequence<1>{}),
                                            make_tuple(Sequence<0>{}, Sequence<1>{}));

         return(src2dDesc_2);
     };

     static auto MakeDst1dDescriptor(const std::vector<int> & outLengths, const std::vector<int> & outStrides, size_t gridSize)
     {
         const auto tupleDstLengths = make_tuple_from_array(outLengths, Number<dstDims>{});
         const auto tupleDstStrides = make_tuple_from_array(outStrides, Number<dstDims>{});

         auto dstDesc = make_naive_tensor_descriptor(tupleDstLengths, tupleDstStrides);

         auto dst1dDesc = transform_tensor_descriptor(dstDesc,
                                         make_tuple(make_merge_transform(tupleDstLengths)),
                                         make_tuple(typename arithmetic_sequence_gen<0, dstDims, 1>::type{}),
                                         make_tuple(Sequence<0>{}));

         const auto invariantLen = dst1dDesc.GetLength(Number<0>{});

         const auto dstPad = gridSize * dim0_tile_size - invariantLen;

         auto dst1dDesc_2 = transform_tensor_descriptor(dst1dDesc,
                                            make_tuple(make_pad_transform(invariantLen, 0, dstPad)),
                                            make_tuple(Sequence<0>{}),
                                            make_tuple(Sequence<0>{}));
         return(dst1dDesc_2);
     };
     
     struct Argument : public BaseArgument
     {
         Argument(const std::vector<int> inLengths, const std::vector<int> inStrides, const std::vector<int> outLengths, const std::vector<int> outStrides,
                             float alpha, float beta, const inType *in_dev, outType *out_dev, int *out_indices_dev, compType *workspace_dev)
             : in_dev_{in_dev}, out_dev_{out_dev}, out_indices_dev_{out_indices_dev} 
         {
             (void) workspace_dev; 

             inLengths_ = inLengths; 
             inStrides_ = inStrides; 
             outLengths_ = outLengths; 
             outStrides_ = outStrides; 	     
	     alpha_ = static_cast<inType>(alpha); 
	     beta_ = static_cast<outType>(beta); 

             dim0_total_length = inLengths[0]; 
	     dim1_total_length = inLengths[1]; 

             dim0_lowest_length = inLengths[0];
             dim1_lowest_length = inLengths[1];

             gridSize = (dim0_total_length + (dim0_thread_cluster_size*dim0_thread_slice_size-1)) / (dim0_thread_cluster_size*dim0_thread_slice_size) * 
		                             (dim0_thread_cluster_size*dim0_thread_slice_size);
         }

         std::vector<int> inLengths_;
         std::vector<int> inStrides_;
         std::vector<int> outLengths_;
         std::vector<int> outStrides_;

         inType alpha_;
         outType beta_;

         const inType* in_dev_;
         outType* out_dev_;
         int* out_indices_dev_;

         int dim0_lowest_length;
         int dim1_lowest_length;
         size_t dim0_total_length;
         size_t dim1_total_length;

         size_t gridSize;
     };      

     struct Invoker : public BaseInvoker
     {
         float Run(const Argument& arg, int nrepeat = 1)
         {
             const auto src2dDesc = DeviceReduceBlockWiseSecondCall::MakeSrc2dDescriptor(arg.inLengths_, arg.inStrides_, arg.gridSize);
             const auto dst1dDesc = DeviceReduceBlockWiseSecondCall::MakeDst1dDescriptor(arg.outLengths_, arg.outStrides_, arg.gridSize);
             using src2dDescType = decltype(src2dDesc);
             using dst1dDescType = decltype(dst1dDesc);

             using gridwise_reduce = GridwiseReduction_xy_to_x_blockwise<inType, outType, compType, src2dDescType, dst1dDescType,
                                                 reduceOp, nanOpt, indicesOpt, blockSize, dim0_thread_cluster_size, dim1_thread_cluster_size,
                                                 dim0_thread_slice_size, dim1_thread_slice_size, dim0_max_vector_size, dim1_max_vector_size, false, true>;

             constexpr int RunId = need_indices ? 3 : 1;

             float avg_time = 0;

	     const auto kernel = kernel_reduce_blockwise<gridwise_reduce, RunId, inType, outType, src2dDescType, dst1dDescType>;

             avg_time = launch_and_time_kernel(kernel, nrepeat,
                                                  dim3(arg.gridSize),
                                                  dim3(blockSize),
                                                  0,
                                                  src2dDesc,
                                                  dst1dDesc,
                                                  static_cast<int>(arg.dim1_total_length),
                                                  arg.alpha_,
                                                  arg.in_dev_,
                                                  arg.beta_,
                                                  arg.out_dev_,
                                                  nullptr,
                                                  nullptr);


            return(avg_time);
         };

         float Run(const BaseArgument* p_arg, int nrepeat = 1) override
         {
            return Run(*dynamic_cast<const Argument*>(p_arg), nrepeat);
         };
     }; 	

     bool IsSupportedArgument(const BaseArgument* p_arg) override
     {
         const Argument *pArg = dynamic_cast<const Argument*>(p_arg); 

         if ( pArg->dim0_lowest_length % dim0_thread_slice_size != 0)
              return(false);

         if ( pArg->dim1_lowest_length % dim1_thread_slice_size != 0)
              return(false);

         // cases with very small dim1_total_length should be handled by the ThreadWise method   
         if ( pArg->dim1_total_length / dim1_thread_slice_size < 2)
              return(false);

         return(true);
     };

     std::unique_ptr<BaseArgument> MakeArgumentPointer(const std::vector<int> & inLengths, const std::vector<int> & inStrides,
		                                       const std::vector<int> & outLengths, const std::vector<int> & outStrides,
                                                       float alpha, float beta, const void *in_dev, void *out_dev, void *out_indices_dev, void *workspace_dev) override
     {
         return std::make_unique<Argument>(inLengths, inStrides, outLengths, outStrides, alpha, beta, 
			 static_cast<const inType*>(in_dev), static_cast<outType*>(out_dev), static_cast<int*>(out_indices_dev), static_cast<compType*>(workspace_dev)); 
     }; 

     std::unique_ptr<BaseInvoker> MakeInvokerPointer() override
     {
         return std::make_unique<Invoker>();
     }; 
};

} // namespace device
} // namespace tensor_operation
} // namespace ck
#endif

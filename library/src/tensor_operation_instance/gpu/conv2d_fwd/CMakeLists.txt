# device_conv2d_fwd_instance
set(DEVICE_CONV2D_FWD_INSTANCE_SOURCE
   device_conv2d_fwd_xdl_nhwc_kyxc_nhwk_f32_instance.cpp;
   device_conv2d_fwd_xdl_nhwc_kyxc_nhwk_f16_instance.cpp;
   device_conv2d_fwd_xdl_nhwc_kyxc_nhwk_bf16_instance.cpp;
   device_conv2d_fwd_xdl_nhwc_kyxc_nhwk_int8_instance.cpp;
   device_conv2d_fwd_xdl_c_shuffle_nhwc_kyxc_nhwk_f16_instance.cpp;
)
set(DEVICE_CONVND_2D_FWD_INSTANCE_SOURCE
   device_convnd_2d_fwd_xdl_nhwc_kyxc_nhwk_f32_instance.cpp;
   device_convnd_2d_fwd_xdl_nhwc_kyxc_nhwk_f16_instance.cpp;
   device_convnd_2d_fwd_xdl_nhwc_kyxc_nhwk_bf16_instance.cpp;
   device_convnd_2d_fwd_xdl_nhwc_kyxc_nhwk_int8_instance.cpp;
)

add_library(device_conv2d_fwd_instance OBJECT ${DEVICE_CONV2D_FWD_INSTANCE_SOURCE}) 
add_library(device_convnd_2d_fwd_instance OBJECT ${DEVICE_CONVND_2D_FWD_INSTANCE_SOURCE}) 

set_target_properties(device_conv2d_fwd_instance PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(device_convnd_2d_fwd_instance PROPERTIES POSITION_INDEPENDENT_CODE ON)

clang_tidy_check(device_conv2d_fwd_instance)
clang_tidy_check(device_convnd_2d_fwd_instance)

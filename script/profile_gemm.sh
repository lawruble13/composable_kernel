#!/bin/bash

## GPU visibility
 export HIP_VISIBLE_DEVICES=0

 make -j ckProfiler

 DRIVER="./profiler/ckProfiler"

OP=$1
DATATYPE=$2
LAYOUT=$3
VERIFY=$4
INIT=$5
LOG=$6
REPEAT=$7
KBATCH=$8

########  op  datatype  layout  verify  init  log  repeat  M___ N___ K___  StrideA StrideB StrideC
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT   256  256  256      256     256     256
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT   960 1024 1024     1024    1024    1024
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT  1920 2048 2048     2048    2048    2048
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT  3840 4096 4096     4096    4096    4096
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT  7680 8192 8192     8192    8192    8192
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT  1024 1024 1024     1024    1024    1024
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT  2048 2048 2048     2048    2048    2048

for BATCH in 16 32 64 512 768 1024 1280 1536
do
 $DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT $BATCH  1920 5120      -1     -1      -1    $KBATCH
 $DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT $BATCH  5120  640      -1     -1      -1    $KBATCH
 $DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT $BATCH  2560 5120      -1     -1      -1    $KBATCH
 $DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT $BATCH  5120 2560      -1     -1      -1    $KBATCH
 $DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT $BATCH  50304 640      -1     -1      -1    $KBATCH
done

#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT  960  1024 1024      -1     -1      -1
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 1920  2048 2048      -1     -1      -1
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 3840  4096 4096      -1     -1      -1
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 7680  8192 8192      -1     -1      -1

#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 1024  1024 1024	 1024	1024	1024
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 2048  2048 2048	 2048	2048	2048
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 4096  4096 4096	 4096	4096	4096
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 8192  8192 8192	 8192	8192	8192

#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 1024  1024 1024	 1056	1056	1056
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 2048  2048 2048	 2080	2080	2080
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 4096  4096 4096	 4128	4128	4128
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 8192  8192 8192	 8224	8224	8224

#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 1024  1024 1024	 1088	1088	1088
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 2048  2048 2048	 2112	2112	2112
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 4096  4096 4096	 4160	4160	4160
#$DRIVER $OP $DATATYPE $LAYOUT $VERIFY $INIT $LOG $REPEAT 8192  8192 8192	 8256	8256	8256

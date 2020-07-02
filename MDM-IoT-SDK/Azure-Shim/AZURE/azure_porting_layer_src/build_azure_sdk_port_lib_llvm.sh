#!/bin/sh
set fileformat=unix
echo "beginning"
export LLVMROOT=/pkg/qct/software/llvm/release/arm/4.0.3
#export LLVMROOT=c:/apps/LLVM/4.0.3
export TOOLCHAIN_PATH=$LLVMROOT/bin
CC=clang
AR=llvm-ar
SDK_LIB_PATH=libs
INC_BASE=../../common/include
AZURE_PORT_SRC_PATH=../porting-layer
AZURE_SDK_INC_PATH=../azure-iot-sdk-c
AZURE_PORT_INC_PATH=../porting-layer
PORT_OUTPUT_PATH=../porting-layer/build
SDK_PORT_LIB_NAME=azure_sdk_port.lib

if test -d "${PORT_OUTPUT_PATH}"; then
	echo "${PORT_OUTPUT_PATH} is present"
else
	echo "mkdir ../azure-iot-sdk-c/build"
	mkdir -p ${PORT_OUTPUT_PATH}
fi
if test -f "$SDK_LIB_PATH"; then
	echo "$SDK_LIB_PATH file present and deleting it"
	rm "$SDK_LIB_PATH"
	mkdir "$SDK_LIB_PATH"
elif test -d "$SDK_LIB_PATH" 
then
	echo "$SDK_LIB_PATH exits"
else
	mkdir "$SDK_LIB_PATH"
fi
SDK_CFLAGS="-DQAPI_TXM_MODULE -DTXM_MODULE -DTX_DAM_QC_CUSTOMIZATIONS -DTX_ENABLE_PROFILING -DTX_ENABLE_EVENT_TRACE -DTX_DISABLE_NOTIFY_CALLBACKS -DQCOM_THREADX_LOG -DAZURE_IOT -DDONT_USE_UPLOADTOBLOB -DQCOM_LLVM_STANDARD_LIB_OVERRIDE -D_WANT_IO_C99_FORMATS -marm -mllvm -enable-gvn-hoist -target armv7m-none-musleabi -mfloat-abi=softfp -mfpu=none -mcpu=cortex-a7 -mno-unaligned-access -fms-extensions -fuse-baremetal-sysroot -fdata-sections -ffunction-sections -fomit-frame-pointer -fshort-enums -Wno-unused-parameter -Wno-missing-field-initializers"					 
SDK_INCPATHS="-I $INC_BASE -I $INC_BASE/threadx_api -I $INC_BASE/qapi -I $AZURE_PORT_INC_PATH/inc -I $AZURE_SDK_INC_PATH/c-utility/inc -I $AZURE_SDK_INC_PATH/c-utility/pal/generic -I $AZURE_SDK_INC_PATH/serializer/inc -I $AZURE_SDK_INC_PATH/iothub_client/inc -I $AZURE_SDK_INC_PATH/deps/parson -I $AZURE_SDK_INC_PATH/umqtt/inc -I $LLVMROOT/armv7m-none-eabi/libc/include"			  		  
			  	
declare -a AZURE_SDK_FILES=("$AZURE_PORT_SRC_PATH/src/lock_qcom_threadx.c"
    "$AZURE_PORT_SRC_PATH/src/agenttime_qcom_threadx.c"
    "$AZURE_PORT_SRC_PATH/src/platform_qcom_threadx.c"
    "$AZURE_PORT_SRC_PATH/src/threadapi_qcom_threadx.c"
    "$AZURE_PORT_SRC_PATH/src/tickcounter_qcom_threadx.c"
    "$AZURE_PORT_SRC_PATH/src/certs.c"	
    "$AZURE_PORT_SRC_PATH/src/tlsio_qcom_threadx.c")

echo "continue"
rm $PORT_OUTPUT_PATH/*.o 

for sdk_file in "${AZURE_SDK_FILES[@]}" 
do
    echo "Building porting library - $TOOLCHAIN_PATH/$CC -g -c $SDK_CFLAGS $SDK_INCPATHS $sdk_file"	
	$TOOLCHAIN_PATH/$CC -g -c $SDK_CFLAGS $SDK_INCPATHS $sdk_file
done

mv *.o $PORT_OUTPUT_PATH
echo "end"

for outfile in $PORT_OUTPUT_PATH/*.o
do
	$TOOLCHAIN_PATH/$AR -rcs $PORT_OUTPUT_PATH/$SDK_PORT_LIB_NAME $outfile
done

mv $PORT_OUTPUT_PATH/$SDK_PORT_LIB_NAME $SDK_LIB_PATH

echo "Finished compiling SDK library, built at $PORT_OUTPUT_PATH/$SDK_PORT_LIB_NAME"
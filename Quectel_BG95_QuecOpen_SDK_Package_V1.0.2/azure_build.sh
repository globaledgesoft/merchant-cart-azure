DEMO_ELF_OUTPUT_PATH="./bin"
DEMO_APP_OUTPUT_PATH="./quectel/example/build"


DAM_RO_BASE=0x43000000
TOOL_PATH_ROOT="../MDM-Others/Tools"
TOOLCHAIN_PATH="$TOOL_PATH_ROOT/LLVM/bin"
#echo TOOLCHAIN_PATH is $TOOLCHAIN_PATH
TOOLCHAIN_PATH_STANDARdS="$TOOL_PATH_ROOT/LLVM/armv7m-none-eabi/libc/include"
LLVMLIB="$TOOL_PATH_ROOT/LLVM/lib/clang/4.0.12/lib"
LLVMLINK_PATH="$TOOL_PATH_ROOT/LLVM/tools/bin"
PYTHON_PATH="/usr/bin/python"
DAM_INC_BASE="./include"
DAM_LIB_PATH="./libs"
DEMO_SRC_PATH="./quectel/example"
DEMO_APP_LD_PATH="./quectel/build"
DEMO_APP_UTILS_SRC_PATH="./quectel/utils/source"

DEMO_APP_UTILS_INC_PATH="./quectel/utils/include"
DAM_LIBNAME="txm_lib.lib"
TIMER_LIBNAME="timer_dam_lib.lib"
DIAG_LIB_NAME="diag_dam_lib.lib"
QMI_LIB_NAME="qcci_dam_lib.lib"
QMI_QCCLI_LIB_NAME="IDL_DAM_LIB.lib"
DAM_ELF_NAME="quectel_demo_azure.elf"
DAM_TARGET_BIN="quectel_demo_azure.bin"
DAM_MAP_NAME="quectel_demo_azure.map"


##########AZURE########################

AZURE_ROOT_PATH="../MDM-IoT-SDK/Azure-Shim/AZURE"
AZURE_SRC_PATH="$AZURE_ROOT_PATH/azure_porting_layer_src/porting-layer/src"
AZURE_INC_PATH="$AZURE_ROOT_PATH/azure_porting_layer_src/porting-layer/inc"
AZURE_MACRO_INC_PATH="$AZURE_ROOT_PATH/azure-macro-utils-c/inc/"
AZURE_UMOCK_INC_PATH="$AZURE_ROOT_PATH/umock-c/inc"
AZURE_HUB_SERVICE_SRC="$AZURE_ROOT_PATH/iothub_service_client/src"
AZURE_HUB_SERVICE_INC="$AZURE_ROOT_PATH/iothub_service_client/inc"
AZURE_UAMQP_SERVICE_SRC="$AZURE_ROOT_PATH/uamqp/src"
AZURE_UAMQP_SERVICE_INC="$AZURE_ROOT_PATH/uamqp/inc"
AZURE_UMQTT_SERVICE_SRC="$AZURE_ROOT_PATH/umqtt/src"
AZURE_UMQTT_SERVICE_INC="$AZURE_ROOT_PATH/umqtt/inc"
AZURE_HUB_CLIENT_SRC="$AZURE_ROOT_PATH/iothub_client/src"
AZURE_HUB_CLIENT_INC="$AZURE_ROOT_PATH/iothub_client/inc"
AZURE_APP_SRC="$AZURE_ROOT_PATH/iothub_client/samples/iothub_ll_telemetry_sample"
AZURE_PARSON_SRC="$AZURE_ROOT_PATH/parson/"
AZURE_PARSON_INC="$AZURE_ROOT_PATH/parson/"
AZURE_C_UTILITY_SRC="$AZURE_ROOT_PATH/c-utility/src"
AZURE_DIR_ROOT="$AZURE_ROOT_PATH"


##########AZURE########################


echo "== Application RO base selected = $DAM_RO_BASE"



DAM_CPPFLAGS="-DQAPI_TXM_MODULE -DTXM_MODULE -DTX_DAM_QC_CUSTOMIZATIONS -DTX_ENABLE_PROFILING -DTX_ENABLE_EVENT_TRACE -DTX_DISABLE_NOTIFY_CALLBACKS -D__EXAMPLE_HTTP__ -DFX_FILEX_PRESENT -DTX_ENABLE_IRQ_NESTING  -DTX3_CHANGES -DAZURE_9206 -DNO_LOGGING -DDONT_USE_UPLOADTOBLOB -DSET_TRUSTED_CERT_IN_SAMPLES -D__EXAMPLE_GPIO__ -DAZURE_BG95" 


#DAM_CFLAGS="-DQCOM_LLVM_STANDARD_LIB_OVERRIDE -D_WANT_IO_C99_FORMATS -marm -mllvm -enable-gvn-hoist -target armv7m-none-musleabi -mfloat-abi=softfp -mfpu=none -mcpu=cortex-a7 -mno-unaligned-access -fms-extensions -fuse-baremetal-sysroot -fdata-sections -ffunction-sections -fomit-frame-pointer -fshort-enums -Wno-unused-parameter -Wno-missing-field-initializers" 

DAM_CFLAGS="-marm -target armv7m-none-musleabi -mfloat-abi=softfp -mfpu=none -mcpu=cortex-a7 -mno-unaligned-access  -fms-extensions -Osize -fshort-enums -Wbuiltin-macro-redefined"

DAM_INCPATHS="-I $DAM_INC_BASE -I $DAM_INC_BASE/threadx_api -I $DAM_INC_BASE/qmi -I $DAM_INC_BASE/qapi -I $TOOLCHAIN_PATH_STANDARdS -I $LLVMLIB -I $DEMO_APP_UTILS_INC_PATH -I $AZURE_INC_PATH -I $AZURE_MACRO_INC_PATH -I $AZURE_UMOCK_INC_PATH -I $AZURE_HUB_SERVICE_INC -I$AZURE_PARSON_INC -I $AZURE_UAMQP_SERVICE_INC -I $AZURE_UMQTT_SERVICE_INC -I $AZURE_HUB_CLIENT_INC"


echo == Cleaning... $DEMO_ELF_OUTPUT_PATH
rm -rf $DEMO_ELF_OUTPUT_PATH/*
rm -rf $DEMO_ELF_OUTPUT_PATH/*o
echo == Cleaning... $DEMO_APP_OUTPUT_PATH
rm -rf $DEMO_APP_OUTPUT_PATH/*
rm -rf $DEMO_APP_OUTPUT_PATH/*o
echo "Done...cleaning"

set -x;


echo "== Compiling .S file..."
$TOOLCHAIN_PATH/clang -E  $DAM_CPPFLAGS $DAM_CFLAGS $DEMO_SRC_PATH/txm_module_preamble_llvm.S > txm_module_preamble_llvm_pp.S
$TOOLCHAIN_PATH/clang -c $DAM_CPPFLAGS $DAM_CFLAGS txm_module_preamble_llvm_pp.S -o $DEMO_APP_OUTPUT_PATH/txm_module_preamble_llvm.o
rm txm_module_preamble_llvm_pp.S


echo "==quectel utils..."


$TOOLCHAIN_PATH/clang  -c  $DAM_CPPFLAGS $BUILD_APP_FLAG $DAM_CFLAGS $DAM_INCPATHS $DEMO_APP_UTILS_SRC_PATH/*.c
mv *.o $DEMO_APP_OUTPUT_PATH



echo "== Compiling Azure C utility files..."
$TOOLCHAIN_PATH/clang  -c $DAM_CPPFLAGS $DAM_CFLAGS $APP_CFLAGS $BUILD_APP_FLAG $DAM_INCPATHS $AZURE_C_UTILITY_SRC/*.c

mv *.o $DEMO_APP_OUTPUT_PATH
echo
echo



echo "== Compiling Azure adapters files..."
$TOOLCHAIN_PATH/clang -c $DAM_CPPFLAGS $DAM_CFLAGS $APP_CFLAGS $BUILD_APP_FLAG $DAM_INCPATHS $AZURE_SRC_PATH/*.c

mv *.o $DEMO_APP_OUTPUT_PATH
echo
echo


echo "== Compiling Azure parson files..."
$TOOLCHAIN_PATH/clang -c $DAM_CPPFLAGS $DAM_CFLAGS $APP_CFLAGS $BUILD_APP_FLAG $DAM_INCPATHS $AZURE_PARSON_SRC/*.c

mv *.o $DEMO_APP_OUTPUT_PATH
echo
echo


echo "== Compiling Azure uMQTT files..."
$TOOLCHAIN_PATH/clang -c $DAM_CPPFLAGS $DAM_CFLAGS $APP_CFLAGS $BUILD_APP_FLAG $DAM_INCPATHS $AZURE_UMQTT_SERVICE_SRC/*.c


mv *.o $DEMO_APP_OUTPUT_PATH
echo
echo



echo "== Compiling Azure hub client files..."
$TOOLCHAIN_PATH/clang -c $DAM_CPPFLAGS $DAM_CFLAGS $APP_CFLAGS $BUILD_APP_FLAG $DAM_INCPATHS $AZURE_HUB_CLIENT_SRC/*.c

mv *.o $DEMO_APP_OUTPUT_PATH
echo
echo


echo "== Compiling Azure application files..."
$TOOLCHAIN_PATH/clang -c $DAM_CPPFLAGS $DAM_CFLAGS $APP_CFLAGS $BUILD_APP_FLAG $DAM_INCPATHS $AZURE_APP_SRC/*.c $AZURE_DIR_ROOT/quectel_data_call_bg95.c $AZURE_DIR_ROOT/example_gpio_bg95.c


mv *.o $DEMO_APP_OUTPUT_PATH
echo
echo


echo "== Linking azure application"
$TOOLCHAIN_PATH/clang++ -d -o $DEMO_ELF_OUTPUT_PATH/$DAM_ELF_NAME -target armv7m-none-musleabi -fuse-ld=qcld -lc++ -Wl,-mno-unaligned-access -fuse-baremetal-sysroot -fno-use-baremetal-crt -Wl,-entry=$DAM_RO_BASE $DEMO_APP_OUTPUT_PATH/txm_module_preamble_llvm.o -Wl,-T$DEMO_APP_LD_PATH/quectel_dam_demo.ld -Wl,-Map=$DEMO_ELF_OUTPUT_PATH/$DAM_MAP_NAME,-gc-sections -Wl,-gc-sections $DEMO_APP_OUTPUT_PATH/*.o $DAM_LIB_PATH/*.lib
$PYTHON_PATH $LLVMLINK_PATH/llvm-elf-to-hex.py --bin $DEMO_ELF_OUTPUT_PATH/$DAM_ELF_NAME --output $DEMO_ELF_OUTPUT_PATH/$DAM_TARGET_BIN

echo "== Demo application is built in $DEMO_ELF_OUTPUT_PATH"
echo -n $DAM_TARGET_BIN > ./bin/oem_app_path.ini
echo "Done."
exit


# rkcodec
RTSP拉取H264流后，通过rockchip的MPP直接硬件转码为H265流

# note
该工程代码是基于rockchip平台上编译调试的，所以在调试该工程前，必须先
编译rockchip的SDK，关于SDK相关的需求项描述如下

## buildroot部分
首先确保buildroot已勾选了MPP模块，因为硬件编码需要调用到该库中的接口

## rockchip MPP
链接：https://github.com/hermanchen/mpp
版本：develop - 33269e39e7de30f7a3903f31422a3b567fec762c


## rkcodec工程路径连接
工程使用cmake来辅助搭建，使用Findrockchip_mpp.cmake来查找到rockchip的
MPP库，所以需要加载到SDK编译后配置的系统环境变量信息，包括交叉编译工具和
库文件路径等，使用cmake的宏
- CMAKE_TOOLCHAIN_FILE

# cross compile
参考编译命令：
    cmake .. -DCMAKE_TOOLCHAIN_FILE="/home/CUAV/kaisen/rv1126_rv1109_linux_v2.1.0_20210512/buildroot/output/rockchip_ltelink2/host/share/buildroot/toolchainfile.cmake" \
             -DCMAKE_EXPORT_COMPILE_COMMANDS=ON


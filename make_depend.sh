#!/bin/sh

SH_DIR=$(dirname `readlink -f $0`)

if [ $# -ne 2 ];
then
    echo usage: $SH_DIR/$0 project_dir compile_dir
    exit
fi

LIB_DIR=$1
COMPILE_DIR=$2

if [ ! -x $LIB_DIR ]; then
    echo $LIB_DIR directory does not exist
    exit
fi
if [ ! -x $COMPILE_DIR ]; then
    mkdir $COMPILE_DIR
fi;
COMPILE_NUM=`cat /proc/cpuinfo| grep  "processor" | wc -l`;

BOOST_LIB=$LIB_DIR/boost_1_76_0.tar.gz
BOOST_DIR=$COMPILE_DIR/boost
if [ ! -d $BOOST_DIR ];
then
    tar -xvf $BOOST_LIB -C $COMPILE_DIR;
    mv $COMPILE_DIR/boost_1_76_0 $BOOST_DIR;
fi;

CRYPTOPP_LIB=$LIB_DIR/cryptopp860.zip
CRYPTOPP_DIR=$COMPILE_DIR/cryptopp/
if [ ! -d $CRYPTOPP_DIR ];
then
    unzip $CRYPTOPP_LIB -d $CRYPTOPP_DIR;
    cd $CRYPTOPP_DIR && make static -j$COMPILE_NUM;
fi;

LIBBASE58_LIB=$LIB_DIR/libbase58-0.1.4.tar.gz
LIBBASE58_DIR=$COMPILE_DIR/libbase58
if [ ! -d $LIBBASE58_DIR ];
then
    tar -xvf $LIBBASE58_LIB -C $COMPILE_DIR;
    mv $COMPILE_DIR/libbase58-0.1.4 $LIBBASE58_DIR;
    cd $LIBBASE58_DIR
    ./autogen.sh
    ./configure --disable-tool
    make -j$COMPILE_NUM;
fi;

LIBEVENT_LIB=$LIB_DIR/libevent-2.1.12.tar.gz
LIBEVENT_DIR=$COMPILE_DIR/libevent
if [ ! -d $LIBEVENT_DIR ];
then
    tar -xvf $LIBEVENT_LIB -C $COMPILE_DIR;
    mv $COMPILE_DIR/libevent-2.1.12-stable $LIBEVENT_DIR;
    cd $LIBEVENT_DIR
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release  -DEVENT__DISABLE_TESTS=ON -DEVENT__DISABLE_SAMPLES=ON -DEVENT__DISABLE_BENCHMARK=ON
    #./autogen.sh && ./configure
    make -j$COMPILE_NUM
fi;

LIBFMT_LIB=$LIB_DIR/libfmt-7.1.3.tar.gz
LIBFMT_DIR=$COMPILE_DIR/libfmt
if [ ! -d $LIBFMT_DIR ];
then
    tar -xvf $LIBFMT_LIB -C $COMPILE_DIR;
    mv $COMPILE_DIR/fmt-7.1.3 $LIBFMT_DIR;
    cd $LIBFMT_DIR
    mkdir build && cd build
    cmake .. -DFMT_TEST=OFF -DFMT_DOC=OFF
    make -j$COMPILE_NUM
fi;

NLOHMANN_LIB=$LIB_DIR/nlohmann_json-3.9.1.tar.gz
NLOHMANN_DIR=$COMPILE_DIR/nlohmann_json
if [ ! -d $NLOHMANN_DIR ];
then
    tar -xvf $NLOHMANN_LIB -C $COMPILE_DIR;
    mv $COMPILE_DIR/json-3.9.1 $NLOHMANN_DIR;
fi;

PROTOBUF_LIB=$LIB_DIR/protobuf-3.17.3.tar.gz
PROTOBUF_DIR=$COMPILE_DIR/protobuf
if [ ! -d $PROTOBUF_DIR ];
then
    tar -xvf $PROTOBUF_LIB -C $COMPILE_DIR;
    mv $COMPILE_DIR/protobuf-3.17.3 $PROTOBUF_DIR;
    cd $PROTOBUF_DIR
    mkdir build && cd build
    cmake ../cmake -Dprotobuf_BUILD_TESTS=OFF
    #./autogen.sh && ./configure
    make -j$COMPILE_NUM
fi;

#ROCKSDB_LIB=$LIB_DIR/rocksdb-6.14.6.tar.gz
ROCKSDB_LIB=$LIB_DIR/rocksdb-6.4.6.zip
ROCKSDB_DIR=$COMPILE_DIR/rocksdb
if [ ! -d $ROCKSDB_DIR ];
then
    #tar -xvf $ROCKSDB_LIB -C $COMPILE_DIR;
    #$COMPILE_DIR/rocksdb-6.14.6 $ROCKSDB_DIR;
    unzip $ROCKSDB_LIB -d $COMPILE_DIR;
    mv $COMPILE_DIR/rocksdb-6.4.6 $ROCKSDB_DIR;
    cd $ROCKSDB_DIR
    mkdir build && cd build
    cmake .. -DFAIL_ON_WARNINGS=OFF -DROCKSDB_BUILD_SHARED=OFF -DPORTABLE=ON -DWITH_TESTS=OFF -DWITH_GFLAGS=OFF
    make -j$COMPILE_NUM
fi;

SPDLOG_LIB=$LIB_DIR/spdlog-1.8.2.tar.gz
SPDLOG_DIR=$COMPILE_DIR/spdlog
if [ ! -d $SPDLOG_DIR ];
then
    tar -xvf $SPDLOG_LIB -C $COMPILE_DIR;
    mv $COMPILE_DIR/spdlog-1.8.2 $SPDLOG_DIR;
    cd $SPDLOG_DIR
    mkdir build && cd build
    fmt_DIR=$LIBFMT_DIR/build cmake .. -DCMAKE_BUILD_TYPE=Release -DSPDLOG_FMT_EXTERNAL=yes -DSPDLOG_BUILD_EXAMPLE=OFF
    make -j$COMPILE_NUM
fi;
echo "-- compile depend done"

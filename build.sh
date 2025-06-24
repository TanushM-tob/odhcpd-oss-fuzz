#!/bin/bash -eu


apt-get update
apt-get install -y build-essential cmake pkg-config git libjson-c-dev

DEPS_DIR="$PWD/deps"
mkdir -p "$DEPS_DIR"
cd "$DEPS_DIR"

if [ ! -d "libubox" ]; then
    echo "Downloading libubox..."
    git clone https://github.com/openwrt/libubox.git
fi

cd libubox
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="$DEPS_DIR/install" \
         -DCMAKE_C_FLAGS="$CFLAGS" \
         -DBUILD_LUA=OFF \
         -DBUILD_EXAMPLES=OFF \
         -DBUILD_STATIC=ON \
         -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
make install
cd "$DEPS_DIR"

if [ ! -d "uci" ]; then
    echo "Downloading libuci..."
    git clone https://git.openwrt.org/project/uci.git
fi

cd uci
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="$DEPS_DIR/install" \
         -DCMAKE_C_FLAGS="$CFLAGS" \
         -DBUILD_LUA=OFF \
         -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
make install
cd "$DEPS_DIR"

if [ ! -d "libnl-tiny" ]; then
    echo "Downloading libnl-tiny..."
    git clone https://git.openwrt.org/project/libnl-tiny.git
fi

cd libnl-tiny
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="$DEPS_DIR/install" \
         -DCMAKE_C_FLAGS="$CFLAGS" \
         -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
make install
cd "$DEPS_DIR"

if [ ! -d "ubus" ]; then
    echo "Downloading libubus..."
    git clone https://git.openwrt.org/project/ubus.git
fi

cd ubus
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="$DEPS_DIR/install" \
         -DCMAKE_C_FLAGS="$CFLAGS" \
         -DBUILD_LUA=OFF \
         -DBUILD_EXAMPLES=OFF \
         -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
make install
cd "$DEPS_DIR"

cd ..

: "${CFLAGS:=-O2 -fPIC}"
: "${LDFLAGS:=}"
: "${PKG_CONFIG_PATH:=}"
: "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"  # Default to libFuzzer if not provided

# Add flag to suppress C23 extension warnings
export CFLAGS="$CFLAGS -Wno-c23-extensions"

export PKG_CONFIG_PATH="$DEPS_DIR/install/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export CFLAGS="$CFLAGS -I$DEPS_DIR/install/include"
export LDFLAGS="$LDFLAGS -L$DEPS_DIR/install/lib"

export CFLAGS="$CFLAGS -D_GNU_SOURCE -DDHCPV4_SUPPORT -DWITH_UBUS -std=gnu99"
export CFLAGS="$CFLAGS -I$DEPS_DIR/install/include/libnl-tiny"

cd src

echo "Compiling odhcpd source files..."
$CC $CFLAGS -c odhcpd.c -o odhcpd.o
$CC $CFLAGS -c config.c -o config.o
$CC $CFLAGS -c router.c -o router.o
$CC $CFLAGS -c dhcpv6.c -o dhcpv6.o
$CC $CFLAGS -c ndp.c -o ndp.o
$CC $CFLAGS -c dhcpv6-ia.c -o dhcpv6-ia.o
$CC $CFLAGS -c dhcpv6-pxe.c -o dhcpv6-pxe.o
$CC $CFLAGS -c netlink.c -o netlink.o
$CC $CFLAGS -c dhcpv4.c -o dhcpv4.o
$CC $CFLAGS -c ubus.c -o ubus.o

cd ..

echo "Compiling fuzzer..."
$CC $CFLAGS -c fuzz_odhcpd.c -o fuzz_odhcpd.o

echo "Linking fuzzer statically..."
$CC $CFLAGS $LIB_FUZZING_ENGINE fuzz_odhcpd.o \
    src/odhcpd.o src/config.o src/router.o src/dhcpv6.o src/ndp.o \
    src/dhcpv6-ia.o src/dhcpv6-pxe.o src/netlink.o src/dhcpv4.o \
    src/ubus.o \
    $LDFLAGS -static -lubox -luci -lnl-tiny -lresolv -ljson-c -lubus \
    -o $OUT/odhcpd_fuzzer
rm -f *.o src/*.o

echo "Build completed successfully!"
echo "Fuzzer binary: $OUT/odhcpd_fuzzer"


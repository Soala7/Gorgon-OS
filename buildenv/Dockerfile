# Start with Ubuntu 24.04
FROM ubuntu:24.04

# Prevent apt from asking questions during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install the tools needed to build the kernel environment
RUN apt-get update && apt-get install -y \
    build-essential \
    bison \
    flex \
    libgmp3-dev \
    libmpc-dev \
    libmpfr-dev \
    texinfo \
    nasm \
    xorriso \
    grub-pc-bin \
    grub-common \
    wget \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set where the cross-compiler will be installed
ENV PREFIX=/usr/local/cross

# Set the target architecture
ENV TARGET=x86_64-elf

# Add the cross-compiler to PATH
ENV PATH="$PREFIX/bin:$PATH"

# Use /tmp for building the compiler
WORKDIR /tmp

# Download and build Binutils
RUN wget https://ftp.gnu.org/gnu/binutils/binutils-2.43.1.tar.xz && \
    tar -xf binutils-2.43.1.tar.xz && \
    mkdir binutils-build && \
    cd binutils-build && \
    ../binutils-2.43.1/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --with-sysroot \
        --disable-nls \
        --disable-werror && \
    make -j$(nproc) && \
    make install

# Download and build GCC
RUN wget https://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.xz && \
    tar -xf gcc-14.2.0.tar.xz && \
    cd gcc-14.2.0 && \
    ./contrib/download_prerequisites && \
    mkdir ../gcc-build && \
    cd ../gcc-build && \
    ../gcc-14.2.0/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --disable-nls \
        --enable-languages=c,c++ \
        --without-headers && \
    make all-gcc -j$(nproc) && \
    make all-target-libgcc -j$(nproc) && \
    make install-gcc && \
    make install-target-libgcc

# Create persistent storage for the kernel project
VOLUME /root/env

# Set the kernel project directory as the working directory
WORKDIR /root/env

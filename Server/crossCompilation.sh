#!/bin/sh

cd /root/
tar zxvf buildroot-precompiled-2017.08.tar.gz
apt-get update
apt-get install -y cmake
cd rpi_camera_ip/Server/
mkdir -p build
cd build
cmake ..
make
cd ../bin/
file server

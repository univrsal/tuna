#!/bin/bash

cd libmpdclient
cp meson.build meson.build.bak

sed -i 's/libmpdclient = library/libmpdclient = static_library/g' meson.build
sed -i 's/soversion: splitted_version.*,$//g' meson.build
sed -i 's/version: meson.project_version(),//g' meson.build

meson setup ./build . --prefix="$(pwd)/install"
meson compile -C ./build && meson install -C ./build

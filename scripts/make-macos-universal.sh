#!/usr/bin/env bash




set -e

rm -rf "build/ninja-x64_osx_vcpkg-release"
rm -rf "build/ninja-arm64_osx_vcpkg-release"
cmake --preset ninja-x64_osx_vcpkg-release -DVCPKG_OVERLAY_TRIPLETS=scripts/ -DVCPKG_TARGET_TRIPLET=x64-osx-1015 -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 "-DSRB2_SDL2_EXE_NAME=srb2"
cmake --build --preset ninja-x64_osx_vcpkg-release
cmake --preset ninja-arm64_osx_vcpkg-release -DVCPKG_OVERLAY_TRIPLETS=scripts/ -DVCPKG_TARGET_TRIPLET=arm64-osx-1015 -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 "-DSRB2_SDL2_EXE_NAME=srb2"
cmake --build --preset ninja-arm64_osx_vcpkg-release

mkdir -p build/dist
rm -rf "build/dist/Sonic Robo Blast 2.app" "build/dist/srb2.app"

cp -r build/ninja-arm64_osx_vcpkg-release/bin/srb2.app build/dist/

lipo -create \
	-output "build/dist/srb2.app/Contents/MacOS/srb2" \
	build/ninja-x64_osx_vcpkg-release/bin/srb2.app/Contents/MacOS/srb2 \
	build/ninja-arm64_osx_vcpkg-release/bin/srb2.app/Contents/MacOS/srb2

mv build/dist/srb2.app "build/dist/Sonic Robo Blast 2.app"

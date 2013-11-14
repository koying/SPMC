#!/bin/bash
pushd . && cd tools/android/packaging/xbmc && ~/android-ndk-r8e/ndk-gdb --start --delay=0 --adb=/home/cbro/android-sdk-linux/platform-tools/adb && popd

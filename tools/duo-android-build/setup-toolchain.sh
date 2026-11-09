#!/bin/bash
# Installs JDK, Android SDK and NDK under ~/android-toolchain (no sudo needed).
set -euo pipefail
ROOT=$HOME/android-toolchain
mkdir -p "$ROOT" && cd "$ROOT"

if [ ! -d jdk ]; then
  curl -fL -o jdk.tar.gz "https://api.adoptium.net/v3/binary/latest/21/ga/linux/x64/jdk/hotspot/normal/eclipse"
  mkdir jdk && tar -xzf jdk.tar.gz -C jdk --strip-components=1 && rm jdk.tar.gz
fi
export JAVA_HOME=$ROOT/jdk
export PATH=$JAVA_HOME/bin:$PATH

if [ ! -d sdk/cmdline-tools/latest ]; then
  mkdir -p sdk/cmdline-tools
  curl -fL -o cmd.zip "https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip"
  python3 -m zipfile -e cmd.zip sdk/cmdline-tools && rm cmd.zip
  mv sdk/cmdline-tools/cmdline-tools sdk/cmdline-tools/latest
  chmod +x sdk/cmdline-tools/latest/bin/*
fi
export ANDROID_SDK_ROOT=$ROOT/sdk
SDKM=$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager

yes | $SDKM --licenses > /dev/null || true
$SDKM "platform-tools" "platforms;android-37" "build-tools;37.0.0" "ndk;23.2.8568313"
echo TOOLCHAIN_DONE

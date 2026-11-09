set -euo pipefail
source ~/env.sh
S=$ANDROID_SDK_ROOT
$S/cmdline-tools/latest/bin/sdkmanager --sdk_root=$S "cmake;3.22.1" > /dev/null
CMAKE=$S/cmake/3.22.1/bin/cmake
NINJA=$S/cmake/3.22.1/bin/ninja
cd ~ && rm -rf oboe-src && mkdir oboe-src && cd oboe-src
curl -fL -o oboe.tgz https://github.com/google/oboe/archive/refs/tags/1.9.3.tar.gz
tar xzf oboe.tgz --strip-components=1
$CMAKE -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM=$NINJA \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/libs-arm64
$CMAKE --build build -j14
$CMAKE --install build
ls ~/libs-arm64/lib ~/libs-arm64/include | head

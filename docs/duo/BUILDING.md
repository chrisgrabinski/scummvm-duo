# Building the Android app

The Android port of ScummVM is built on Linux (or WSL on Windows). The scripts in
`tools/duo-android-build` set up what is needed under `~/android-toolchain` without root, and
were used to produce the debug APK. They assume Linux x86_64 and download a JDK (Temurin 21),
the Android command line tools, SDK platform 37.0, build-tools 37.0.0, NDK 23.2.8568313 and
CMake 3.22.1, and accept the Android SDK licenses for you; read them first.

```bash
tools/duo-android-build/setup-toolchain.sh     # JDK, SDK, NDK (accepts the SDK licenses)
cp tools/duo-android-build/env.sh ~/env.sh && source ~/env.sh
bash tools/duo-android-build/build-oboe.sh     # Oboe audio library, built from source
```

Configure and build (arm64, SCUMM only; run from an empty build directory):

```bash
mkdir ~/build-arm64 && cd ~/build-arm64
CXXFLAGS="-I$HOME/libs-arm64/include" LDFLAGS="-L$HOME/libs-arm64/lib/arm64-v8a" \
  /path/to/scummvm-duo/configure --host=android-arm64-v8a \
  --disable-all-engines --enable-engine=scumm,scumm-7-8 --disable-detection-full
make -j"$(nproc)" androidtest
```

`scumm-7-8` adds the SCUMM v7 and v8 games (Full Throttle, The Dig, The Curse of Monkey Island);
without it ScummVM says "SCUMM v7-8 support is not compiled in" for them. Humongous Entertainment
games need `he` as well. These games use the normal full-screen layout.

`androidtest` builds `ScummVM-debug.apk` and also installs it on a connected device through
Gradle. Use `make androidrelease` for an unsigned release APK.

Notes:

- If `./gradlew` is "not found", the file has Windows line endings: run
  `sed -i 's/\r$//' dists/android/gradlew`.
- Build from the Linux filesystem (for example `~/scummvm`), not from `/mnt/c`, which is slow.
- More Android build information is in the upstream documentation.

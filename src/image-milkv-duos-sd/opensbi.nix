# SPDX-FileCopyrightText: 2025 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

{
  u-boot,

  fetchFromGitHub,
  fetchpatch,
  pkgsCross,
  stdenvNoCC,
}:

stdenvNoCC.mkDerivation {
  pname = "opensbi";
  version = "2025.01.24";

  srcs = [
    (fetchFromGitHub {
      name = "opensbi";
      owner = "sophgo";
      repo = "opensbi";
      rev = "6766e9cc96df51cde91540f9a15f291bf843b682";
      hash = "sha256-+MrJBGJ2qjOx9CRAXhzQ5Ecc4GH6Ea5nRsAXC7ExvGI=";
    })
    (fetchFromGitHub {
      name = "sophgo-sg200x-debian";
      owner = "Fishwaldo";
      repo = "sophgo-sg200x-debian";
      rev = "aa713be0d4ae3e1098deb9248af279d02a665410";
      hash = "sha256-qANU+bV6VpxCs1u8L0VzdZ4O77tAzN2oifB9RofyZYc=";
    })
  ];
  patches = [
    ./opensbi-patches/0001-fix-build-with-binutils-238.patch
    (fetchpatch {
      url = "https://github.com/riscv-software-src/opensbi/commit/360ab88569201f8d282cf98546b86b8576b6f2ad.patch";
      hash = "sha256-RjroyjFieP/fWsM5aEoY+BSm8pEKVXQrOxjLLFnmeRs=";
    })
  ];
  sourceRoot = "opensbi";

  nativeBuildInputs = [
    pkgsCross.riscv64-musl.stdenv.cc.bintools.bintools
    pkgsCross.riscv64-musl.stdenv.cc.cc
  ];

  postPatch = ''
    for file in ../sophgo-sg200x-debian/configs/common/patches/opensbi/*.patch; do
      patch -p1 <$file
    done
    for file in ../sophgo-sg200x-debian/configs/duos/patches/opensbi/*.patch; do
      patch -p1 <$file
    done
  '';

  buildPhase = ''
    runHook preBuild

    make \
      ''${enableParallelBuilding:+-j''${NIX_BUILD_CORES}} \
      CHIP_ARCH=CV181X \
      CROSS_COMPILE=riscv64-unknown-linux-musl- \
      FW_FDT_PATH=${u-boot}/u-boot.dtb \
      OPENSBI_PATH=$PWD \
      PLATFORM=generic

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    install -Dt $out -m 0644 build/platform/generic/firmware/fw_dynamic.bin

    runHook postInstall
  '';

  enableParallelBuilding = true;
}

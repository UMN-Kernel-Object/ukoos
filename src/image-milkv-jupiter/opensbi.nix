# SPDX-FileCopyrightText: 2025 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

{
  u-boot,

  dtc,
  fetchgit,
  pkgsCross,
  stdenvNoCC,
  python3,
}:

stdenvNoCC.mkDerivation {
  pname = "opensbi";
  version = "2.2.27";

  src = fetchgit {
    url = "https://gitee.com/bianbu-linux/opensbi.git";
    rev = "05479f5228f3fab2a4221fe0745f3703171ace58";
    hash = "sha256-Vd/a1gIP3bBqLpmz5HAU6HyrK5SuFDnI2m1T2DrtkwE=";
  };
  patches = [
    ./opensbi-patches/0001-include-sbi-Fix-compiling-with-C23-enabled-compilers.patch
  ];

  nativeBuildInputs = [
    dtc
    # Upstream says to build with a musl Linux toolchain, using anything else causes u-boot to not validate on boot
    # https://docs.u-boot.org/en/latest/build/gcc.html#building
    pkgsCross.riscv64-musl.stdenv.cc.bintools.bintools
    pkgsCross.riscv64-musl.stdenv.cc.cc
    python3
    u-boot
  ];

  postPatch = ''
    patchShebangs .
  '';

  buildPhase = ''
    runHook preBuild

    make \
      ''${enableParallelBuilding:+-j''${NIX_BUILD_CORES}} \
      ARCH=riscv \
      CROSS_COMPILE=riscv64-unknown-linux-musl- \
      PLATFORM=generic \
      PLATFORM_DEFCONFIG=k1_defconfig

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    install -Dt $out -m 0644 build/platform/generic/firmware/fw_dynamic.itb

    runHook postInstall
  '';

  enableParallelBuilding = true;
}

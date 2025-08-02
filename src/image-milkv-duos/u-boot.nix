{
  storageType ? "sd",

  fetchFromGitHub,
  fetchpatch,
  pkgsCross,
  stdenv,

  bc,
  bison,
  flex,
  python3,
}:

stdenv.mkDerivation {
  pname = "u-boot-${storageType}";
  version = "2025.06.23";

  srcs = [
    (fetchFromGitHub {
      name = "u-boot";
      owner = "sophgo";
      repo = "u-boot-2021.10";
      rev = "99637b2476145fca443d13844ef15ed39aab7bf3";
      hash = "sha256-i2zhG7cK7073rmukdwxyuXtg2InC/iqnds9wE2qH5Vw=";
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
    ./u-boot-patches/0002-cmd-inconsistent-return-type.patch
    ./u-boot-patches/0003-remove-config-in-headers.patch
    ./u-boot-patches/0004-remove-mach-types-from-dwc2-gadget.patch
    ./u-boot-patches/0005-fix-dwc2-gadget-writel.patch
    ./u-boot-patches/0006-dwc2-add-cvitek-cv182x.patch
    (fetchpatch {
      url = "https://github.com/u-boot/u-boot/commit/1dde977518f13824b847e23275001191139bc384.patch";
      hash = "sha256-V0jDpx6O4bFzuaOQejdrRnLiWb5LBTx47T0TZqNtMXk=";
    })
    (fetchpatch {
      url = "https://github.com/u-boot/u-boot/commit/661e2215f8483cc7a77badeab6dcbf6a88cc715c.patch";
      hash = "sha256-RZfMfpPlZZ6YezU/gwYxMugW+TxUNUSixnt59ZRfRUU=";
    })
  ];
  sourceRoot = "u-boot";

  nativeBuildInputs = [
    pkgsCross.riscv64-musl.stdenv.cc.bintools.bintools
    pkgsCross.riscv64-musl.stdenv.cc.cc
    bc
    bison
    flex
    python3
  ];

  postPatch = ''
    for file in ../sophgo-sg200x-debian/configs/common/patches/u-boot/*.patch; do
      patch -p1 <$file
    done
    for file in ../sophgo-sg200x-debian/configs/duos/patches/u-boot/*.patch; do
      patch -p1 <$file
    done

    cp --no-preserve=all ${./defconfig} configs/milkv_duos_defconfig

    cp ../sophgo-sg200x-debian/configs/common/dts/cv181x/* arch/riscv/dts/
    cp ../sophgo-sg200x-debian/configs/common/dts/cv181x_riscv/* arch/riscv/dts/
    cp ../sophgo-sg200x-debian/configs/duos/dts/* arch/riscv/dts/

    cp ../sophgo-sg200x-debian/configs/duos/u-boot/cvi_board_init.c include/cvi_board_init.c
    cp ../sophgo-sg200x-debian/configs/duos/u-boot/cvitek.h include/cvitek.h

    python3 ../sophgo-sg200x-debian/scripts/python/mmap_conv.py \
      --type h \
      ../sophgo-sg200x-debian/configs/duos/memmap.py \
      include/cvi_board_memmap.h
    python3 ../sophgo-sg200x-debian/scripts/python/mkcvipart.py \
      ../sophgo-sg200x-debian/configs/duos/partition_${storageType}.xml \
      include/
    python3 ../sophgo-sg200x-debian/scripts/python/mk_imgHeader.py \
      ../sophgo-sg200x-debian/configs/duos/partition_${storageType}.xml \
      include/
  '';

  configurePhase = ''
    runHook preConfigure

    make \
      ''${enableParallelBuilding:+-j''${NIX_BUILD_CORES}} \
      ARCH=riscv \
      BOARD=cv181x \
      CHIP=cv181x \
      CONFIG_USE_DEFAULT_ENV=y \
      CROSS_COMPILE=riscv64-unknown-linux-musl- \
      CVIBOARD=milkv_duos_${storageType} \
      STORAGE_TYPE=${storageType} \
      milkv_duos_defconfig

    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild

    make \
      ''${enableParallelBuilding:+-j''${NIX_BUILD_CORES}} \
      ARCH=riscv \
      BOARD=cv181x \
      CHIP=cv181x \
      CONFIG_USE_DEFAULT_ENV=y \
      CROSS_COMPILE=riscv64-unknown-linux-musl- \
      CVIBOARD=milkv_duos_${storageType} \
      STORAGE_TYPE=${storageType}

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    install -Dt $out -m 0644 u-boot.bin
    install -Dt $out -m 0644 u-boot.dtb
    install -Dt $out/bin tools/mkenvimage

    runHook postInstall
  '';

  enableParallelBuilding = true;
}

{
  opensbi,
  u-boot,

  fetchFromGitHub,
  pkgsCross,
  stdenvNoCC,

  python3,
}:

stdenvNoCC.mkDerivation (self: {
  pname = "fsbl";
  version = "2025.07.15";

  srcs = [
    (fetchFromGitHub {
      name = "fsbl";
      owner = "sophgo";
      repo = "fsbl";
      rev = "92735b967c951883c478bae07ca171fa758426c6";
      hash = "sha256-TQ7ZplQ5j0oyqh6hxyJYPR+L63dQvew1j63wsB9InUo=";
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
    ./fsbl-patches/0001-fix-xthead-extension-names.patch
  ];
  sourceRoot = "fsbl";

  nativeBuildInputs = [
    pkgsCross.riscv64-musl.stdenv.cc.bintools.bintools
    pkgsCross.riscv64-musl.stdenv.cc.cc
    python3
  ];

  postPatch = ''
    for file in ../sophgo-sg200x-debian/configs/common/patches/fsbl/*.patch; do
      patch -p1 <$file
    done
    for file in ../sophgo-sg200x-debian/configs/duos/patches/fsbl/*.patch; do
      patch -p1 <$file
    done

    python3 ../sophgo-sg200x-debian/scripts/python/mmap_conv.py \
      --type h \
      ../sophgo-sg200x-debian/configs/duos/memmap.py \
      plat/cv181x/include/cvi_board_memmap.h
    printf '\163\000\120\020\157\360\337\377' > blank.bin

    patchShebangs plat/cv181x/chip_conf.py
    patchShebangs plat/cv181x/fiptool.py
  '';

  buildPhase = ''
    runHook preBuild

    make \
      ''${enableParallelBuilding:+-j''${NIX_BUILD_CORES}} \
      ARCH=riscv \
      BLCP_2ND_PATH=blank.bin \
      BOOT_CPU=riscv \
      BUILD_STRING=${self.version} \
      CHIP_ARCH=cv181x \
      CROSS_COMPILE=riscv64-unknown-linux-musl- \
      DDR_CFG=ddr3_1866_x16 \
      LDFLAGS="--no-warn-rwx-segments $LDFLAGS" \
      LOADER_2ND_PATH=${u-boot}/u-boot.bin \
      MONITOR_PATH=${opensbi}/fw_dynamic.bin \
      OD_CLK_SEL=y \
      RTOS_ENABLE_FREERTOS=y

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    install -Dt $out -m 0644 build/cv181x/fip.bin

    runHook postInstall
  '';

  enableParallelBuilding = true;
})

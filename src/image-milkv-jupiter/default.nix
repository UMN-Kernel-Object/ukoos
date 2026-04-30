# SPDX-FileCopyrightText: 2025 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

{
  dev ? false,
  u-boot-spacemit-k1,

  callPackage,
  lib,
  pkgsCross,
  stdenvNoCC,

  dosfstools,
  genimage,
  mtools,
  ukoos-milkv-jupiter,
}:

stdenvNoCC.mkDerivation (self: {
  pname = "image-milkv-jupiter" + (if dev then "-dev" else "");
  inherit (ukoos-milkv-jupiter) version;

  nativeBuildInputs = [
    dosfstools
    genimage
    mtools
    u-boot-spacemit-k1
  ];

  dontUnpack = true;
  buildPhase = ''
        runHook preBuild
    	
        mkdir -p root/boot
        ${lib.optionalString (!dev) ''
          install -Dt root/boot ${ukoos-milkv-jupiter}/sys/kernel.elf
        ''}
        install -Dt input ${u-boot-spacemit-k1}/FSBL.bin
        install -Dt input ${u-boot-spacemit-k1}/u-boot.itb
        install -Dt input ${u-boot-spacemit-k1}/bootinfo_sd.bin
        install -Dt input ${self.passthru.opensbi-milkv-jupiter}/fw_dynamic.itb
        cp ${./u-boot.txt} input/u-boot.txt
        cp ${./u-boot.txt} root/boot/u-boot.txt
        genimage --config ${./genimage.cfg}
        runHook postBuild
  '';
  installPhase = ''
    runHook preInstall

    install -Dt $out -m 0644 images/ukoos.img

    runHook postInstall
  '';

  passthru = {
    opensbi-milkv-jupiter = pkgsCross.riscv64-musl.callPackage ./opensbi.nix {
      u-boot = u-boot-spacemit-k1;
    };
  };
})

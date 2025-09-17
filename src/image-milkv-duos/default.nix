{
  dev ? false,
  u-boot-milkv-duos,

  callPackage,
  lib,
  pkgsCross,
  stdenvNoCC,

  dosfstools,
  genimage,
  mtools,
  ukoos-riscv64,
}:

stdenvNoCC.mkDerivation (self: {
  pname = "image-milkv-duos" + (if dev then "-dev" else "");
  inherit (ukoos-riscv64) version;

  nativeBuildInputs = [
    dosfstools
    genimage
    mtools
    u-boot-milkv-duos
  ];

  dontUnpack = true;
  buildPhase = ''
    runHook preBuild

    ${lib.optionalString (!dev) ''
      install -Dt root/boot ${ukoos-riscv64}/sys/kernel.elf
    ''}
    install -Dt root/boot ${self.passthru.fsbl-milkv-duos}/fip.bin
    mkenvimage -s 0x20000 -o root/boot/uboot.env ${./u-boot.txt}
    genimage --config ${./genimage.cfg}

    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall

    install -Dt $out -m 0644 images/sdcard.img

    runHook postInstall
  '';

  passthru = {
    fsbl-milkv-duos = callPackage ./fsbl.nix {
      inherit (self.passthru) opensbi-milkv-duos;
      inherit u-boot-milkv-duos;
    };
    opensbi-milkv-duos = pkgsCross.riscv64-musl.callPackage ./opensbi.nix {
      inherit u-boot-milkv-duos;
    };
  };
})

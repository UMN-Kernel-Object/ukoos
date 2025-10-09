{
  dev ? false,
  u-boot-milkv-duos-sd,
  ukoos-milkv-duos,

  callPackage,
  lib,
  pkgsCross,
  stdenvNoCC,

  dosfstools,
  genimage,
  mtools,
}:

stdenvNoCC.mkDerivation (self: {
  pname = "image-milkv-duos" + (if dev then "-dev" else "");
  inherit (ukoos-milkv-duos) version;

  nativeBuildInputs = [
    dosfstools
    genimage
    mtools
    u-boot-milkv-duos-sd
  ];

  dontUnpack = true;
  buildPhase = ''
    runHook preBuild

    ${lib.optionalString (!dev) ''
      install -Dt root/boot ${ukoos-milkv-duos}/sys/kernel.elf
    ''}
    install -Dt root/boot ${self.passthru.fsbl}/fip.bin
    mkenvimage -s 0x20000 -o root/boot/uboot.env ${./u-boot.txt}
    genimage --config ${./genimage.cfg}

    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall

    install -Dt $out -m 0644 images/ukoos.img

    runHook postInstall
  '';

  passthru = {
    fsbl = callPackage ./fsbl.nix {
      inherit (self.passthru) opensbi u-boot;
    };
    opensbi = pkgsCross.riscv64-musl.callPackage ./opensbi.nix {
      inherit (self.passthru) u-boot;
    };
    u-boot = u-boot-milkv-duos-sd;
  };
})

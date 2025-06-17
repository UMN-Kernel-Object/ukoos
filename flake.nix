{
  outputs =
    {
      self,
      flake-utils,
      nixpkgs,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      rec {
        devShells.default = pkgs.mkShell {
          inputsFrom = builtins.attrValues packages;
          nativeBuildInputs = [
            pkgs.bear
            pkgs.qemu
          ];
        };

        packages.default = pkgs.stdenvNoCC.mkDerivation {
          pname = "ukoos";
          version = "git-${self.shortRev or self.dirtyShortRev or "unknown"}";

          src = ./.;
          nativeBuildInputs = [
            pkgs.getopt
            pkgs.pkgsCross.riscv64-embedded.stdenv.cc.bintools.bintools
            pkgs.pkgsCross.riscv64-embedded.stdenv.cc.cc
            pkgs.python3
          ];

          dontUnpack = true;
          configurePhase = ''
            runHook preConfigure

            mkdir build
            cd build
            bash $src/configure

            runHook postConfigure
          '';
          installPhase = ''
            runHook preInstall

            make install DESTDIR=$out

            runHook postInstall
          '';
        };
      }
    );
}

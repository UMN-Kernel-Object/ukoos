{
  version,

  mdbook,
  stdenvNoCC,
}:

stdenvNoCC.mkDerivation {
  pname = "ukoos-docs";
  inherit version;
  src = ./.;
  nativeBuildInputs = [ mdbook ];
  buildPhase = ''
    runHook preBuild
    mdbook build
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    mv book $out
    runHook postInstall
  '';
}

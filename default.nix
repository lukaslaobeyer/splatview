{ buildBazelPackage
, bazel_6
, gcc13
, pkg-config
, glfw
}:
buildBazelPackage rec {
  version = "0.0.1";
  name = "splatview-${version}";
  src = ./.;
  bazel = bazel_6;
  nativeBuildInputs = [ gcc13 pkg-config ];
  dontAddBazelOpts = true;
  buildInputs = [ glfw ];
  buildAttrs = {
    installPhase = ''
      install -Dm0755 bazel-bin/viewer/viewer $out/bin/splatview
    '';
  };
  fetchAttrs = {
    sha256 = "sha256-4UnaKD1/YrQbEr17tL6Eff+bQNLwgdkcU6oUh2eovCQ=";
  };
  bazelTargets = [ "//viewer" ];
}

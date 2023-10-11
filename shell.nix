{ pkgs ? import <nixpkgs> {} }:
pkgs.gcc13Stdenv.mkDerivation rec {
  name = "splatview-shell";
  buildInputs = with pkgs; [
    bazel_6
    pkg-config
    glfw

    # debug
    gdb
  ];
}

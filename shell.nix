{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    gcc.man
    gcc.cc.lib
    glibc
    glibc.dev
    glibc.static
    gnumake
    man-pages
  ];
    
  shellHook = ''
    echo "Shell initialized!"
  '';
}
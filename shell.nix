{ pkgs ? import <nixpkgs> { } }:
with pkgs;
mkShell {
  packages = [
    git

    arduino-cli
  ];

  ARDUINO_CONFIG_FILE = ./arduino-cli.yaml;
}


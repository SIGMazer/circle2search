{
  description = "Circle2Search - Wayland/X11 Screen Search";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      # Package definition for installation
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "circle2search";
        version = "1.0";
        src = ./.;

        nativeBuildInputs = with pkgs; [ pkg-config makeWrapper ];
        buildInputs = with pkgs; [
          gtk3
          tesseract
          leptonica
          libx11
          grim # Essential for Wayland
          python3
        ];

        # Configure build
        buildPhase = ''
          make
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp build/circle2search $out/bin/
          
          # Wrap the binary to ensure dependencies are in PATH at runtime
          wrapProgram $out/bin/circle2search \
            --prefix PATH : ${pkgs.grim}/bin:${pkgs.python3}/bin
        '';
      };
    };
}

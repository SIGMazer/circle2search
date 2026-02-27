# Circle2Search

Desktop screen capture tool with smart text/image detection, instant search, and translation.

**Index:**
- [Installation](#installation)
  - [general](#install-dependencies)
  - [nixos](#for-nixos)
- [Usage](#usage)

**Supports both X11 and Wayland!**

## Installation

### Install Dependencies

```bash
chmod +x install_deps.sh
./install_deps.sh
```

#### Build

```bash
make
```

#### Run

```bash
./build/circle2search
```

### For NixOS
Terminal session (for test)
```sh
git clone https://github.com/SIGMazer/circle2search.git
cd circle2search
nix develop
make
./build/circle2search
```
System-Wide

In `/etc/nixos/configuration.nix` file
```nix
{ pkgs, ... }: # your args
let
  # Flake file location
  circle2search = builtins.getFlake "github:SIGMazer/circle2search/8a4576642efb926d2bf6e4d0c0a325a83439e8c8";
in {
  # System packages
  environment.systemPackages = with pkgs; [ 
    circle2search.packages.x86_64-linux.default
  ];

  # OR User packages
  users.users.username = {
    packages = with pkgs; [
      circle2search.packages.x86_64-linux.default
    ];
  };
}

```

## Usage

1. **Select** - Click and drag to select any screen area
2. **Auto-detect** - Text or image is detected automatically
3. **Translate** (text only) - Click translate to open Google Translate with the selected text
4. **Search** - Click the search button or press ESC to cancel

### Tips

- Normal drag: Auto-detects text or image
- Ctrl + drag: Force image search mode
- **Ctrl + C**: Copy detected text to clipboard (text mode only)
- ESC: Exit without searching

### Display Server Support

- **X11**: Native support using X11 screen capture
- **Wayland**: Requires `grim` for screen capture (installed via install_deps.sh)

#!/bin/bash

echo "Installing dependencies for Circle2Search..."

if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Cannot determine Linux distribution"
    exit 1
fi

case $OS in
    ubuntu|debian|linuxmint|mint)
        echo "Detected Ubuntu/Debian/Mint"
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            libgtk-3-dev \
            libtesseract-dev \
            libleptonica-dev \
            libx11-dev \
            libcairo2-dev \
            tesseract-ocr \
            tesseract-ocr-eng \
            tesseract-ocr-ara \
            python3 \
            openssh-client \
            grim \
            pkg-config
        ;;
    fedora|rhel|centos)
        echo "Detected Fedora/RHEL/CentOS"
        sudo dnf install -y \
            gcc \
            make \
            gtk3-devel \
            tesseract-devel \
            leptonica-devel \
            libX11-devel \
            cairo-devel \
            tesseract \
            tesseract-langpack-eng \
            tesseract-langpack-ara \
            python3 \
            openssh-clients \
            grim \
            pkg-config
        ;;
    arch|manjaro)
        echo "Detected Arch/Manjaro"
        sudo pacman -S --noconfirm \
            base-devel \
            gtk3 \
            tesseract \
            tesseract-data-eng \
            tesseract-data-ara \
            leptonica \
            libx11 \
            cairo \
            python \
            openssh \
            grim \
            pkg-config
        ;;
    *)
        echo "Unsupported distribution: $OS"
        echo "Please install manually:"
        echo "  - GTK3 + Cairo development files"
        echo "  - Tesseract OCR + development files"
        echo "  - Leptonica development files"
        echo "  - X11 development files"
        echo "  - Python 3"
        echo "  - OpenSSH client"
        echo "  - GCC and Make"
        exit 1
        ;;
esac

echo ""
echo "Dependencies installed successfully!"
echo "Run 'make' to build the project."

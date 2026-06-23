#!/bin/bash
# VioletOS build script
set -e

cd "$(dirname "$0")"

echo "=== VioletOS Build ==="

if ! command -v nasm &>/dev/null; then
    echo "Error: nasm not found. Install with: sudo dnf install nasm"
    exit 1
fi

if ! command -v gcc &>/dev/null; then
    echo "Error: gcc not found."
    exit 1
fi

make clean 2>/dev/null || true
make iso

echo ""
echo "Build complete!"
echo "  ISO: build/violetos.iso"
echo ""
echo "Run in QEMU:"
echo "  make run"
echo ""
echo "Or manually:"
echo "  qemu-system-i386 -cdrom build/violetos.iso -m 128M"

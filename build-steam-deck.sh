#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

if ! command -v make >/dev/null || ! command -v cc >/dev/null; then
  echo "A Linux C compiler and GNU make are required. Build on a Linux PC or in a Linux development container." >&2
  exit 1
fi

make -f Makefile.libretro platform=unix CC=cc -j"$(nproc)"

if ! file fuse_libretro.so | grep -q 'ELF 64-bit.*x86-64'; then
  echo "Build finished, but fuse_libretro.so is not an x86_64 Linux ELF library." >&2
  exit 1
fi

echo "Built: $PWD/fuse_libretro.so"

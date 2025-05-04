#!/usr/bin/env bash
set -euo pipefail

# Build script for ThermalCamera library (static & shared)

# Determine project root (where this script lives)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 1. Clean and recreate the build directory
BUILD_DIR="${ROOT_DIR}/build"
echo "→ Cleaning build directory..."
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# 2. Configure CMake for a Release build
echo "→ Configuring CMake (Release)..."
cmake "${ROOT_DIR}" -DCMAKE_BUILD_TYPE=Release

# 3. Build both the static and shared targets
echo "→ Building static & shared libraries..."
cmake --build . --parallel --target ThermalCamera_static ThermalCamera_shared

# 4. Report results
echo "  Build complete."
echo "  Static lib located at: $(find . -type f -name 'libThermalCamera_static.*')"
echo "  Shared lib located at: $(find . -type f -name 'libThermalCamera_shared.*')"

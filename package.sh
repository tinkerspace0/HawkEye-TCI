#!/usr/bin/env bash
set -euo pipefail

# 1. Variables
BUILD_DIR=build
RESULT_DIR=result

# 2. Clean up
rm -rf "${BUILD_DIR}" "${RESULT_DIR}"
mkdir -p "${BUILD_DIR}" "${RESULT_DIR}"

# 3. Configure & build
cd "${BUILD_DIR}"
cmake .. -DCMAKE_BUILD_TYPE=Release
make thermal_test   # or your umbrella lib target if you created one
cd ..

# 4. Copy the executable (and any umbrella .so you built) to result/
cp "${BUILD_DIR}/thermal_test" "${RESULT_DIR}/"
# If you built libHawkEyeTCI.so, copy that too:
# cp "${BUILD_DIR}/libHawkEyeTCI.so" "${RESULT_DIR}/"

# 5. Copy *all* vendor .so into result/
cp i3system/lib/libi3system_*.so*     "${RESULT_DIR}/"

# 6. Patch RPATH on every ELF in result/
pushd "${RESULT_DIR}" >/dev/null
for BIN in thermal_test libHawkEyeTCI.so libi3system_*.so*; do
  if file "$BIN" | grep -q 'ELF'; then
    patchelf --set-rpath '$ORIGIN' "$BIN"
  fi
done
popd >/dev/null

# 7. Show result
echo "Packaged into ${RESULT_DIR}:"
ls -1 "${RESULT_DIR}"

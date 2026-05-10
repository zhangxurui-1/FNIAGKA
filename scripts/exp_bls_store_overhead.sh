#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN=${ROOT}/build/NIAGKA/main
if [ "$#" -ge 1 ]; then
    OUT_DIR="$(cd "$1" && pwd)"
    echo "Using provided directory: ${OUT_DIR}"
else
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    OUT_DIR="${ROOT}/log/log_${TIMESTAMP}"
    echo "No directory provided, creating new directory: ${OUT_DIR}"
fi

mkdir -p "${OUT_DIR}"

OUT_FILE=${OUT_DIR}/exp_storage_bls_security256.log
FIGURE_FILE=${ROOT}/figures/store_overhead_bls_security256.pdf
MEASURE_GROUP_SIZE=10
PLOT_MAX_GROUP_SIZE=500

${BIN} 256 "${MEASURE_GROUP_SIZE}" test_store_overhead > "${OUT_FILE}"
python3 "${ROOT}/scripts/draw_store_overhead_bls_security256.py" \
    --log "${OUT_FILE}" \
    --max-group-size "${PLOT_MAX_GROUP_SIZE}" \
    --save "${FIGURE_FILE}"

echo "Storage log written to ${OUT_FILE}"
echo "Figure written to ${FIGURE_FILE}"

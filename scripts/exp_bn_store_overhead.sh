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

MEASURE_GROUP_SIZE=10
PLOT_MAX_GROUP_SIZE=500
DRAW_SCRIPT="${ROOT}/scripts/draw_store_overhead_bn.py"
OUT_128=${OUT_DIR}/exp_storage_bn_security128.log
OUT_192=${OUT_DIR}/exp_storage_bn_security192.log
FIGURE_FILE=${ROOT}/figures/store_overhead_bn.pdf

${BIN} 128 "${MEASURE_GROUP_SIZE}" test_store_overhead > "${OUT_128}"
${BIN} 192 "${MEASURE_GROUP_SIZE}" test_store_overhead > "${OUT_192}"

python3 "${DRAW_SCRIPT}" \
    --series 128 "${OUT_128}" \
    --series 192 "${OUT_192}" \
    --max-group-size "${PLOT_MAX_GROUP_SIZE}" \
    --save "${FIGURE_FILE}" \
    --step 50

echo "Storage logs written to ${OUT_128} and ${OUT_192}"
echo "Figure written to ${FIGURE_FILE}"

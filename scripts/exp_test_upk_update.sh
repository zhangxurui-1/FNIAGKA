#!/usr/bin/env bash

set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN=${ROOT}/build/NIAGKA/main
TOOLS_BIN=${ROOT}/tools
TRIM_BIN=${TOOLS_BIN}/trim
AGG_BIN=${TOOLS_BIN}/aggregate
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUT_DIR="${ROOT}/log/log_test_upk_update_${TIMESTAMP}"

mkdir -p "${OUT_DIR}"
rm -f ${OUT_DIR}/exp_*.log

for SECURITY_LEVEL in 80 128; do
    for ((i=1; i<=5; i++)); do
        MAX_GROUP_SIZE=$((10 * i))
        OUT_FILE=${OUT_DIR}/exp_security${SECURITY_LEVEL}_size${MAX_GROUP_SIZE}.log

        echo "Running Experiment ${i}, maxGroupSize=${MAX_GROUP_SIZE}, securityLevel=${SECURITY_LEVEL}"

        ${BIN} ${SECURITY_LEVEL} ${MAX_GROUP_SIZE} test_upk_update | ${TRIM_BIN} "${OUT_FILE}"
        
        echo "Experiment ${i} done"
    done

done

"$AGG_BIN" "${OUT_DIR}"

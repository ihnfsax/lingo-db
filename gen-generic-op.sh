#!/bin/bash
set -e

if [ -z "$1" ]
  then
    echo "Usage: ./gen-generic-op.sh <sql_file>"
    exit 1
fi

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
export PATH="${PATH:+${PATH}:}${SCRIPTPATH}/build"

cd ${SCRIPTPATH}
LINGODB_EXECUTION_MODE=DEBUGGING run-sql resources/sql/temp/$1 resources/data/uni
mlir-db-opt -mlir-print-op-generic snapshot-0.mlir > snapshot-0-generic.mlir
mlir-db-opt -mlir-print-op-generic snapshot-1.mlir > snapshot-1-generic.mlir
mlir-db-opt -mlir-print-op-generic snapshot-2.mlir > snapshot-2-generic.mlir
mlir-db-opt -mlir-print-op-generic snapshot-3.mlir > snapshot-3-generic.mlir
mlir-db-opt -mlir-print-op-generic snapshot-4.mlir > snapshot-4-generic.mlir
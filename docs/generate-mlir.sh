#!/bin/bash

source_dir="../resources/sql/tpch"
destination_dir="../resources/mlir/tpch"
metadata_path="../resources/data/tpch/metadata.json"

# Create the destination directory if it doesn't exist
mkdir -p "$destination_dir"

# Iterate over the .sql files in the source directory
for sql_file in "$source_dir"/*.sql; do
  # Extract the file name without extension
  file_name=$(basename "$sql_file" .sql)

  # Generate the corresponding .mlir file path in the destination directory
  mlir_file="$destination_dir/$file_name.mlir"

  echo "sql-to-mlir $sql_file $metadata_path > $mlir_file"

  # Run the 'opt' tool to convert the SQL file to MLIR
  sql-to-mlir "$sql_file" "$metadata_path" > "$mlir_file"
done

#!/bin/bash

DIRECTORY="./build/target/bin/MinSizeRel"

find "$DIRECTORY" -type f | while read -r file; do
    echo "Stripping: $file"
    llvm-strip --strip-all "$file" 2>/dev/null || echo "Failed to strip: $file"
done

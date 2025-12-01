#!/bin/sh

# POSIX 2024 compliant shell script

INPUT_DIR="$1"
OUTPUT_DIR="$2"

if ! [ "$INPUT_DIR" ]; then
    echo "No input directory specified!" >&2
    exit 1
fi

if ! [ "$OUTPUT_DIR" ]; then
    echo "No output directory specified!" >&2
    exit 1
fi

if ! [ -d "$INPUT_DIR" ]; then
    echo "Input directory does not exist: $INPUT_DIR" >&2
    exit 1
fi

if ! [ -d "$OUTPUT_DIR" ]; then
    echo "Output directory does not exist: $OUTPUT_DIR" >&2
    exit 1
fi

# Find .ova (VMWare) files in input directory (non-recursive)
find "$INPUT_DIR" -maxdepth 1 -type f -iname '*.ova' -print0 | while IFS= read -r -d '' src; do
    echo "Extracting: $src"
    if ! tar -xf "$src" -C "$INPUT_DIR"; then
        echo "Failed to extract: $src" >&2
        exit 1
    fi
done

# Find .vdi (VirtualBox) and .vmdk (VMWare) files in input directory (non-recursive)
find "$INPUT_DIR" -maxdepth 1 -type f \( -iname '*.vmdk' -o -iname '*.vdi' \) -print0 | while IFS= read -r -d '' src; do
    basename="$(basename "$src")"
    name=${basename%.*}
    dst="$OUTPUT_DIR/$name.img"

    echo "Converting: $src -> $dst"
    if ! qemu-img convert -O raw "$src" "$dst"; then
        echo "Failed to convert: $src" >&2
        exit 1
    fi
done

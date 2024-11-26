#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input file path> <output file path>"
    exit 1
fi

# Get input and output file paths
input_file="$1"
output_file="$2"

# Use ffmpeg to convert the image to YUV420P format JPEG
ffmpeg -i "$input_file" -pix_fmt yuvj420p "$output_file"

# Check if the conversion was successful
if [ $? -eq 0 ]; then
    echo "The file has been successfully converted to YUV420P JPEG and saved to $output_file"
else
    echo "Conversion failed. Please check the input file path and ensure ffmpeg is installed."
    exit 1
fi

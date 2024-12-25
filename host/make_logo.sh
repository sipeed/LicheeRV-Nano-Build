#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -eq 2 ]; then
    # Get input and output file paths
    input_file="$1"
    output_file="$2"

    # Use ffmpeg to convert the image to YUV420P format JPEG
    ffmpeg -i "$input_file" -pix_fmt yuvj420p "$output_file"
elif [ "$#" -eq 4 ]; then
    # Get input and output file paths
    input_file="$1"
    output_file="$2"
    output_width="$3"
    output_height="$4"

    # Use ffmpeg to convert the image to YUV420P format JPEG
    ffmpeg -i "$input_file" -vf "scale=$output_width:$output_height" -pix_fmt yuvj420p "$output_file"
elif [ "$#" -eq 5 ]; then
    # Get input and output file paths
    input_file="$1"
    output_file="$2"
    output_width="$3"
    output_height="$4"
    transpose="$5"

    # Use ffmpeg to convert the image to YUV420P format JPEG
    ffmpeg -i "$input_file" -vf "transpose=$transpose,scale=$output_width:$output_height" -pix_fmt yuvj420p "$output_file"
else
    echo "Usage: $0 <input file path> <output file path> <output width> <output height> <transpose>"
    echo "example: $0 input.jpg logo.jpeg"
    echo "example: $0 input.jpg logo.jpeg 640 480"
    echo "example: $0 input.jpg logo.jpeg 640 480 3"
    exit 1
fi

# Check if the conversion was successful
if [ $? -eq 0 ]; then
    echo "The file has been successfully converted to YUV420P JPEG and saved to $output_file"
else
    echo "Conversion failed. Please check the input file path and ensure ffmpeg is installed."
    exit 1
fi

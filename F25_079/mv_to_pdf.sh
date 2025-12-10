#!/bin/bash
# Ensure the target pdf folder exists (outside F25_079)
mkdir -p ../pdf

# Move all main.* files except main.tex into the pdf folder
shopt -s extglob
mv main.@(!(tex)) ../pdf/ 2>/dev/null

# Change directory into the pdf folder
cd ../pdf || exit

# Print current directory
pwd

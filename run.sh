#!/bin/bash

# Check if we're on macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS system"
    
    # Check if libomp is installed via Homebrew
    if [ ! -d "/opt/homebrew/opt/libomp" ]; then
        echo "libomp not found. Installing via Homebrew..."
        brew install libomp
    fi
    
    # Compile with macOS-specific OpenMP flags
    echo "Compiling with macOS OpenMP support..."
    g++ -std=c++17 -I./eigen -Xclang -fopenmp -isystem/opt/homebrew/opt/libomp/include -L/opt/homebrew/opt/libomp/lib -lomp -o main main.cpp
else
    # For non-macOS systems, try standard OpenMP compilation
    echo "Attempting standard OpenMP compilation..."
    g++ -std=c++17 -I./eigen -fopenmp -o main main.cpp
fi

# Run the program if compilation was successful
if [ $? -eq 0 ]; then
    echo "Running program..."
    ./main
    
    if [ $? -eq 0 ]; then
        echo "Program executed successfully. Check ground_scene.ppm"
    else
        echo "Program failed to execute"
    fi
else
    echo "Compilation failed. Please check the error messages above."
    echo "If you're on macOS, make sure you have Homebrew installed and run:"
    echo "brew install libomp"
fi 
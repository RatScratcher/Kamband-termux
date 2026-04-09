#!/bin/bash
# Simple script to attempt to test the angband binary

if [ -f angband ]; then
    echo "Running basic start test for angband..."
    ./angband --version || echo "Warning: '--version' not supported or crashed"
    ./angband -h || echo "Warning: '-h' not supported or crashed"
    echo "Tests complete."
else
    echo "angband binary not found. Please compile it first."
fi

#!/bin/bash

#-------------------------------------------------------------------------
# Run this script in the root directory of profanity projects via 
#	./generate-docs.sh
# This file will generate the profanity's documentation:
# 
# * Doxygen Developer Documentation in folder docs/html/
# * manpages in folder docs/*
# * C API for profanity's plugin API in apidocs/c/
#   
#-------------------------------------------------------------------------

echo "Generating developer documentation..."
doxygen docs/Doxyfile

echo "Generating manpages..."
./profanity mangen

echo "Generating C API documentation..."
./apidocs/c/gen.sh 


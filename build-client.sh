#!/bin/bash

echo "building ftpserver:"

g++ -g -I./json/include/ client.cpp  -o client

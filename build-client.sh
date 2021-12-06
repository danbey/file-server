#!/bin/bash

echo "building ftpclient:"

g++ -g -I./json/include/ client.cpp  -o client

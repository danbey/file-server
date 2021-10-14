#!/bin/bash

echo "building ftpserver:"

g++ -g -I./json/include/ servercore.cpp serverconnection.cpp fileoperator.cpp ftpserver.cpp -o ftpserver

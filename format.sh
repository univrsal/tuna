#!/bin/bash
find ./src -iname *.h* -o -iname *.c* | xargs clang-format -style=WebKit -i -verbose

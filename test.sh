#!/usr/bin/bash
set -e

./build/main ./examples/main.em
# qbe -o out.s build/ir.ssa
# cc out.s -o out
# ./out
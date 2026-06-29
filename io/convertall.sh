#!/usr/bin/env bash

cd ..
for f in ./io/bin/tech/hog/*; do
  name=$(basename "${f%.*}")
  echo $name
  ./build/april $f --output=./io/out/$name.luau || exit 1
done

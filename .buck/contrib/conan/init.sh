#!/bin/bash

set -e

while ! [ -d '.buck' ] && ! [ -f 'conanfile.txt' ]
do
  if [ '$PWD' == '/' ]
  then
    echo "This script should be run from the directory inside https://github.com/VestniK/wayland-test/ clone"
    exit 1
  fi
  cd ..
done

conan install . --deploy=full_deploy -of .buck/contrib/conan/import

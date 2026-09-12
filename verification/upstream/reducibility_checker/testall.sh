#!/bin/bash
set -euxo pipefail

mkdir -p log

FILES="./projective_configurations/reducible/dconf/*.dconf"

for filePath in ${FILES} ; do
    echo "Start testing ${filePath}"
    fileName=`basename ${filePath%.*}`
    fileBase=${fileName%.*}
    ./build/a.out -i $filePath $@ > "log/${fileBase}.log"
done
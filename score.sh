#!/bin/bash
timestamp=$1
seed=$2
if java -jar tester.jar -exec ./log/$timestamp.bin -debug -novis -seed $seed > log/$timestamp.$seed.log ; then
    cat log/$timestamp.$seed.log \
        | tee /dev/stderr \
        | grep '{"seed":'
else
    cat log/$timestamp.$seed.log > /dev/stderr
    echo "{\"seed\":$seed}"
fi \
    > log/$timestamp.$seed.json
flock log/$timestamp.lock sh -c "cat log/$timestamp.$seed.json >> log/$timestamp.json"

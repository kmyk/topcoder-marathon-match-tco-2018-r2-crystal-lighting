#!/bin/bash
timestamp=$1
seed=$2
if java -jar tester.jar -exec ./log/$timestamp.bin -debug -novis -timeout 20 -seed $seed > log/$timestamp.$seed.log ; then
    cat log/$timestamp.$seed.log \
        | tee /dev/stderr \
        | grep '{"seed":'
else
    cat log/$timestamp.$seed.log > /dev/stderr
    error=$(grep '[FATAL]' log/$timestamp.$seed.log)
    echo "{\"seed\":$seed,\"error\":\"$error\"}"
fi \
    > log/$timestamp.$seed.json
flock log/$timestamp.lock sh -c "cat log/$timestamp.$seed.json >> log/$timestamp.json"
rm log/$timestamp.$seed.json

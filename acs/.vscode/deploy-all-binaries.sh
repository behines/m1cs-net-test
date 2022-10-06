#!/bin/bash
readonly TARGET_IP="$1"
readonly TARGET_DIR="$2" #"/home/root/bin"
readonly TARGET_USERNAME="$3"
readonly LOCAL_BIN_DIR="$4" # "../bin"

# Must match startsPattern in tasks.json
echo "Deploying to target ${TARGET_IP}"
pwd

# Send all compiled binaries to the target
scp `ls -d -p ${LOCAL_BIN_DIR}/** | grep -v -E '\.([^./])' | grep -v '/$'` ${TARGET_USERNAME}@${TARGET_IP}:${TARGET_DIR}

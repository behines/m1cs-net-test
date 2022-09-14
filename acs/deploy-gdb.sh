#!/bin/bash
readonly TARGET_IP="$1"
readonly PROGRAM="$2"
readonly PROGRAM_ARGS="$3"
readonly TARGET_DIR="$4" #"/home/root/bin"
readonly TARGET_USERNAME="$5"
readonly LOCAL_BIN_DIR="$6" # "../bin"

# Must match startsPattern in tasks.json
echo "Deploying to target ${TARGET_IP}"

# kill gdbserver on target and delete old binary
ssh ${TARGET_USERNAME}@${TARGET_IP} "sh -c '/usr/bin/killall -q gdbserver; rm -rf ${TARGET_DIR}/${PROGRAM}  exit 0'"

pwd

# Choose whichever of these two lines you prefer.
# Send either the program, or all compiled binaries to the target
scp ${LOCAL_BIN_DIR}/${PROGRAM} ${TARGET_USERNAME}@${TARGET_IP}:${TARGET_DIR}
#scp ${LOCAL_BIN_DIR}/* root@${TARGET_IP}:${TARGET_DIR}

echo "Starting GDB Server on Target"

# start gdbserver on target
ssh -t ${TARGET_USERNAME}@${TARGET_IP} "sh -c 'cd ${TARGET_DIR}; gdbserver localhost:3000 ${PROGRAM} ${PROGRAM_ARGS}'"

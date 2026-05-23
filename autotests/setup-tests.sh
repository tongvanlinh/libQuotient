#!/bin/bash

# OCI CLI flavour (e.g. docker or podman). Override if your `docker` is not the desired CLI.
CONTAINER_CLI="${CONTAINER_CLI:-docker}"

# 1.95.1 is the latest release that allows user registration over HTTPS with self-signed certs
# The setup has to be changed to support newer versions
SYNAPSE_REF=${SYNAPSE_REF:-v1.95.1}
SYNAPSE_IMAGE="matrixdotorg/synapse:$SYNAPSE_REF"
SCRIPT_DIR="$PWD/autotests"

# curl loop: max attempts and sleep between tries (total ~ attempts * sleep seconds)
SYNAPSE_WAIT_ATTEMPTS="${SYNAPSE_WAIT_ATTEMPTS:-60}"
SYNAPSE_WAIT_SLEEP="${SYNAPSE_WAIT_SLEEP:-2}"

if [ ! -f $SCRIPT_DIR/adjust-config.sh ]; then
    echo "This script should be run from the directory above autotests/"
    echo "(i.e. autotests/setup-tests.sh). Other ways of invocation are not supported."
    return 1
fi

DATA_PATH="$SCRIPT_DIR/synapse-data"
if [ ! -d "$DATA_PATH" ]; then
    mkdir -p -- "$DATA_PATH"
    chmod 0777 -- "$DATA_PATH"
else
    rm -rf $DATA_PATH/*
fi

echo "Generating the configuration"
if ! "$CONTAINER_CLI" run -v "$DATA_PATH:/data:z" --rm \
    -e SYNAPSE_SERVER_NAME=localhost -e SYNAPSE_REPORT_STATS=no "$SYNAPSE_IMAGE" generate
then
    echo "Synapse config generation failed." >&2
    return 1
fi

echo "Adjusting the configuration and preparing the data directory"
# Run inside the image so bind-mounted files keep container ownership; host user need not write them.
# :ro,z relabels for SELinux (e.g. Fedora) so the container can read scripts from the checkout.
if ! "$CONTAINER_CLI" run --rm \
    --entrypoint /bin/bash \
    -v "$DATA_PATH:/data:z" \
    -v "$SCRIPT_DIR/adjust-config.sh:/tmp/adjust-config.sh:ro,z" \
    -v "$SCRIPT_DIR/register-users.sh:/tmp/register-users.sh:ro,z" \
    -e REGISTER_USERS_PATH=/tmp/register-users.sh \
    "$SYNAPSE_IMAGE" \
    -lc 'set -e; cd /data && . /tmp/adjust-config.sh'
then
    echo "Adjusting Synapse configuration failed." >&2
    return 1
fi

echo "Starting Synapse"
if ! "$CONTAINER_CLI" run -d \
    --name synapse \
    -p 1234:8008 \
    -p 8448:8008 \
    -p 8008:8008 \
    -v "$DATA_PATH:/data:z" "$SYNAPSE_IMAGE"
then
    echo "Starting Synapse container failed." >&2
    return 1
fi

if [ -z "$KEEP_SYNAPSE" ]; then
    TRAP_CMD="\"$CONTAINER_CLI\" rm -f synapse 2>&1 >/dev/null"
    if [ -z "$KEEP_DATA_PATH" ]; then
        TRAP_CMD="$TRAP_CMD; rm -rf $DATA_PATH"
    fi
    trap "$TRAP_CMD; trap - EXIT" EXIT
fi

printf "Waiting for synapse to start "
synapse_ready=0
n=0
while [ "$n" -lt "$SYNAPSE_WAIT_ATTEMPTS" ]; do
    if curl -s -f -k https://localhost:1234/_matrix/client/versions >/dev/null; then
        synapse_ready=1
        echo
        break
    fi
    printf "."
    n=$((n + 1))
    sleep "$SYNAPSE_WAIT_SLEEP"
done

if [ "$synapse_ready" -ne 1 ]; then
    echo
    echo "Timed out waiting for Synapse at https://localhost:1234 (after ~ $((SYNAPSE_WAIT_ATTEMPTS * SYNAPSE_WAIT_SLEEP))s)." >&2
    echo "See logs: $CONTAINER_CLI logs synapse" >&2
    "$CONTAINER_CLI" logs synapse 2>&1 | tail -n 40 >&2 || true
    return 1
fi

if "$CONTAINER_CLI" exec synapse /bin/sh /data/register-users.sh; then
    echo "You can run ctest with a full set of tests now!"
    echo "If you don't find the synapse container running, make sure to source"
    echo "this script instead of running it in a subshell (the container will be"
    echo "deleted when you exit the shell then), or run it with KEEP_SYNAPSE"
    echo "environment variable set to any value"
fi

#!/bin/bash

# Globals
CLIENTPATH="./client"
TARGETNAME="oob-spi-v3"
BACKUP=0

# Colored echos
function success() {
    B=$(tput bold)
    G=$(tput setaf 2)
    RESET=$(tput sgr0)
    echo -e "$B$G$1$RESET"
}

function fail() {
    B=$(tput bold)
    R=$(tput setaf 1)
    RESET=$(tput sgr0)
    echo -e "$B$R$1$RESET"
}

function info() {
    B=$(tput bold)
    Y=$(tput setaf 3)
    RESET=$(tput sgr0)
    echo -e "$B$Y$1$RESET"
}

# Parse arguments
if [ "$#" -ne 2 ]; then
    fail "Illegal number of parameters"
    info "./update-client.sh -p <client folder> [-b <1|0>]"
    exit 1
fi

for i in "$@"; do
    case $i in
    -p=* | --path=*)
        CLIENTPATH="${i#*=}"
        shift
        ;;
    --default)
        DEFAULT=YES
        shift
        ;;
    *) ;;

    esac
done

# Create hosts file
rm .hosts
echo "root@192.168.1.100" >> .hosts
echo "root@192.168.1.101" >> .hosts
echo "root@192.168.1.102" >> .hosts
echo "root@192.168.1.103" >> .hosts

info "Stop service"
echo
parallel-ssh -h .hosts "systemctl stop encoder"

if [ $? -ne 0 ]; then
    fail "Service stopped failed"
    exit 1
fi

success "Service stopped"

# Copy the client folder
info "Copying the new client source code"
echo
parallel-scp -h .hosts -r $CLIENTPATH /tmp

if [ $? -ne 0 ]; then
    fail "Copy failed"
    exit 1
fi

success "Copied"

# Compile the client
info "Compiling the new client"
echo
parallel-ssh -h .hosts "cd /tmp/client && cmake . && make"

if [ $? -ne 0 ]; then
    fail "Compile failed"
    exit 1
fi

success "Compiled"

# Copy to /usr/local/bin
info "Copying the new client"
echo
parallel-ssh -h .hosts "cp /tmp/client/client /usr/local/bin/$TARGETNAME"

if [ $? -ne 0 ]; then
    fail "Copy failed"
    exit 1
fi

success "Copied"

# Restart service
info "Start service"
echo
parallel-ssh -h .hosts "systemctl start encoder"

if [ $? -ne 0 ]; then
    fail "Service started failed"
    exit 1
fi

success "Service started"

# Remove artifacts
info "Removing build directory"
echo
parallel-ssh -h .hosts "rm -rf /tmp/client"

if [ $? -ne 0 ]; then
    fail "Remove failed"
    exit 1
fi

success "Removed"

# Remove hosts file
rm .hosts

echo
success "All clients have been updated"

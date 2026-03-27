#!/usr/bin/env bash

# Exhaustive build configuration matrix testing for Profanity

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_content() {
    echo
    if [ -f "$1" ]; then
        echo "Content of $1:"
        cat "$1"
    else
        echo "Log file $1 not found."
    fi
}

error_handler() {
    ERR_CODE=$?
    echo -e "${RED}Error ${ERR_CODE} with command '${BASH_COMMAND}' on line ${BASH_LINENO[0]}. Exiting.${NC}"

    for log in build_run/meson-logs/testlog.txt build_valgrind/meson-logs/testlog.txt; do
        if [ -f "$log" ]; then
            echo "--- Meson Test Log ($log) ---"
            cat "$log"
        fi
    done
    exit ${ERR_CODE}
}

trap error_handler ERR

num_cores() {
    nproc \
        || sysctl -n hw.ncpu \
        || getconf _NPROCESSORS_ONLN 2>/dev/null \
        || echo 2
}

usage() {
    echo "Usage: $0 [extra-args]"
    echo ""
    echo "Run exhaustive build matrix tests."
    echo ""
    echo "Extra arguments are passed directly to: meson setup [extra-args]"
    exit 0
}

if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    usage
fi

# Compatibility with old script that took autotools|meson as first arg
if [[ "$1" == "meson" ]]; then
    shift
elif [[ "$1" == "autotools" ]]; then
    echo -e "${RED}Error: Autotools is no longer supported.${NC}"
    exit 1
fi

ARCH="$(uname | tr '[:upper:]' '[:lower:]')"
EXTRA_ARGS="$*"

# Build Matrix
echo -e "${YELLOW}---> Starting build matrix...${NC}"

tests=(
    "-Dnotifications=enabled -Dicons-and-clipboard=enabled -Dotr=enabled -Dpgp=enabled -Domemo=enabled -Dc-plugins=enabled -Dpython-plugins=enabled -Dxscreensaver=enabled -Domemo-qrcode=enabled -Dgdk-pixbuf=enabled"
    ""
    "-Dnotifications=disabled"
    "-Dicons-and-clipboard=disabled"
    "-Dotr=disabled"
    "-Dpgp=disabled"
    "-Domemo=disabled -Domemo-qrcode=disabled"
    "-Dpgp=disabled -Dotr=disabled"
    "-Dpgp=disabled -Dotr=disabled -Domemo=disabled"
    "-Dpython-plugins=disabled"
    "-Dc-plugins=disabled"
    "-Dc-plugins=disabled -Dpython-plugins=disabled"
    "-Dxscreensaver=disabled"
    "-Dgdk-pixbuf=disabled"
)

BACKEND_OPT=""
if [ -n "${OMEMO_BACKEND}" ]; then
    BACKEND_OPT="-Domemo-backend=${OMEMO_BACKEND}"
fi

# Valgrind check (Linux only)
if [[ "$ARCH" == linux* ]]; then
    echo -e "${YELLOW}--> Running Valgrind check with full features ${BACKEND_OPT} ${EXTRA_ARGS}${NC}"
    rm -rf build_valgrind
    meson setup build_valgrind ${tests[0]} ${BACKEND_OPT} -Dtests=true -Db_sanitize=address,undefined ${EXTRA_ARGS}
    meson compile -C build_valgrind
    meson test -C build_valgrind "unit tests" --print-errorlogs --wrap=valgrind || echo "Valgrind issues detected"
    rm -rf build_valgrind
fi

for features in "${tests[@]}"
do
    echo -e "${YELLOW}--> Building with: ${features} ${BACKEND_OPT} ${EXTRA_ARGS}${NC}"
    rm -rf build_run
    meson setup build_run ${features} ${BACKEND_OPT} -Dtests=true ${EXTRA_ARGS}
    meson compile -C build_run
    meson test -C build_run "unit tests" --print-errorlogs
    ./build_run/profanity -v
done

echo -e "${GREEN}Build configuration matrix testing successful!${NC}"

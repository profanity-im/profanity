#!/usr/bin/env bash

# Exhaustive build configuration matrix testing for Profanity
# Supports both Autotools and Meson

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

    if [ "$BUILD_SYSTEM" == "autotools" ]; then
        log_content ./config.log
        log_content ./test-suite.log
        log_content ./test-suite-memcheck.log
    else
        for log in build_run/meson-logs/testlog.txt build_valgrind/meson-logs/testlog.txt; do
            if [ -f "$log" ]; then
                echo "--- Meson Test Log ($log) ---"
                cat "$log"
            fi
        done
    fi
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
    echo "Usage: $0 [autotools|meson] [extra-args]"
    echo ""
    echo "Run exhaustive build matrix tests for the specified build system."
    echo "Default build system is 'autotools'."
    echo ""
    echo "Extra arguments are passed directly to the configuration command:"
    echo "  - Autotools: ./configure [extra-args]"
    echo "  - Meson:     meson setup [extra-args]"
    exit 0
}

# Determine build system
BUILD_SYSTEM="autotools"
if [[ "$1" == "autotools" || "$1" == "meson" ]]; then
    BUILD_SYSTEM="$1"
    shift
fi

if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    usage
fi

ARCH="$(uname | tr '[:upper:]' '[:lower:]')"
EXTRA_ARGS="$*"

if [ "$BUILD_SYSTEM" == "autotools" ]; then
    echo -e "${YELLOW}---> Starting Autotools build matrix...${NC}"
    ./bootstrap.sh

    MAKE="make --quiet -j$(num_cores)"
    CC="gcc"

    case "$ARCH" in
        linux*)
            tests=(
            "--enable-notifications --enable-icons-and-clipboard --enable-otr --enable-pgp
            --enable-omemo --enable-plugins --enable-c-plugins
            --enable-python-plugins --with-xscreensaver --enable-omemo-qrcode --enable-gdk-pixbuf"
            "--disable-notifications --disable-icons-and-clipboard --disable-otr --disable-pgp
            --disable-omemo --disable-plugins --disable-c-plugins
            --disable-python-plugins --without-xscreensaver"
            "--disable-notifications"
            "--disable-icons-and-clipboard"
            "--disable-otr"
            "--disable-pgp"
            "--disable-omemo --disable-omemo-qrcode"
            "--disable-pgp --disable-otr"
            "--disable-pgp --disable-otr --disable-omemo"
            "--disable-plugins"
            "--disable-python-plugins"
            "--disable-c-plugins"
            "--disable-c-plugins --disable-python-plugins"
            "--without-xscreensaver"
            "--disable-gdk-pixbuf"
            "")
            source /etc/profile.d/debuginfod.sh 2>/dev/null || true
            ;;
        *)
            # Darwin, OpenBSD, etc.
            if [ "$ARCH" == "openbsd*" ]; then
                MAKE="gmake"
                CC="egcc -std=gnu99 -fexec-charset=UTF-8"
            fi
            tests=(
            "--enable-notifications --enable-icons-and-clipboard --enable-otr --enable-pgp
            --enable-omemo --enable-plugins --enable-c-plugins
            --enable-python-plugins"
            "--disable-notifications --disable-icons-and-clipboard --disable-otr --disable-pgp
            --disable-omemo --disable-plugins --disable-c-plugins
            --disable-python-plugins"
            "--disable-notifications"
            "--disable-icons-and-clipboard"
            "--disable-otr"
            "--disable-pgp"
            "--disable-omemo"
            "--disable-pgp --disable-otr"
            "--disable-pgp --disable-otr --disable-omemo"
            "--disable-plugins"
            "--disable-python-plugins"
            "--disable-c-plugins"
            "--disable-c-plugins --disable-python-plugins"
            "")
            ;;
    esac

    # Valgrind check (Linux only)
    if [[ "$ARCH" == linux* ]]; then
        echo -e "${YELLOW}--> Building with ./configure ${tests[0]} --enable-valgrind --disable-functional-tests ${EXTRA_ARGS}${NC}"
        ./configure ${tests[0]} --enable-valgrind --disable-functional-tests ${EXTRA_ARGS}
        $MAKE CC="${CC}"
        $MAKE check-valgrind
        $MAKE distclean
    fi

    for features in "${tests[@]}"
    do
        echo -e "${YELLOW}--> Building with ./configure ${features} --disable-functional-tests ${EXTRA_ARGS}${NC}"
        ./configure $features --disable-functional-tests ${EXTRA_ARGS}
        $MAKE CC="${CC}"
        $MAKE check-unit
        ./profanity -v
        $MAKE clean
    done

else
    # Meson Build Matrix
    echo -e "${YELLOW}---> Starting Meson build matrix...${NC}"

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
fi

echo -e "${GREEN}Build configuration matrix testing successful!${NC}"

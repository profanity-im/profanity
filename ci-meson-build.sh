#!/usr/bin/env bash

# Exit on error
set -e

error_handler()
{
    ERR_CODE=$?
    echo
    echo "Error ${ERR_CODE} with command '${BASH_COMMAND}' on line ${BASH_LINENO[0]}. Exiting."
    # Meson logs are stored in the build directory
    if [ -f "build/meson-logs/testlog.txt" ]; then
        echo "--- Meson Test Log ---"
        cat build/meson-logs/testlog.txt
    fi
    exit ${ERR_CODE}
}

trap error_handler ERR

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

# Run Valgrind check (Only on Linux, on first/full feature set)
if [[ "$(uname | tr '[:upper:]' '[:lower:]')" == linux* ]]; then
    echo "--> Running Valgrind check with full features"

    meson setup build_valgrind ${tests[0]} -Dtests=true -Db_sanitize=address,undefined
    meson compile -C build_valgrind
    
    meson test -C build_valgrind --print-errorlogs --wrap=valgrind || echo "Valgrind issues detected"
    
    rm -rf build_valgrind
fi

# Iterate through all feature combinations
for features in "${tests[@]}"
do
    echo "----------------------------------------------------"
    echo "--> Building with: ${features}"
    echo "----------------------------------------------------"
    
    rm -rf build_run
    
    meson setup build_run ${features} -Dtests=true
    meson compile -C build_run
    
    meson test -C build_run --print-errorlogs
    
    ./build_run/profanity -v
done

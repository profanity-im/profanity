export COLUMNS=300
if [ -f "./build_run/profanity" ]; then
    ./build_run/profanity -l DEBUG
else
    # Fallback for autotools
    ./profanity -l DEBUG
fi

#!/usr/bin/env bash

# Central quality check script for Profanity

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Go to project root
cd "$(dirname "$0")/.."

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Description:"
    echo "  Code quality checks (spelling, formatting, and unit tests)."
    echo "  Works with autotools and meson and can be used as a git"
    echo "  pre-commit hook."
    echo ""
    echo "Options:"
    echo "  --fix-formatting Apply formatting fixes (default is check only)"
    echo "  --no-format      Skip the formatting check/fix entirely. Use this if your local"
    echo "                   clang-format version produces different results than the CI."
    echo "                   Can also be set via SKIP_FORMAT=1 environment variable."
    echo "  --autotools      Run unit tests using Autotools (make check-unit)"
    echo "  --meson          Run unit tests using Meson (meson test)"
    echo "  --hook           Git hook mode: Checks only staged files for spelling and"
    echo "                   formatting. Does not run tests to ensure fast commits."
    echo "  --install        Install this script as a git pre-commit hook"
    echo "  --help           Show this help message"
}

run_tests() {
    local system=$1
    echo -e "${YELLOW}---> Running unit tests...${NC}"
    
    if [ "$system" == "autotools" ]; then
        echo "Using Autotools..."
        make check-unit
    elif [ "$system" == "meson" ]; then
        echo "Using Meson..."
        # Uses build_run
        meson test -C build_run "unit tests"
    fi
}

run_format() {
    local mode=$1
    local hook_mode=$2
    local skip=$3
    local files

    if [ "$skip" = true ]; then
        echo -e "${YELLOW}---> Skipping formatting check.${NC}"
        return
    fi

    if [ "$hook_mode" = true ]; then
        echo -e "${YELLOW}---> Checking staged files for formatting...${NC}"
        # Only added/modified C files
        files=$(git diff --cached --name-only --diff-filter=ACMR | grep -E "\.(c|h)$" || true)
    else
        echo -e "${YELLOW}---> Checking all files for formatting...${NC}"
        files=$(find src tests -name "*.[ch]" || true)
    fi

    if [ -z "$files" ]; then
        echo "No C files to check."
        return
    fi

    if [ "$mode" = "fix" ]; then
        clang-format -i $files
        echo -e "${GREEN}Formatting applied.${NC}"
    else
        # --Werror makes it return non-zero on diff
        if ! clang-format --dry-run --Werror $files; then
            echo -e "${RED}Error: Style violations found. Run '$0 --fix-formatting' to resolve.${NC}"
            echo -e "${RED}If this is due to a clang-format version mismatch, use --no-format or SKIP_FORMAT=1.${NC}"
            exit 1
        fi
        echo -e "${GREEN}Style check passed.${NC}"
    fi
}

run_spell() {
    echo -e "${YELLOW}---> Running spell check...${NC}"
    if command -v codespell >/dev/null 2>&1; then
        codespell
        echo -e "${GREEN}Spell check passed.${NC}"
    else
        echo -e "${YELLOW}Warning: codespell not found, skipping.${NC}"
    fi
}

MODE="check"
BUILD_SYSTEM="none"
HOOK=false

# Support environment variable override
if [[ "$SKIP_FORMAT" == "1" || "$SKIP_FORMAT" == "true" ]]; then
    INTERNAL_SKIP_FORMAT=true
else
    INTERNAL_SKIP_FORMAT=false
fi

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --fix-formatting) MODE="fix" ;;
        --no-format) INTERNAL_SKIP_FORMAT=true ;;
        --autotools) BUILD_SYSTEM="autotools" ;;
        --meson) BUILD_SYSTEM="meson" ;;
        --hook) HOOK=true ;;
        --install)
            echo "Installing pre-commit hook..."
            mkdir -p .git/hooks
            echo "#!/usr/bin/env bash" > .git/hooks/pre-commit
            echo "./scripts/quality-check.sh --hook" >> .git/hooks/pre-commit
            chmod +x .git/hooks/pre-commit
            echo "Hook installed."
            exit 0
            ;;
        --help) usage; exit 0 ;;
        *) echo "Unknown parameter: $1"; usage; exit 1 ;;
    esac
    shift
done

# Always run spell check
run_spell

# Run format check unless skipped
run_format "$MODE" "$HOOK" "$INTERNAL_SKIP_FORMAT"

# Run tests if a build system was specified
if [ "$BUILD_SYSTEM" != "none" ]; then
    run_tests "$BUILD_SYSTEM"
fi

echo -e "${GREEN}Quality check successful!${NC}"

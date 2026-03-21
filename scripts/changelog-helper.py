#!/usr/bin/env python3

# Since we use Conventional Commits
# we can now create our Changelog semi-automatically

import subprocess
import sys
import re
import argparse
from collections import defaultdict

# Configuration for sections and their order
SECTION_CONFIG = [
    ("fix", "Bug Fixes"),
    ("feat", "Features"),
    ("build", "Build System"),
    ("ci", "CI"),
    ("docs", "Documentation"),
    ("perf", "Performance Improvements"),
    ("refactor", "Refactorings"),
    ("cleanup", "Cleanup"),
    ("style", "Style"),
    ("tests", "Tests"),
    ("chore", "Chores"),
]
SECTIONS = dict(SECTION_CONFIG)
TYPE_ORDER = [t for t, _ in SECTION_CONFIG]

CORRECTIONS = {
    "ests": "tests",
    "wleanup": "cleanup",
}

def git_run(args):
    """Run a git command and return stripped output lines."""
    try:
        result = subprocess.run(["git"] + args, capture_output=True, text=True, check=True)
        return [line for line in result.stdout.strip().split('\n') if line]
    except subprocess.CalledProcessError:
        return []

def get_last_tag():
    output = git_run(["describe", "--tags", "--abbrev=0"])
    return output[0] if output else None

def get_commits(revision_range):
    """Get list of (hash, subject) tuples."""
    lines = git_run(["log", revision_range, "--format=%H %s"])
    commits = []
    for line in lines:
        parts = line.split(' ', 1)
        if len(parts) == 2:
            commits.append(parts)
    return commits

def get_pr_mappings(revision_range):
    """Map commit hashes to PR numbers found in merge commits."""
    merge_commits = git_run(["log", revision_range, "--merges", "--format=%H %s"])
    pr_map = {}
    pr_re = re.compile(r'Merge pull request #(\d+)')

    for line in merge_commits:
        parts = line.split(' ', 1)
        if len(parts) < 2:
            continue
        m_hash, m_subject = parts
        match = pr_re.search(m_subject)
        if match:
            pr_num = match.group(1)
            # Find all commits that are part of this merge branch
            branch_commits = git_run(["rev-list", f"{m_hash}^1..{m_hash}^2"])
            for b_hash in branch_commits:
                pr_map[b_hash] = pr_num
    return pr_map

def get_contributors(revision_range):
    """Get sorted list of all unique contributors."""
    cmd = ["log", revision_range, "--format=%an%n%(trailers:key=Co-authored-by,valueonly=true)"]
    output = git_run(cmd)
    
    contributors = set()
    for line in output:
        # Remove email part if present: "Name <email@example.com>" -> "Name"
        name = line.split('<')[0].strip()
        if name:
            contributors.add(name)
    
    return sorted(list(contributors))

def format_description(description):
    """Capitalize first letter of description."""
    description = description.strip()
    if description and description[0].islower():
        return description[0].upper() + description[1:]
    return description

def main():
    parser = argparse.ArgumentParser(description="Generate a sorted changelog from git commits.")
    parser.add_argument("--pr", action="store_true", help="Append PR number to each commit.")
    args = parser.parse_args()

    last_tag = get_last_tag()
    if not last_tag:
        print("No tags found in the repository.", file=sys.stderr)
    
    revision_range = f"{last_tag}..HEAD" if last_tag else "HEAD"
    
    commits = get_commits(revision_range)
    if not commits:
        print(f"No commits found since {last_tag if last_tag else 'the beginning'}.")
        return

    pr_map = get_pr_mappings(revision_range) if args.pr else {}

    # Conventional Commit regex: type(scope): description
    commit_re = re.compile(r'^(\w+)(?:\(([^)]+)\))?:\s*(.*)$')

    grouped = defaultdict(list)
    others = []

    for c_hash, c_subject in commits:
        # Skip merge commits in the output
        if c_subject.startswith(("Merge pull request", "Merge branch")):
            continue

        pr_suffix = f" (#{pr_map[c_hash]})" if c_hash in pr_map else ""
        match = commit_re.match(c_subject)

        if match:
            ctype = match.group(1).lower()
            ctype = CORRECTIONS.get(ctype, ctype)
            description = format_description(match.group(3))
            grouped[ctype].append(f"{description}{pr_suffix}")
        else:
            others.append(f"{c_subject}{pr_suffix}")

    # Output sections in ordered priority
    all_types = TYPE_ORDER + sorted([t for t in grouped if t not in TYPE_ORDER])
    first = True
    for ctype in all_types:
        if ctype in grouped:
            if not first:
                print()
            section_name = SECTIONS.get(ctype, ctype.capitalize())
            print(f"{section_name}:")
            for msg in sorted(grouped[ctype]):
                print(f"- {msg}")
            first = False

    if others:
        if not first:
            print()
        print("Others:")
        for msg in sorted(others):
            print(f"- {msg}")

    # Contributors section
    contributors = get_contributors(revision_range)
    if contributors:
        print("\nContributors:")
        for i, name in enumerate(contributors, 1):
            print(f"{i}. {name}")

if __name__ == "__main__":
    main()

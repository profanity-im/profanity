#!/usr/bin/env python3
import os
import re
import sys
import urllib.request
import xml.etree.ElementTree as ET
from itertools import zip_longest
from typing import Dict, List, Optional


# Namespaces in DOAP
NS = {
    "rdf": "http://www.w3.org/1999/02/22-rdf-syntax-ns#",
    "foaf": "http://xmlns.com/foaf/0.1/",
    "doap": "http://usefulinc.com/ns/doap#",
    "xmpp": "https://linkmauve.fr/ns/xmpp-doap#",
    "schema": "https://schema.org/",
}


def compare_versions(v1: str, v2: str) -> int:
    """
    Compare two version strings.
    Returns 1 if v2 > v1, -1 if v1 > v2, 0 if equal.
    """
    def parse_v(v: str) -> List[int]:
        return [int(x) for x in re.split(r"[^0-9]+", v) if x]

    parts1 = parse_v(v1)
    parts2 = parse_v(v2)

    for p1, p2 in zip_longest(parts1, parts2, fillvalue=0):
        if p2 > p1:
            return 1
        if p1 > p2:
            return -1
    return 0


def find_doap() -> Optional[str]:
    """Try to find profanity.doap in current or parent directory."""
    candidates = ["profanity.doap", "../profanity.doap"]
    for c in candidates:
        if os.path.exists(c):
            return c
    return None


def get_implemented_xeps(doap_path: str) -> Dict[str, str]:
    """Parse DOAP file and return a map of XEP number to version."""
    implemented_xeps: Dict[str, str] = {}
    try:
        tree = ET.parse(doap_path)
        root = tree.getroot()

        for implements in root.findall(".//doap:implements", NS):
            supported_xep = implements.find(".//xmpp:SupportedXep", NS)
            if supported_xep is not None:
                xep_res = supported_xep.find(".//xmpp:xep", NS)
                version_elem = supported_xep.find(".//xmpp:version", NS)

                if xep_res is not None and version_elem is not None:
                    resource = xep_res.attrib.get(f"{{{NS['rdf']}}}resource", "")
                    match = re.search(r"xep-(\d+)\.html", resource)
                    if match:
                        xep_num = match.group(1)
                        current_version = version_elem.text.strip() if version_elem.text else ""
                        implemented_xeps[xep_num] = current_version
    except (ET.ParseError, PermissionError) as e:
        print(f"Error reading {doap_path}: {e}")
        sys.exit(1)

    return implemented_xeps


def check_xeps() -> None:
    """Main logic for checking XEP updates."""
    doap_path = find_doap()
    if not doap_path:
        print("Error: Could not find DOAP file.")
        sys.exit(1)

    implemented_xeps = get_implemented_xeps(doap_path)
    if not implemented_xeps:
        print("No XEPs found in DOAP file.")
        return

    print(f"XEPs tracked: {len(implemented_xeps)}")

    try:
        url = "https://xmpp.org/extensions/xeplist.xml"
        with urllib.request.urlopen(url, timeout=15) as response:
            xeplist_xml = response.read()
    except Exception as e:
        print(f"Error fetching xeplist.xml: {e}")
        return

    try:
        xeplist_tree = ET.fromstring(xeplist_xml)
    except ET.ParseError as e:
        print(f"Error parsing xeplist.xml: {e}")
        return

    print(f"\n{'XEP':<10} | {'Name':<35} | {'Ours':<8} | {'Latest':<8}")
    print("-" * 80)

    updates_found = 0
    for xep_node in xeplist_tree.findall("xep"):
        number_node = xep_node.find("number")
        title_node = xep_node.find("title")
        last_revision_node = xep_node.find("last-revision")

        if number_node is None or title_node is None or last_revision_node is None:
            continue

        number = (number_node.text or "0").zfill(4)
        name = title_node.text or "Unknown"
        version_node = last_revision_node.find("version")

        if version_node is None or version_node.text is None:
            continue

        latest_version = version_node.text

        if number in implemented_xeps:
            old_version = implemented_xeps[number]
            if compare_versions(old_version, latest_version) > 0:
                print(f"XEP-{number:<4} | {name[:35]:<35} | {old_version:<8} | {latest_version:<8}")
                updates_found += 1

    if updates_found == 0:
        print("All tracked XEPs are up to date.")


if __name__ == "__main__":
    check_xeps()

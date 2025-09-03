#!/usr/bin/env python3
"""
Script that extracts the test coverage percentage and saves it as a JSON
shields.io endpoint.
"""

import contextlib
import re
import sys
from os.path import dirname, join, normpath, realpath

script_dir = dirname(realpath(__file__))
cov_dir = normpath(join(dirname(script_dir), "docs", "Coverage"))

with contextlib.suppress(IndexError):
    cov_dir = sys.argv[1]

json = """\
{{
  "schemaVersion": 1,
  "label": "Test Coverage",
  "message": "{linecov}%",
  "color": "green"
}}
"""


def main():
    with open(join(cov_dir, "index.html")) as f:
        pattern = r'<td class="headerCovTableEntry\w+">([\d.]+)'
        (linecov,) = (m.group(1) for m in re.finditer(pattern, f.read()))
        print(linecov)  # noqa: T201

    with open(join(cov_dir, "shield.io.coverage.json"), "w") as f:
        f.write(json.format(linecov=linecov))


if __name__ == "__main__":
    main()

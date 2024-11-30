#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Simple script to update the list of contributors in AUTHORS.md
by fetching contribution statistics from GitHub.

Just call this script with no arguments provided.

Requirements:

    pip install PyGithub~=2.5
"""

import os
from github import Auth, Github  # noqa: F401


auth = None
# auth = Auth.Token("insert-token-here")


if __name__ == '__main__':
    # Fetch contributors from GitHub
    g = Github(auth=auth)
    repo = g.get_repo("LibrePCB/LibrePCB")
    contributors = list(repo.get_contributors())
    contributors = sorted(contributors,
                          key=lambda user: user.contributions, reverse=True)
    lines = []
    for c in contributors:
        if c.login in ['ubruhin']:
            continue  # Skip main authors
        elif c.name is not None:
            line = f"- {c.name} ([@{c.login}]({c.html_url}))"
        else:
            line = f"- [@{c.login}]({c.html_url})"
        lines.append(line)

    # Update file AUTHORS.md
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    authors_file = os.path.join(root, 'AUTHORS.md')
    with open(authors_file, 'r') as f:
        content = f.read()
    start_pattern = '## Contributors\n'
    start_index = content.find(start_pattern) + len(start_pattern)
    end_index = content.find('\n\n', start_index)
    manual_lines = []
    for line in content[start_index:end_index].splitlines():
        if line.startswith('- ') and ('github.com' not in line):
            manual_lines.append(line)
    content = content[:start_index] + \
        '\n'.join(lines + sorted(manual_lines)) + \
        content[end_index:]
    with open(authors_file, 'w') as f:
        f.write(content)

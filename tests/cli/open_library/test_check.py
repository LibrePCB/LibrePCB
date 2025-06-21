#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import shutil
from helpers import nofmt

"""
Test command "open-library --check"
"""


def test_no_messages(cli):
    library = params.EMPTY_LIBRARY
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run("open-library", "--all", "--check", library.dir)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open library '{library.dir}'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process {library.sym} symbols...
Process {library.pkg} packages...
Process {library.cmp} components...
Process {library.dev} devices...
SUCCESS
""")
    assert code == 0


def test_messages(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    for subdir in ["sym", "pkg", "cmp"]:
        shutil.rmtree(cli.abspath(os.path.join(library.dir, subdir)))
    code, stdout, stderr = cli.run("open-library", "--all", "--check", library.dir)
    assert stderr == nofmt("""\
  - R-0805 (078650d3-483c-4b9e-a848-b14f1aad2edc):
    - [HINT] No part numbers added
  - R-0603 (483a71eb-318e-448e-82ff-f02efc4821aa):
    - [HINT] No part numbers added
  - PSMN022-30PL (5738d8f9-4101-4409-bd46-d9c173b40d60):
    - [ERROR] No categories set
    - [HINT] No part numbers added
  - R-1206 (a6a6744d-7d3b-450a-b782-feca43939ca5):
    - [HINT] No part numbers added
  - C-0805 (c139e505-592b-46ba-bdf2-acb7383ea0cd):
    - [HINT] No part numbers added
  - PSMN5R8 (f7fb22e8-0bbc-4f0f-aa89-596823b5bc3e):
    - [ERROR] No categories set
    - [HINT] No part numbers added
  - Crystal ABM3 (f83a5ae8-7f42-42be-9dd6-e762f4da2ec2):
    - [HINT] No part numbers added
""")
    assert stdout == nofmt(f"""\
Open library 'Populated Library.lplib'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process 0 symbols...
Process 0 packages...
Process 0 components...
Process {library.dev} devices...
Finished with errors!
""")
    assert code == 1

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import shutil
import pytest
import subprocess


CLI_DIR = os.path.dirname(__file__)
TESTS_DIR = os.path.dirname(CLI_DIR)
REPO_DIR = os.path.dirname(TESTS_DIR)
DATA_DIR = os.path.join(TESTS_DIR, "data")


def pytest_addoption(parser):
    parser.addoption(
        "--librepcb-executable",
        action="store",
        help="Path to librepcb-cli executable to test",
    )


class CliExecutor(object):
    def __init__(self, config, tmpdir):
        super(CliExecutor, self).__init__()
        self.executable = os.path.abspath(config.getoption("--librepcb-executable"))
        if not os.path.exists(self.executable):
            raise Exception(
                "Executable '{}' not found. Please pass it with "
                "'--librepcb-executable'.".format(self.executable)
            )
        self.tmpdir = tmpdir

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def abspath(self, relpath):
        return os.path.normpath(os.path.join(self.tmpdir, relpath))

    def add_library(self, library):
        src = os.path.join(DATA_DIR, "libraries", library)
        dst = os.path.join(self.tmpdir, library)
        shutil.copytree(src, dst)

    def add_project(self, project, as_lppz=False):
        src = os.path.join(DATA_DIR, "projects", project)
        dst = os.path.join(self.tmpdir, project)
        if as_lppz:
            shutil.make_archive(dst, "zip", src)
            shutil.move(dst + ".zip", dst + ".lppz")
        else:
            shutil.copytree(src, dst)

    def add_file(self, relpath):
        src = os.path.join(DATA_DIR, relpath)
        dst = os.path.join(self.tmpdir, os.path.basename(relpath))
        shutil.copyfile(src, dst)
        return dst

    def run(self, *args):
        p = subprocess.Popen(
            [self.executable] + list(args),
            cwd=self.tmpdir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            env=self._env(),
        )
        stdout, stderr = p.communicate()
        # output to stdout/stderr because it helps debugging failed tests
        sys.stdout.write(stdout)
        sys.stderr.write(stderr)
        return p.returncode, stdout, stderr

    def _env(self):
        env = os.environ
        # Make output independent from the system's language
        env["LC_ALL"] = "C"
        # Override configuration location to make tests independent of existing configs
        env["LIBREPCB_CONFIG_DIR"] = os.path.join(self.tmpdir, "config")
        # Use a neutral username
        env["USERNAME"] = "testuser"
        # Disable warning about unstable file format, since tests are run also
        # on the (unstable) master branch
        env["LIBREPCB_DISABLE_UNSTABLE_WARNING"] = "1"
        return env


@pytest.fixture
def cli(request, tmpdir):
    """
    Fixture to start the LibrePCB CLI
    """
    with CliExecutor(request.config, str(tmpdir)) as executor:
        yield executor

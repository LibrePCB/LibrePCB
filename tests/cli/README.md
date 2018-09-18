# System Tests for the LibrePCB CLI

This directory contains system tests for `librepcb-cli` using
[pytest](https://docs.pytest.org).

## Create Virtualenv

    mkvirtualenv -p `which python3` librepcb-cli

## Install Requirements

    pip install -r requirements.txt

## Run Tests

    pytest -v --librepcb-executable=/path/to/librepcb-cli

## Links

- [Documentation of `pytest`](https://docs.pytest.org/en/latest/contents.html)

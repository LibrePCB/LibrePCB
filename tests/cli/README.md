# System Tests for the LibrePCB CLI

This directory contains system tests for `librepcb-cli` using
[pytest](https://docs.pytest.org).

## Requirements

- [`uv`](https://docs.astral.sh/uv/)

## Run Tests

    uv run pytest -v --librepcb-executable=/path/to/librepcb-cli

## Run Linter

    uv run ruff check

## Auto-format Files

    uv run ruff format

## Links

- [Documentation of `pytest`](https://docs.pytest.org/en/latest/contents.html)

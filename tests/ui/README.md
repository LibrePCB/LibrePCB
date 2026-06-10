# System Level UI Tests

This directory contains system tests (simulating user input) using
[pytest](https://docs.pytest.org).

## Requirements

- [`uv`](https://docs.astral.sh/uv/)
- Access to the Slint's [UI Testing Framework](https://testing.slint.dev/).
  This is not public so far, therefore it's unfortunately not possible yet
  for you to run those tests locally. On CI, those tests are run.

## Run Tests

    uv run pytest -v --librepcb-executable=/path/to/librepcb

## Run Linter

    uv run ruff check

## Auto-format Files

    uv run ruff format

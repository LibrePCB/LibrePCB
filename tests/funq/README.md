# Functional Tests

This directory contains functional tests (simulating user input) using
[funq](https://github.com/parkouss/funq) and [pytest](https://docs.pytest.org).

## Requirements

- [`uv`](https://docs.astral.sh/uv/)

## Run Tests

    uv run pytest -v --librepcb-executable=/path/to/librepcb

## Run Linter

    uv run ruff check

## Auto-format Files

    uv run ruff format

## Links

- [Documentation of `funq`](http://funq.readthedocs.io/en/latest/)
- [Documentation of `pytest`](https://docs.pytest.org/en/latest/contents.html)

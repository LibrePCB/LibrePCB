# Functional Tests

This directory contains functional tests (simulating user input) using
[funq](https://github.com/parkouss/funq) and [pytest](https://docs.pytest.org).

## Create Virtualenv

    mkvirtualenv -p `which python3` librepcb-funq

## Install Requirements

    pip install -r requirements.txt

## Run Tests

    pytest -v --librepcb-executable=/path/to/librepcb

## Links

- [Documentation of `funq`](http://funq.readthedocs.io/en/latest/)
- [Documentation of `pytest`](https://docs.pytest.org/en/latest/contents.html)

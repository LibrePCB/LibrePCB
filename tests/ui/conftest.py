#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import platform
import pytest
import shutil
import slint_testing
import time
from copy import deepcopy
from typing import Iterable


ROOT_DIR = os.path.dirname(__file__)
TESTS_DIR = os.path.dirname(ROOT_DIR)
REPO_DIR = os.path.dirname(TESTS_DIR)
DATA_DIR = os.path.join(TESTS_DIR, "data")


def pytest_addoption(parser):
    parser.addoption(
        "--librepcb-executable",
        action="store",
        help="Path to librepcb executable to test",
    )


# Avoid errors due to too long file path on Windows :-|
def _long_path(path):
    if os.name == "nt":
        return "\\\\?\\" + path.replace("/", "\\")
    else:
        return path


def _flatten(items):
    for x in items:
        if isinstance(x, Iterable) and not isinstance(x, (str, bytes)):
            for sub_x in _flatten(x):
                yield sub_x
        else:
            yield x


def _query_childs(element, query):
    segment = query[0].split("[")
    segment_name = segment[0]
    if segment_name.startswith("#"):
        segment_name = segment_name[1:]
        childs = element.query_descendants().match_type_name(segment_name).find_all()
    else:
        childs = element.query_descendants().match_id(segment_name).find_all()
    if len(segment) > 1:
        index = int(segment[1][:-1])
        childs = [childs[index]]
    if len(query) > 1:
        childs = [_query_childs(child, query[1:]) for child in childs]
    return childs


class Element:
    """
    Wrapper around slint_testing.Element

    Basically this wrapper only provides shorter names for the properties and
    functions, to keep test code as readable as possible.
    """

    def __init__(self, element):
        self._element = element

    @property
    def valid(self):
        return self._element.is_valid

    @property
    def position_abs(self):
        return self._element.absolute_position

    @property
    def label(self):
        return self._element.accessible_label

    @property
    def placeholder(self):
        return self._element.accessible_placeholder_text

    @property
    def value(self):
        return self._element.accessible_value

    @property
    def checked(self):
        return self._element.accessible_checked

    @property
    def enabled(self):
        return self._element.accessible_enabled

    @property
    def readonly(self):
        return self._element.accessible_read_only

    def trigger(self):
        self._element.invoke_accessible_default_action()

    def click(self):
        self._element.single_click(slint_testing.PointerEventButton.Left)

    def dclick(self):
        self._element.double_click(slint_testing.PointerEventButton.Left)

    def set_value(self, value):
        self._element.accessible_value = value

    def set_checked(self, checked):
        if self._element.accessible_checked != checked:
            self._element.invoke_accessible_default_action()


class ElementQuery:
    """
    Helper to query and validate UI elements

    The `get()` method allows to pass a query string in this form:

        LibrariesPanel::local-libs #LibraryListViewItem *

    Where:

    * Each nesting level is separated by a space. Queries are executed
      recursively, so not every level needs to be supplied.
    * Each string segment matches Slint elements by their ID.
    * Segments starting with "#" match Slint elements by type name rather than
      by ID.
    * Optionally, a last segment can be supplied to specify the expected number
      of Slint elements matching the query. A star "*" means any number of
      result is allowed (0..n). A question mark "?" means any number of result
      is allowed (0..n), and the query shall not be executed yet (must be done
      by calling `wait()` manually). Omitting the suffix means exactly one
      result (==1) is expected. The method raises an exception if there
      are != 1 elements matching the query.

    The suffix also impacts the API of this class. All functions returning
    a value will return a single value if no suffix is provided. If "*" or "?"
    was provided, all methods return a list of values, according to the
    number of elements matching the query.
    """

    def __init__(self, app, window, query=None):
        self._app = app
        self._window = window
        self._query = query or []
        self._results = None
        self._multi_result = False
        suffix = self._query[-1] if len(self._query) else ""
        if suffix == "?":
            self._query = self._query[:-1]
        elif suffix == "*":
            self._query = self._query[:-1]
            self._multi_result = True
            self.refresh()
        else:
            self.wait(1)

    def __getitem__(self, index):
        """
        Get a particular result by index (an `Element` object)
        """
        return self._results[index]

    @property
    def window(self):
        return self._app.windows[self._window]

    def get(self, path):
        """
        Query children of this query's results

        See class documentation for details.
        """
        return ElementQuery(self._app, self._window, self._query + path.split(" "))

    def refresh(self):
        """
        Re-execute the query once (e.g. after the UI has changed)
        """
        root = self.window.root_element
        if not self._query:
            self._results = [root]
        else:
            self._results = list(_flatten(_query_childs(root, self._query)))
        self._results = [Element(x) for x in self._results]

    def wait(self, min=1, max=-1, timeout=10.0):
        """
        Re-execute the query in a loop until a specific number of results are
        matching the query

        The arguments `min` and `max` are used to specify the number of
        results to wait for. A `max` value of `-1` means `max==min`, i.e.
        a specific count is expected rather than a range. If `max` is `None`,
        any number of results `>=min` are considered as valid.

        The function aborts after the provided timeout in seconds.
        """
        if max == -1:
            max = min
        start = time.time()
        while True:
            self.refresh()
            count = len(self._results)
            if (count >= min) and ((max is None) or (count <= max)):
                return
            if time.time() - start > timeout:
                self.save_screenshot()
                raise TimeoutError(
                    f"Expecting {min}..{max} results, got {count}: {self._query} "
                    + "-- Screenshot saved."
                )
            time.sleep(0.01)

    def read(self, keys):
        """
        Read several element properties
        """
        props = dict()
        for key in keys:
            match key:
                case "valid":
                    props[key] = [r.valid for r in self._results]
                case "label":
                    props[key] = [r.label for r in self._results]
                case "placeholder":
                    props[key] = [r.placeholder for r in self._results]
                case "value":
                    props[key] = [r.value for r in self._results]
                case "checked":
                    props[key] = [r.checked for r in self._results]
                case "enabled":
                    props[key] = [r.enabled for r in self._results]
                case "readonly":
                    props[key] = [r.readonly for r in self._results]
                case _:
                    raise ValueError(f"Unknown property: '{key}'")
        return props

    def wait_for(self, timeout=10.0, **properties):
        """
        Wait for all query's results to have specific properties set
        """

        def _compare(actual, expected):
            if isinstance(expected, list):
                return actual == expected
            else:
                return all([a == expected for a in actual])

        if ("valid" in properties) and (len(properties) > 1):
            raise ValueError(
                f"Cannot check for other properties in addition to 'valid': {properties}"
            )

        start = time.time()
        while True:
            actual_props = self.read(properties.keys())
            if all([_compare(v, properties[k]) for k, v in actual_props.items()]):
                return
            if time.time() > (start + timeout):
                self.save_screenshot()
                raise TimeoutError(
                    f"Timeout while waiting for {properties}, actual {actual_props}: "
                    + f"{self._query} -- Screenshot saved."
                )
            time.sleep(0.01)
            # In some cases, UI elements temporarily disappear (for example
            # due to model resets). This would invalidate element handles,
            # so we regularly have to re-evaluate the query.
            self.refresh()
        return self

    @property
    def valid(self):
        return self._flatten([x.valid for x in self._results])

    @property
    def label(self):
        return self._flatten([x.label for x in self._results])

    @property
    def placeholder(self):
        return self._flatten([x.placeholder for x in self._results])

    @property
    def value(self):
        return self._flatten([x.value for x in self._results])

    @property
    def checked(self):
        return self._flatten([x.checked for x in self._results])

    @property
    def enabled(self):
        return self._flatten([x.enabled for x in self._results])

    def trigger(self):
        for x in self._results:
            x.trigger()

    def click(self):
        for x in self._results:
            x.click()

    def dclick(self):
        for x in self._results:
            x.dclick()

    def set_value(self, value):
        for x in self._results:
            x.set_value(value)

    def set_checked(self, checked):
        for x in self._results:
            x.set_checked(checked)

    def save_screenshot(self, fname="screenshot.png"):
        path = os.path.join(ROOT_DIR, fname)
        with open(path, "wb") as f:
            f.write(self.window.grab_window_as_png())

    def _flatten(self, result):
        if self._multi_result or len(result) != 1:
            return result
        else:
            return result[0]


class Application(slint_testing.Application):
    def root(self, window=0):
        return ElementQuery(self, window)

    def get(self, path, window=0):
        return self.root(window=window).get(path)


class LibrePcbFixture(object):
    def __init__(self, config, tmpdir):
        super(LibrePcbFixture, self).__init__()
        self.executable = os.path.abspath(config.getoption("--librepcb-executable"))
        if not os.path.exists(self.executable):
            raise Exception(
                "Executable '{}' not found. Please pass it with "
                "'--librepcb-executable'.".format(self.executable)
            )
        self.tmpdir = tmpdir
        # Copy test data to temporary directory to avoid modifications in original data
        shutil.copytree(
            _long_path(os.path.join(DATA_DIR, "workspaces", "Empty Workspace")),
            _long_path(os.path.join(self.tmpdir, "Empty Workspace")),
        )
        # Init members to default values
        self.workspace_path = os.path.join(self.tmpdir, "Empty Workspace")
        self.project_path = None

        # Set environment variables
        self.env = deepcopy(os.environ)
        # Make GUI independent from the system's language
        self.env["LC_ALL"] = "C"
        # Override configuration location to make tests independent of existing configs
        self.env["LIBREPCB_CONFIG_DIR"] = os.path.join(self.tmpdir, "config")
        # Override cache location to make each test indepotent
        self.env["LIBREPCB_CACHE_DIR"] = os.path.join(self.tmpdir, "cache")
        # Use a neutral username
        self.env["USERNAME"] = "testuser"
        # Force large enough window size because UI elements are only visible
        # if there is enough space for them.
        self.env["LIBREPCB_WINDOW_SIZE"] = "1600x900"
        # Force LibrePCB to use Qt-style file dialogs because native dialogs don't work
        self.env["LIBREPCB_DISABLE_NATIVE_DIALOGS"] = "1"
        # Disable warning about unstable file format, since tests are run also
        # on the (unstable) master branch
        self.env["LIBREPCB_DISABLE_UNSTABLE_WARNING"] = "1"

    def abspath(self, relpath):
        return os.path.join(self.tmpdir, relpath)

    def get_data_path(self, relpath):
        return os.path.join(DATA_DIR, relpath)

    def set_workspace(self, path):
        if not os.path.isabs(path):
            path = self.abspath(path)
        self.workspace_path = path

    def add_project(self, project, as_lppz=False):
        src = os.path.join(DATA_DIR, "projects", project)
        dst = os.path.join(self.tmpdir, project)
        if as_lppz:
            shutil.make_archive(dst, "zip", src)
            shutil.move(dst + ".zip", dst + ".lppz")
        else:
            shutil.copytree(_long_path(src), _long_path(dst))

    def set_project(self, path):
        if not os.path.isabs(path):
            path = self.abspath(path)
        self.project_path = path

    def get_workspace_libraries_path(self, subdir=""):
        return os.path.join(self.workspace_path, "data", "libraries", subdir)

    def add_local_library_to_workspace(self, path):
        if not os.path.isabs(path):
            path = os.path.join(DATA_DIR, path)
        dest = self.get_workspace_libraries_path("local")
        dest = os.path.join(dest, os.path.basename(path))
        shutil.copytree(_long_path(path), _long_path(dest))

    def open(self):
        self._create_application_config_file()
        return Application([self.executable] + self._args(), env=self.env)

    def _create_application_config_file(self):
        org_dir = "LibrePCB.org" if platform.system() == "Darwin" else "LibrePCB"
        config_dir = os.path.join(self.tmpdir, "config", org_dir)
        config_ini = os.path.join(config_dir, "LibrePCB.ini")
        if not os.path.exists(config_dir):
            os.makedirs(config_dir)
        # Only create config file once per test, so tests can check if settings
        # are stored permanently.
        if not os.path.exists(config_ini):
            with open(config_ini, "w") as f:
                if self.workspace_path:
                    f.write("[workspaces]\n")
                    f.write(
                        'most_recently_used="{}"\n'.format(
                            self.workspace_path.replace("\\", "/")
                        )
                    )

    def _args(self):
        args = []
        if self.project_path:
            args.append(self.project_path)
        return args


class Helpers(object):
    @staticmethod
    def wait_for_file_exists(path, timeout=10.0):
        start = time.time()
        while True:
            if os.path.exists(path):
                return
            if time.time() > (start + timeout):
                raise TimeoutError(f"File does not exist: {path}")
            time.sleep(0.02)

    @staticmethod
    def wait_for_directories(parent, directories, timeout=10.0):
        start = time.time()
        directories = set(directories)
        dirs = None
        while True:
            if os.path.exists(parent):
                dirs = set(os.listdir(parent))
                if dirs == directories:
                    return
            if time.time() > (start + timeout):
                raise TimeoutError(
                    f"Expected directories {directories}, but found {dirs}."
                )
            time.sleep(0.02)


@pytest.fixture(scope="session")
def librepcb_server():
    """
    Fixture which provides a HTTP server at localhost:8080

    All tests should use this server instead of the official LibrePCB API server
    or GitHub for downloading libraries.
    """
    import time
    import threading
    import socket
    import socketserver
    import http.server

    class Handler(http.server.SimpleHTTPRequestHandler, object):
        def translate_path(self, path):
            path = super(Handler, self).translate_path(path)
            relpath = os.path.relpath(path, os.curdir)
            return os.path.join(DATA_DIR, "server", relpath)

    # Set SO_REUSEADDR option to avoid "port already in use" errors
    httpd = socketserver.TCPServer(("", 50080), Handler, bind_and_activate=False)
    httpd.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    httpd.server_bind()
    httpd.server_activate()
    thread = threading.Thread(target=httpd.serve_forever)
    thread.daemon = True
    thread.start()
    time.sleep(0.2)  # wait a bit to make sure the server is ready


@pytest.fixture(scope="session")
def check_resolution(request, tmp_path_factory):
    """
    Fixture which just fails the complete session if the screen resolution is
    not sufficient (some tests require a large enough screen)
    """
    path = tmp_path_factory.mktemp("check_resolution")
    with LibrePcbFixture(request.config, path).open() as app:
        min_size = (1024, 720)
        win_size = app.first_window.size
        if (win_size[0] < min_size[0]) or (win_size[1] < min_size[1]):
            pytest.exit(
                f"Screen resolution is {win_size}, requires minimum {min_size}.",
                returncode=1,
            )


@pytest.fixture
def create_librepcb(request, tmpdir, librepcb_server, check_resolution):
    """
    Fixture allowing to create multiple application instances
    """

    def _create():
        return LibrePcbFixture(request.config, str(tmpdir))

    return _create


@pytest.fixture
def librepcb(create_librepcb):
    """
    Fixture allowing to create one application instance
    """
    yield create_librepcb()


@pytest.fixture(scope="session")
def helpers():
    """
    Fixture providing some helper functions
    """
    return Helpers()

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import platform
import pytest
import shutil
import slint_testing
import time
from copy import deepcopy


# Avoid errors due to too long file path on Windows :-|
def _long_path(path):
    if os.name == 'nt':
        return '\\\\?\\' + path.replace('/', '\\')
    else:
        return path


ROOT_DIR = os.path.dirname(__file__)
TESTS_DIR = os.path.dirname(ROOT_DIR)
REPO_DIR = os.path.dirname(TESTS_DIR)
DATA_DIR = os.path.join(TESTS_DIR, 'data')


def pytest_addoption(parser):
    parser.addoption("--librepcb-executable",
                     action="store",
                     help="Path to librepcb executable to test")


def _query_childs(element, query):
    segment = query[0].split('[')
    segment_name = segment[0]
    if segment_name.startswith('#'):
        segment_name = segment_name[1:]
        childs = element.query_descendants().match_type_name(segment_name).find_all()
    else:
        childs = element.query_descendants().match_id(segment_name).find_all()
    if len(segment) > 1:
        index = int(segment[1][:-1])
        childs = [childs[index]]
    if len(query) > 1:
        childs = [_query_childs(child, query[1:]) for child in childs]
    if len(childs) == 0:
        childs = None
    elif len(childs) == 1:
        childs = childs[0]
    return childs


class ElementQuery:
    def __init__(self, app, window, parent=None, query=None):
        self._app = app
        self._window = window
        self._parent = parent
        self._query = query or []
        self._result = None

    def get(self, path):
        return ElementQuery(self._app, self._window, self, path.split(' '))

    @property
    def full_query(self):
        if self._parent is None:
            return self._query
        else:
            return self._parent.full_query + self._query

    @property
    def is_valid(self):
        self._update()
        return self._result.is_valid if self._result else False

    @property
    def label(self):
        self._update()
        self._check_result()
        return self._result.accessible_label

    def click(self):
        self._update()
        self._check_result()
        self._result.single_click(slint_testing.PointerEventButton.Left)

    def dclick(self):
        self._update()
        self._check_result()
        self._result.double_click(slint_testing.PointerEventButton.Left)

    def set_value(self, value):
        self._update()
        self._check_result()
        self._result.accessible_value = value

    def set_checked(self, checked):
        self._update()
        self._check_result()
        if self._result.accessible_checked != checked:
            self._result.invoke_accessible_default_action()

    def _update(self):
        if not self._result or not self._result.is_valid:
            root = self._app.windows[self._window].root_element
            if self._parent is None:
                self._result = root
            else:
                self._parent._update()
                self._parent._check_result()
                self._result = _query_childs(self._parent._result, self._query)

    def _check_result(self):
        if not self.is_valid:
            raise Exception("Element does not exist: {}".format(self.full_query))
        elif type(self.is_valid) is list:
            raise Exception("Query returned {} results: {}".format(len(self._result), self.full_query))



class Application(slint_testing.Application):
    def root(self, window=0):
        return ElementQuery(self, window)

    def get(self, path, window=0):
        return self.root(window=window).get(path)


class LibrePcbFixture(object):
    def __init__(self, config, tmpdir):
        super(LibrePcbFixture, self).__init__()
        self.executable = os.path.abspath(config.getoption('--librepcb-executable'))
        if not os.path.exists(self.executable):
            raise Exception("Executable '{}' not found. Please pass it with "
                            "'--librepcb-executable'.".format(self.executable))
        self.tmpdir = tmpdir
        # Copy test data to temporary directory to avoid modifications in original data
        shutil.copytree(_long_path(os.path.join(DATA_DIR, 'workspaces', 'Empty Workspace')),
                        _long_path(os.path.join(self.tmpdir, 'Empty Workspace')))
        # Init members to default values
        self.workspace_path = os.path.join(self.tmpdir, 'Empty Workspace')
        self.project_path = None

        # Set environment variables
        self.env = deepcopy(os.environ)
        # Make GUI independent from the system's language
        self.env['LC_ALL'] = 'C'
        # Override configuration location to make tests independent of existing configs
        self.env['LIBREPCB_CONFIG_DIR'] = os.path.join(self.tmpdir, 'config')
        # Override cache location to make each test indepotent
        self.env['LIBREPCB_CACHE_DIR'] = os.path.join(self.tmpdir, 'cache')
        # Use a neutral username
        self.env['USERNAME'] = 'testuser'
        # Force LibrePCB to use Qt-style file dialogs because native dialogs don't work
        self.env['LIBREPCB_DISABLE_NATIVE_DIALOGS'] = '1'
        # Disable warning about unstable file format, since tests are run also
        # on the (unstable) master branch
        self.env['LIBREPCB_DISABLE_UNSTABLE_WARNING'] = '1'

    def abspath(self, relpath):
        return os.path.join(self.tmpdir, relpath)

    def get_data_path(self, relpath):
        return os.path.join(DATA_DIR, relpath)

    def set_workspace(self, path):
        if not os.path.isabs(path):
            path = self.abspath(path)
        self.workspace_path = path

    def add_project(self, project, as_lppz=False):
        src = os.path.join(DATA_DIR, 'projects', project)
        dst = os.path.join(self.tmpdir, project)
        if as_lppz:
            shutil.make_archive(dst, 'zip', src)
            shutil.move(dst + '.zip', dst + '.lppz')
        else:
            shutil.copytree(_long_path(src), _long_path(dst))

    def set_project(self, path):
        if not os.path.isabs(path):
            path = self.abspath(path)
        self.project_path = path

    def get_workspace_libraries_path(self, subdir=''):
        return os.path.join(self.workspace_path, 'data', 'libraries', subdir)

    def add_local_library_to_workspace(self, path):
        if not os.path.isabs(path):
            path = os.path.join(DATA_DIR, path)
        dest = self.get_workspace_libraries_path('local')
        dest = os.path.join(dest, os.path.basename(path))
        shutil.copytree(_long_path(path), _long_path(dest))

    def open(self):
        self._create_application_config_file()
        return Application([self.executable] + self._args(), env=self.env)

    def _create_application_config_file(self):
        org_dir = 'LibrePCB.org' if platform.system() == 'Darwin' else 'LibrePCB'
        config_dir = os.path.join(self.tmpdir, 'config', org_dir)
        config_ini = os.path.join(config_dir, 'LibrePCB.ini')
        if not os.path.exists(config_dir):
            os.makedirs(config_dir)
        # Only create config file once per test, so tests can check if settings
        # are stored permanently.
        if not os.path.exists(config_ini):
            with open(config_ini, 'w') as f:
                if self.workspace_path:
                    f.write("[workspaces]\n")
                    f.write("most_recently_used=\"{}\"\n".format(self.workspace_path.replace('\\', '/')))

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
                raise TimeoutError("File does not exist: {}".format(path))
            time.sleep(0.1)

    @staticmethod
    def wait_for_element_invalid(element, timeout=10.0):
        start = time.time()
        while True:
            if not element.is_valid:
                return
            if time.time() > (start + timeout):
                raise TimeoutError("Element is still valid: {}".format(element.id))
            time.sleep(0.1)

    @staticmethod
    def wait_for_project(app, name):
        docs_panel = app.first_window.root_element.query_descendants().match_id("AppWindow::documents-panel").find_first()
        for project in docs_panel.query_descendants().match_id("DocumentsPanel::project-item").find_all():
            header = project.query_descendants().match_id("ProjectSection::header").find_first()
            if header.accessible_label == name.upper():
                return
        raise Exception('Failed to wait for project "{}"!'.format(name))


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
            return os.path.join(DATA_DIR, 'server', relpath)

    # Set SO_REUSEADDR option to avoid "port already in use" errors
    httpd = socketserver.TCPServer(("", 50080), Handler, bind_and_activate=False)
    httpd.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    httpd.server_bind()
    httpd.server_activate()
    thread = threading.Thread(target=httpd.serve_forever)
    thread.daemon = True
    thread.start()
    time.sleep(0.2)  # wait a bit to make sure the server is ready


@pytest.fixture
def create_librepcb(request, tmpdir, librepcb_server):
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

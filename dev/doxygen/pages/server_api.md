Server API Specification {#doc_server_api}
==========================================

[TOC]

LibrePCB provides an API at [api.librepcb.org](https://api.librepcb.org) which
is used by the LibrePCB application, for example to fetch the latest libraries.

The API uses JSON for responses.


# URL {#doc_server_api_url}

An API URL consists of following parts:

| Part          | Description                    | Example                  |
|---------------|--------------------------------|--------------------------|
| `<BASE>`      | Base URL of API server         | https://api.librepcb.org |
| `/api`        | Base path of API               | `/api`                   |
| `/<VERSION>`  | API version number             | `/v1`                    |
| `/<RESOURCE>` | Path to the requested resource | `/libraries/v0.1`        |

The base URL of the official API server is https://api.librepcb.org/, but for
your own API server you can use any other base URL. This is the URL which must
be specified in the workspace settings of LibrePCB.

Currently the only available API version is `v1`, but in future there might
be newer API specifications available with higher version numbers. Because the
version number is part of the URL, a server could support multiple API versions
at the same time.

## Example

**Request the list of libraries:**

~~~{.sh}
curl 'https://api.librepcb.org/api/v1/libraries/v0.1'
~~~


# Pagination {#doc_server_api_pagination}

The API uses pagination (i.e. splitting large responses across several shorter
responses) to keep the response time of requests short.

A paginated response contains following JSON entries:

| Key       | Type      | Description                                  |
|-----------|-----------|----------------------------------------------|
| count     | integer   | Total count of results over all pages        |
| next      | string    | URL of next page (`null` for last page)      |
| previous  | string    | URL of previous page (`null` for first page) |
| results   | list      | The actual response objects                  |

## Example

Here an example how a paginated response looks like:

**Request:**

~~~{.sh}
curl 'https://api.librepcb.org/api/v1/libraries/v0.1'
~~~

**Response:**

~~~{.json}
{
  "count": 42,
  "next": "https://api.librepcb.org/api/v1/libraries/v0.1?page=2",
  "previous": null,
  "results": [
    ...
  ]
}
~~~


# Available Resources {#doc_server_api_resources}

Following resources are available:

| Path         | Description                           |
|--------------|---------------------------------------|
| [/libraries] | Fetch list of available libraries     |
| [/order]     | Upload a project to start ordering it |


## Libraries {#doc_server_api_resources_libraries}

The resource path `/libraries` is used to fetch the list of all available
libraries. It is used in the LibrePCB Library Manager to list and download
libraries.

The file format version of the application needs to be appended to the path
to fetch only libraries with compatible file format. For example if the
application uses file format version `0.1`, use the path
`/api/v1/libraries/v0.1`. Note that the response can still contain libraries
with a file format older than the specified one. But it will never contain
libraries with a newer file format.

The response is a list of objects with following properties:

| Name                  | Type    | Description                                           |
|-----------------------|---------|-------------------------------------------------------|
| uuid                  | string  | UUID of the library (from `library.lp`)               |
| name                  | object  | Library name, multilingual (from `library.lp`)        |
| description           | object  | Library description, multilingual (from `library.lp`) |
| keywords              | object  | Library keywords, multilingual (from `library.lp`)    |
| author                | string  | Author of the library (from `library.lp`)             |
| version               | string  | Version of the library (from `library.lp`)            |
| deprecated            | boolean | Deprecated flag (from `library.lp`)                   |
| url                   | string  | URL of library (from `library.lp`)                    |
| dependencies          | list    | List of dependent library UUIDs (from `library.lp`)   |
| component_categories  | integer | Count of contained component categories               |
| package_categories    | integer | Count of contained package categories                 |
| symbols               | integer | Count of contained symbols                            |
| packages              | integer | Count of contained packages                           |
| components            | integer | Count of contained components                         |
| devices               | integer | Count of contained devices                            |
| recommended           | boolean | Whether the library is recommended to install         |
| updated_at            | string  | [ISO 8601] datetime of last update                    |
| format_version        | string  | File format of the library (from `.librepcb-lib`)     |
| icon_url              | string  | URL to library icon (`null` if N/A)                   |
| download_url          | string  | URL to file for downloading the library (*.zip)       |
| download_size         | integer | Size of file to download in bytes (`null` if unknown) |
| download_sha256       | string  | SHA256 of file to download (`null` if unknown)        |

### Example

**Request:**

~~~{.sh}
curl 'https://api.librepcb.org/api/v1/libraries/v0.1'
~~~

**Response (without pagination):**

~~~{.json}
[
  {
    "uuid": "e3ce49bf-e959-4092-8b48-aeead756f8d1",
    "name": {
      "default": "Molex",
      "en_US": "Molex"
    },
    "description": {
      "default": "Connectors, Sockets and more"
    },
    "keywords": {
      "default": "connector,molex"
    },
    "author": "LibrePCB",
    "version": "0.0.1",
    "deprecated": false,
    "url": "https://github.com/LibrePCB-Libraries/Molex.lplib",
    "dependencies": [
      "a9ddf0c6-9b1c-4730-b300-01b4f192ad40",
      "6ccc516c-21b7-4cd5-9cf2-7a04cfa361c6"
    ],
    "component_categories": 13,
    "package_categories": 37,
    "symbols": 123,
    "packages": 321,
    "components": 42,
    "devices": 0,
    "recommended": false,
    "updated_at": "2018-04-13T12:43:52Z",
    "format_version": "0.1",
    "icon_url": "https://github.com/LibrePCB-Libraries/Molex.lplib/raw/master/library.png",
    "download_url": "https://github.com/LibrePCB-Libraries/Molex.lplib/archive/master.zip",
    "download_size": null,
    "download_sha256": null,
  }
]
~~~

## Order PCB {#doc_server_api_resources_order}

The resource path `/order` is used to upload a LibrePCB project to start
ordering the PCB. After the project has been uploaded, the order process
needs to be continued in the web browser.

The client has to initiate the order with a GET request to the path `/order`.
The response is a JSON object with the following data (no pagination used):

| Name       | Type    | Description                                               |
|------------|---------|-----------------------------------------------------------|
| info_url   | string  | URL pointing to service information (e.g. privacy policy) |
| upload_url | string  | URL where the project has to be uploaded                  |
| max_size   | integer | Maximum allowed size (in bytes) of the uploaded project   |

*Notes:*
- *The `info_url` should be short and descriptive to allow displaying it as-is
  in the LibrePCB GUI.*
- *If the service is (temporarily) not available, `upload_url` can be set to
  an empty string or `null`. LibrePCB shall then display a message like
  "Service is currently not available, please try again later".*
- *The `max_size` applies to the raw \*.lppz file **without** base64 encoding,
  so the actually uploaded JSON object can be much larger. LibrePCB shall not
  start the upload if the project is larger than this value.*

Afterwards, the client has to make a POST request to the received `upload_url`
with a JSON object containing the following data:

| Name    | Type   | Description                                                 |
|---------|--------|-------------------------------------------------------------|
| project | string | The whole project as a Base64 encoded `*.lppz` archive      |
| board   | string | The filepath of the board to be ordered (`null` if unknown) |

The response is a JSON object with the following data (no pagination used):

| Name         | Type    | Description                                          |
|--------------|---------|------------------------------------------------------|
| redirect_url | string  | URL to continue the order process in the web browser |

### Example

**Initial Request:**

~~~{.sh}
curl -H "Content-Type: application/json" \
     'https://api.librepcb.org/api/v1/order'
~~~

**Initial Response:**

~~~{.json}
{
  "info_url": "https://fab.librepcb.org/about",
  "upload_url": "https://fab.librepcb.org/upload",
  "max_size": 100000000
}
~~~

**Upload Request:**

~~~{.sh}
curl -X POST -H "Content-Type: application/json" \
     -d '{"project": "...", "board": "boards/default/board.lp"}' \
     'https://fab.librepcb.org/upload'
~~~

**Upload Response:**

~~~{.json}
{
  "redirect_url": "https://fab.librepcb.org/nnwyw55pA0Z0sw/"
}
~~~

[/libraries]: @ref doc_server_api_resources_libraries
[/order]: @ref doc_server_api_resources_order
[ISO 8601]: https://en.wikipedia.org/wiki/ISO_8601

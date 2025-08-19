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


# Localization {#doc_server_api_localization}

In order to get the response in the user's language, the client may set the
HTTP header `Accept-Language` to the desired locale string (e.g. `de_DE`).


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

| Path         | Description                                  |
|--------------|----------------------------------------------|
| [/info]      | Get general information about the API server |
| [/libraries] | Fetch list of available libraries            |
| [/order]     | Upload a project to start ordering it        |
| [/parts]     | Request live information about parts         |


## Info {#doc_server_api_resources_info}

The resource path `/info` provides some general information about the API
server or about the upstream LibrePCB project.

One purpose of this resource is to fetch information about the latest released
LibrePCB version (version number, download URLs etc.).

This resource also contains information about the capabilities of the API
server, for example which resource endpoints are implemented. Calling this
endpoint avoids that clients require to call every resource endpoint
individually (and possibly ending up in HTTP errors). It is recommended that
clients check this endpoint e.g. once per day before trying to access any other
API resource.

The response is an object with following properties:

| Name                   | Type    | Description                                           |
|------------------------|---------|-------------------------------------------------------|
| stable_version  | string  | Version number of the latest stable LibrePCB release  |
| preview_version | string  | Version number of the latest preview LibrePCB release |
| stable_url_appimage
| libraries       | `null` or `{}` | Whether the `/libraries` endpoint is supported |
| order           | `null` or `{}` | Whether the `/order` endpoint is supported     |
| parts           | `null` or `{}` | Whether the `/parts` endpoint is supported     |

*Notes:*
- *Both `stable_version` and `preview_version` shall contain the
  version number exactly as specified in `CMakeLists.txt` (e.g. `1.2.3-rc4`).*
- *If there is currently no preview version available, the whole
  `preview_version` property should be omitted from the response*
- The resource capabilities (`libraries`, `order`, `parts`) are usually just
  empty objects (which means the resource is supported) but may be extended
  with properties in future. Unsupported resources may either not appear
  in the response at all (preferred), or as `null` values.*

The `latest_stable` and `latest_preview` objects contain the following
properties:

| Name                   | Type    | Description                                           |
|------------------------|---------|-------------------------------------------------------|
| version  | string  | Version number of the latest stable LibrePCB release  |
| changelog_url | string | URL to the release notes / changelog |
| appimage_url | string  | |
| url_appimage | string  | |
| url_appimage | string  | |

### Example

**Request:**

~~~{.sh}
curl 'https://api.librepcb.org/api/v1/info'
~~~

**Response:**

~~~{.json}
{
  "stable_version": {
    "version": "1.3.0",
    "release_date": "2025-08-15",
    "changelog_url": "https://librepcb.org/blog/2025-03-24_release_1.3.0/",
    "download_url": "https://librepcb.org/download/",
    "linux_x86_64_appimage": {
      "requires": {"libc": "2.28"},
      "url": "https://.../librepcb-1.3.0-linux-x86_64.AppImage",
      "size": 52428800,
      "sha256": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4"
    },
    "linux_x86_64_tgz": {
      "requires": {"libc": "2.28"},
      "url": "https://.../librepcb-1.3.0-linux-x86_64.tar.gz",
      "size": 52428800,
      "sha256": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4"
    },
    "mac_x86_64_dmg": {
      "requires": {"macos": "11.2"},
      "url": "https://.../librepcb-1.3.0-mac-x86_64.dmg",
      "size": 52428800,
      "sha256": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4"
    },
    "mac_arm64_dmg": {
      "requires": {"macos": "13.0"},
      "url": "https://.../librepcb-1.3.0-mac-arm64.dmg",
      "size": 52428800,
      "sha256": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4"
    },
    "windows_x86_64_zip": {
      "requires": {"windows": "10.0"},
      "url": "https://.../librepcb-1.3.0-windows-x86_64.zip",
      "size": 52428800,
      "sha256": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4"
    },
    "windows_x86_64_innosetup": {
      "requires": {"windows": "10.0"},
      "url": "https://.../librepcb-installer-1.3.0-windows-x86_64.exe",
      "size": 52428800,
      "sha256": "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4"
    }
  },
  "news": [
    {
      "date": "2025-05-20T00:00:00Z",
      "title": "Library Updates Q1 2025",
      "url": "https://librepcb.org/blog/2025-05-20_library_updates/"
    }
  ],
  "libraries": {},
  "order": null,
  "parts": {}
}
~~~


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

## Parts Information {#doc_server_api_resources_parts}

The resource path `/parts` is used to request live information about concrete
parts (by MPN), for example prices or stock availability. The availability
of this API depends on the cooperation with a corresponding data provider
since the LibrePCB project cannot maintain such a parts database. If there is
no such cooperation, this API will not be available.

The client has to initiate the query with a GET request to the path `/parts`.
The response is a JSON object with the following data (no pagination used):

| Name                       | Type    | Description                                               |
|----------------------------|---------|-----------------------------------------------------------|
| provider_name              | string  | Name of the data provider                                 |
| provider_url               | string  | URL to the data providers website                         |
| provider_logo_url          | string  | URL to the data providers logo (light theme, optional)    |
| ~~provider_logo_dark_url~~ | string  | URL to the data providers logo (dark theme, optional)     |
| info_url                   | string  | URL pointing to service information (e.g. privacy policy) |
| query_url                  | string  | URL where to post the actual queries                      |
| max_parts                  | integer | Maximum number of parts to query in one request           |

*Notes:*
- *If the service is (temporarily) not available, an empty JSON object can
  be returned. The client shall detect this by the property `query_url` which
  may be missing, `null` or an empty string, and then either display a message
  like "No part information available" or just silently don't display any
  part information.*
- The `provider_logo_dark_url` key is currently only a proposal, not officially
  part of the API.
- *The `info_url` should be short and descriptive to allow displaying it as-is
  in the LibrePCB GUI.*
- *The `max_parts` specifies how many parts are allowed to be queried in a
  single request. However, clients should apply their own limit which is
  smaller or equal to the value returned here.*

Afterwards, the client has to make POST requests to the received `query_url`
with a JSON object containing the following data:

| Name       | Type    | Description                                           |
|------------|---------|-------------------------------------------------------|
| parts      | array   | Array of parts to ask for information                 |

Where the `parts` array shall consist of 0..n JSON objects with the following
properties:

| Name         | Type    | Description                                         |
|--------------|---------|-----------------------------------------------------|
| mpn          | string  | Manufacturer part number                            |
| manufacturer | string  | Manufacturer name of the part                       |

The response is a JSON object with the following data (no pagination used):

| Name              | Type    | Description                                    |
|-------------------|---------|------------------------------------------------|
| parts             | array   | Array with an entry for each requested part    |

Where the `parts` array consists of JSON objects with the following properties:

| Name            | Type    | Description                                                |
|-----------------|---------|------------------------------------------------------------|
| mpn             | string  | Manufacturer part number (copied from request)             |
| manufacturer    | string  | Manufacturer name of the part (copied from request)        |
| results         | integer | Number of parts found for the given MPN/manufacturer       |
| product_url     | string  | URL to the manufacturers part information page             |
| picture_url     | string  | URL to a picture of the part (e.g. PNG or JPEG)            |
| pricing_url     | string  | URL to pricing information across suppliers                |
| status          | string  | Either Preview, Active, NRND or Obsolete (to be extended)  |
| availability    | integer | -10=VeryBad, -5=Bad, 0=Normal, 5=Good, 10=VeryGood         |
| prices          | array   | Part price for various quantities                          |
| resources       | array   | Part resources, e.g. datasheets and reference manuals      |
| ~~suggestions~~ | array   | Alternative part suggestions (if obsolete or `results!=1`) |

Where the `prices` array consists of JSON objects with the following
properties:

| Name     | Type    | Description                                                 |
|----------|---------|-------------------------------------------------------------|
| quantity | integer | Quantity for which this price is valid                      |
| price    | float   | Part price in USD for 1 piece, valid for the given quantity |

And the `resources` array consists of JSON objects with the following
properties:

| Name      | Type    | Description                                                   |
|-----------|---------|---------------------------------------------------------------|
| name      | string  | Name of the resource (e.g. "Datasheet")                       |
| mediatype | string  | Type of the resource linked by `url` (e.g. `application/pdf`) |
| url       | string  | Direct URL to the resource                                    |

The `suggestions` key is only a proposal, not officially part of the API yet.
It may be added later as an array which consists of JSON objects with the
following properties:

| Name         | Type    | Description                                         |
|--------------|---------|-----------------------------------------------------|
| mpn          | string  | MPN of a suggested alternative part                 |
| manufacturer | string  | Manufacturer name of the suggested part             |

*Notes:*
- *Except for `mpn`, `manufacturer` and `results`, each property of the
  response part objects may be omitted or set to `null` to indicate the
  corresponding information is not available.*
- *If `results` is not `1`, no part information shall be contained in the
  response.*
- *Since no pagination is used, it is the responsibility of the client to
  avoid too large responses by limiting the number of parts in the request.
  Instead of requesting many parts at once, several smaller requests should
  be made. The `max_parts` value returned in the status response shall be
  respected as the upper limit.*
- *To avoid too much traffic & server load, clients shall always wait for
  the response before sending the next request, i.e. parallel requests
  shall not be made! This also applies to large requests split into several
  smaller requests.*

### Example

**Initial Request:**

~~~{.sh}
curl -H "Content-Type: application/json" \
     'https://api.librepcb.org/api/v1/parts'
~~~

**Initial Response:**

~~~{.json}
{
  "provider_name": "LibrePCB Data Provider",
  "provider_url": "https://librepcb-data-provider.com",
  "provider_logo_url": "https://librepcb-data-provider.com/logo.png",
  "info_url": "https://api.librepcb.org",
  "query_url": "https://api.librepcb.org/api/v1/parts/query",
  "max_parts": 10
}
~~~

**Query Request:**

~~~{.json}
{
  "parts": [
    {
      "mpn": "1N4148",
      "manufacturer": "Texas Instruments"
    }
  ]
}
~~~

~~~{.sh}
curl -X POST -H "Content-Type: application/json" -d @request.json \
     'https://api.librepcb.org/api/v1/parts/query'
~~~

**Query Response:**

~~~{.json}
{
  "parts": [
    {
      "mpn": "1N4148",
      "manufacturer": "Texas Instruments",
      "results": 1,
      "product_url": "https://ti.com/1n4148/",
      "picture_url": "https://ti.com/1n4148/picture.png",
      "pricing_url": "https://librepcb-data-provider.com/1n4148/",
      "status": "Obsolete",
      "availability": 5,
      "prices": [
        {
          "quantity": 1,
          "price": 0.01
        },
        {
          "quantity": 10,
          "price": 0.009
        },
        {
          "quantity": 1000,
          "price": 0.008
        }
      ],
      "resources": [
        {
          "name": "Datasheet",
          "mediatype": "application/pdf",
          "url": "https://ti.com/1n4148/datasheet.pdf"
        }
      ]
    }
  ]
}
~~~

[/libraries]: @ref doc_server_api_resources_libraries
[/order]: @ref doc_server_api_resources_order
[/parts]: @ref doc_server_api_resources_parts
[ISO 8601]: https://en.wikipedia.org/wiki/ISO_8601

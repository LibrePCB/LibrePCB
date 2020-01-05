#!/bin/bash
set -euo pipefail

# Configuration
SCHEMACRAWLER_VER="16.3.0"
SCHEMACRAWLER_URL="https://github.com/schemacrawler/SchemaCrawler/releases/download/v${SCHEMACRAWLER_VER}/schemacrawler-${SCHEMACRAWLER_VER}-distribution.zip"
SCHEMACRAWLER_ZIP="schemacrawler-${SCHEMACRAWLER_VER}.zip"
SCHEMACRAWLER_DIR="schemacrawler-${SCHEMACRAWLER_VER}-distribution"
OUT_FILE="doxygen/images/database_diagram.png"

# Parse arguments
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <path-to-database.sqlite>"
  exit 1
fi

# Determine db path
DATABASE_PATH="$(realpath "$1")"
if [ ! -f "$DATABASE_PATH" ]; then
  echo "Database \"$DATABASE_PATH\" not found"
  exit 2
fi

# Download
if [ ! -f "$SCHEMACRAWLER_ZIP" ]; then
  echo "Downloading schemacrawler $SCHEMACRAWLER_VER..."
  curl -L -o "$SCHEMACRAWLER_ZIP" "$SCHEMACRAWLER_URL"
else
  echo "Found schemacrawler $SCHEMACRAWLER_ZIP"
fi

# Unpack
if [ ! -d "$SCHEMACRAWLER_DIR" ]; then
  echo "Unpacking $SCHEMACRAWLER_ZIP"
  unzip -q "$SCHEMACRAWLER_ZIP"
else
  echo "Found $SCHEMACRAWLER_DIR"
fi

# Run
echo -e "\nProcessing database $DATABASE_PATH"
echo "--- Begin schemacrawler log ---"
"$SCHEMACRAWLER_DIR/_schemacrawler/schemacrawler.sh" \
  --server=sqlite \
  --database="$DATABASE_PATH" \
  --user="none" \
  --password="none" \
  --info-level=standard \
  --command=schema \
  --output-format=png \
  --log-level=WARNING \
  --title="LibrePCB Library Cache Database Schema" \
  -o "$OUT_FILE"
echo "--- End schemacrawler log ---"

# Compress
echo -e "\nCompressing generated PNG"
echo "--- Begin optipng log ---"
optipng -o5 "$OUT_FILE"
echo "--- End optipng log ---"

echo -e "\nDone! Generated $OUT_FILE"

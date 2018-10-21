#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -eu -o pipefail

# get absolute path to installer root directory
ROOT="$( cd "$(dirname "$0")" ; pwd -P )"

# get cli parameters
TARGET_NAME="$1"
APP_VERSION="$2"

echo "Updating installer packages for $TARGET_NAME v$APP_VERSION..."

# determine more parameters
DESTINATION="$ROOT/output"
DATE="`date +%Y-%m-%d`"

# OS specific parameters
if [[ "$TARGET_NAME" == windows* ]]
then
  DEFAULT_TARGET_DIR="@ApplicationsDirX86@/LibrePCB"
elif [[ "$TARGET_NAME" == linux* ]]
then
  DEFAULT_TARGET_DIR="@homeDir@/LibrePCB"
elif [[ "$TARGET_NAME" == mac* ]]
then
  DEFAULT_TARGET_DIR="@homeDir@/Applications/LibrePCB"
fi

# copy to destination directory
echo "Copy data to $DESTINATION..."
mkdir -p $DESTINATION
cp -rf $ROOT/config $DESTINATION/
cp -rf $ROOT/packages $DESTINATION/

# copy additional files
echo "Copy additional files to $DESTINATION/config..."
cp $ROOT/../../img/logo/*.png $DESTINATION/config/
cp $ROOT/../../LICENSE.txt $DESTINATION/packages/librepcb.stable.app/meta/license_gplv3.txt
REGFILEEXT_DIR=$DESTINATION/packages/librepcb.registerfileextensions/data/registerfileextensions
mkdir -p $REGFILEEXT_DIR/mime
cp $ROOT/../../share/mime/packages/org.librepcb.LibrePCB.xml $REGFILEEXT_DIR/mime/librepcb-from-installer.xml

# remove packages which are not supported by the selected target
if [[ "$TARGET_NAME" == mac* ]]
then
  echo "Remove package 'librepcb.registerfileextensions'..."
  rm -rf "$DESTINATION/packages/librepcb.registerfileextensions"
fi

# replace specific placeholders in a given file
function replace {
  FILE="$1"
  FROM=$(sed 's/[^^]/[&]/g; s/\^/\\^/g' <<< "$2" )
  TO=$(sed 's/[\/&]/\\&/g' <<< "$3" )
  sed -i -e "s/$FROM/$TO/g" "$FILE"
}

# replace placeholders in all *.xml files
for f in $(find "$DESTINATION" -name '*.xml')
do
  echo "Update metadata of $f..."
  replace "$f" "TARGET_NAME"            "$TARGET_NAME"
  replace "$f" "DEFAULT_TARGET_DIR"     "$DEFAULT_TARGET_DIR"
  replace "$f" "APP_VERSION"            "$APP_VERSION"
  replace "$f" "RELEASE_DATE"           "$DATE"
done

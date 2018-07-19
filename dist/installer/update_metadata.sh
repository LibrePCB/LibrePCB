#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -eu -o pipefail

# get absolute path to installer root directory
ROOT="$( cd "$(dirname "$0")" ; pwd -P )"

# get cli parameters
TARGET_NAME="$1"
APP_VERSION_FULL="$2"

echo "Updating installer packages for $TARGET_NAME v$APP_VERSION_FULL..."

# determine more parameters
DESTINATION="$ROOT/output"
APP_VERSION_SHORT="${APP_VERSION_FULL%.0}"  # remove patch version if it is zero
COMMIT_COUNT="`git -C "$ROOT" rev-list --count HEAD`"
DATE="`date +%Y-%m-%d`"

# make sure the whole Git history is available (if not the whole history is
# available, the generated version number is wrong!)
if (( $COMMIT_COUNT < 1200 ))
then
  echo "ERROR: Too less commits found! Repository cloned with a limited depth?"
  exit 1
fi

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
cp $ROOT/../../LICENSE.txt $DESTINATION/packages/librepcb.nightly.app/meta/license_gplv3.txt
REGFILEEXT_DIR=$DESTINATION/packages/librepcb.registerfileextensions/data/registerfileextensions
mkdir -p $REGFILEEXT_DIR/mime
cp $ROOT/../../share/mime/packages/librepcb.xml $REGFILEEXT_DIR/mime/librepcb-from-installer.xml

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
  replace "$f" "APP_VERSION_FULL"       "$APP_VERSION_FULL"
  replace "$f" "APP_VERSION_SHORT"      "$APP_VERSION_SHORT"
  replace "$f" "APP_VERSION_NIGHTLY"    "$APP_VERSION_FULL-$COMMIT_COUNT"
  replace "$f" "RELEASE_DATE"           "$DATE"
done

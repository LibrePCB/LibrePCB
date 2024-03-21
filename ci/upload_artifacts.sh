#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Bash on Windows calls the Windows provided "tar" and "openssl" tool instead
# of the one from MSYS, which is bullshit since it isn't compatible. As a
# workaround, we adjust PATH.
export PATH="/usr/bin:$PATH"

# get branch name from Azure
BRANCH_NAME="${AZURE_BRANCH_NAME#refs/heads/}"

# upload build artifacts for all branches of the upstream repository
if [ -n "${UPLOAD_URL-}" ]
then
  # create tarball of all artifacts
  cd ./artifacts
  tar -cf artifacts.tar *
  # create digital signature of artifacts tarball
  openssl dgst -sha256 -sign <(echo -e "$UPLOAD_SIGN_KEY") -out ./artifacts.tar.sha256 ./artifacts.tar
  # create archive containing the artifacts and its digital signature
  BASENAME=$(echo $BRANCH_NAME | sed -e 's/[^A-Za-z0-9_-]/_/g')
  tar -cjf "./$BASENAME.tbz2" ./artifacts.tar.sha256 ./artifacts.tar
  # upload archive
  curl --fail --basic -u "$UPLOAD_USER:$UPLOAD_PASS" -F "path=@$BASENAME.tbz2" "$UPLOAD_URL"
fi

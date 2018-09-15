#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# detect CI
if [ -n "${TRAVIS-}" ]
then
  IS_PULL_REQUEST="$TRAVIS_PULL_REQUEST"
  BRANCH_NAME="$TRAVIS_BRANCH"
elif [ -n "${APPVEYOR-}" ]
then
  IS_PULL_REQUEST="${APPVEYOR_PULL_REQUEST_NUMBER:-false}"
  BRANCH_NAME="$APPVEYOR_REPO_BRANCH"
fi

# upload build artifacts for all branches of the upstream repository
if [ "${IS_PULL_REQUEST}" = "false" -a -n "${UPLOAD_URL-}" ]
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
  curl --fail --basic -u "$UPLOAD_USER:$UPLOAD_PASS" -T "./$BASENAME.tbz2" "$UPLOAD_URL/$BASENAME.tbz2"
fi

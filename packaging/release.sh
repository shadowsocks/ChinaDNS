#!/bin/bash

if [ $# -ne 1 ]; then
  echo usage: release.sh version
  exit 1
fi

VERSION=$1

API_JSON=$(printf '{"tag_name": "%s","target_commitish": "master","name": "%s","body": "","draft": false,"prerelease": false}' $VERSION $VERSION)
ID=`curl -v --data "$API_JSON" https://api.github.com/repos/clowwindy/ChinaDNS-C/releases?access_token=$GITHUB_TOKEN | python -c 'import json,sys;print json.load(sys.stdin)["id"]'`

curl -v -H "Content-Type: application/x-tar" \
  -H "Authorization: token $GITHUB_TOKEN" \
  --data-binary "@chinadns-c-$VERSION.tar.gz" \
  https://uploads.github.com/repos/clowwindy/ChinaDNS-C/releases/$ID/assets?name=chinadns-c-$VERSION.tar.gz

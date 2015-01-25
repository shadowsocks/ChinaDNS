#!/bin/bash

if [ $# -ne 1 ]; then
  echo usage: release.sh version
  exit 1
fi

VERSION=$1

git tag $1
git push origin $1
./autogen.sh && ./configure && make dist

API_JSON=$(printf '{"tag_name": "%s","target_commitish": "master","name": "%s","body": "","draft": false,"prerelease": false}' $VERSION $VERSION)
ID=`curl -L -v --data "$API_JSON" https://api.github.com/repos/clowwindy/ChinaDNS/releases?access_token=$GITHUB_TOKEN | python -c 'import json,sys;d = json.load(sys.stdin);print >>sys.stderr, d;print d["id"]'`

curl -v -L -H "Content-Type: application/x-tar" \
  -H "Authorization: token $GITHUB_TOKEN" \
  --data-binary "@chinadns-$VERSION.tar.gz" \
  https://uploads.github.com/repos/clowwindy/ChinaDNS/releases/$ID/assets?name=chinadns-$VERSION.tar.gz

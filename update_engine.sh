#!/bin/bash

REPO_URL="https://github.com/aloyak/origin"
TEMP_DIR="temp_repo_clone"

if [ ! -d "$TEMP_DIR" ]; then
    git clone --filter=blob:none --no-checkout $REPO_URL $TEMP_DIR
fi

cd $TEMP_DIR
git sparse-checkout init --cone
git sparse-checkout set engine sandbox
git checkout master
git pull origin master

cd ..

cp -r $TEMP_DIR/engine/. ./engine/
cp -r $TEMP_DIR/sandbox/. ./sandbox/

echo "Update complete."
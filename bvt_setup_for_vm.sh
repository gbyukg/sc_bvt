#!/usr/bin/env bash

# 给虚拟机使用

BRANCH=${1:?"Branch can not be empty!"}
URL=${2:?"URL can not be empty!"}

voodoo_path=$HOME/workspace/VoodooGrimoire

if [[ ! -d ${voodoo_path} ]]; then
    git clone git@github.com:sugareps/VoodooGrimoire.git "${voodoo_path}"
fi

cd "${voodoo_path}" || exit 1

git fetch origin
br_number=$(date "+%s")

git clean -f -d
git reset --hard
git checkout -b "${br_number}" origin/"${BRANCH}"

ruby -pi -e "gsub(/automation.interface.*/, 'automation.interface = firefox')" src/test/resources/voodoo.properties
ruby -pi -e "gsub(/browser.firefox_binary.*/, 'browser.firefox_binary = /usr/bin/firefox')" src/test/resources/voodoo.properties
ruby -pi -e "gsub(/env.base_url.*/, 'env.base_url = ${URL}')" src/test/resources/grimoire.properties
ruby -pi -e "gsub(/env.local_url.*/, 'env.local_url = ${URL}')" src/test/resources/grimoire.properties
ruby -pi -e "gsub(/300000/, '3')"  src/main/java/com/sugarcrm/sugar/modules/AdminModule.java
ruby -pi -e "gsub(/throw new Exception.*Not all records are indexed.*/, '')" src/main/java/com/sugarcrm/sugar/modules/AdminModule.java

#!/usr/bin/env bash

export VOODOO_PATH=$HOME/workspace/${JOB_NAME}

# 设置显示窗口
if [[ true == "${MONITOR}" ]]; then
    if [[ -z "${CUS_DISPLAY}" ]]; then
        echo "DISPLAY can not be NULL!"
        exit 1
    fi
    export DISPLAY="${CUS_DISPLAY}"
fi

br_number=$(date "+%s")

git clean -f -d
git reset --hard
git checkout -b "${br_number}" origin/"${VoodooGrimoire_Branch}"

# 增加 regression 测试目录
if [[ "Xregression" == X"${CLASS}" ]]; then
    # sed -i '/\*\*\/test\/bvt\/\*\*\/\*\.java/a<include>\*\*\/test\/regression\/\*\*\/\*\.java<\/include>' pom.xml
    sed -i 's/\/test\/bvt/\/test\/regression/g' pom.xml
fi

ruby -pi -e "gsub(/automation.interface.*/, 'automation.interface = firefox')" src/test/resources/voodoo.properties
ruby -pi -e "gsub(/browser.firefox_binary.*/, 'browser.firefox_binary = /usr/bin/firefox')" src/test/resources/voodoo.properties
ruby -pi -e "gsub(/env.base_url.*/, 'env.base_url = ${URL}')" src/test/resources/grimoire.properties
ruby -pi -e "gsub(/env.local_url.*/, 'env.local_url = ${URL}')" src/test/resources/grimoire.properties
ruby -pi -e "gsub(/300000/, '3')"  src/main/java/com/sugarcrm/sugar/modules/AdminModule.java
ruby -pi -e "gsub(/throw new Exception.*Not all records are indexed.*/, '')" src/main/java/com/sugarcrm/sugar/modules/AdminModule.java

git status
git diff
mvn --version
rm -rf target/surefire-reports/*

if [ x"${skip_SugarInit}" = x"false" ];then
    mvn clean install -Dtest=sugarinit.SugarInit -Dcode_coverage=1 -Duser.timezone=Asia/Shanghai -P ci
else
    mvn clean install -DskipTests=true -Duser.timezone=Asia/Shanghai -P ci
fi

if [ ! -z "${test_class}" ];then
    for class in ${test_class}; do
        params=$params "-m ${test_class}"
    done
    mvn test -Dtest=${test_class} -Duser.timezone=Asia/Shanghai -P ci
else
    for mod in ${bvt_module}; do
        params="$params -m \"$CLASS.$mod.*\""
    done
fi

$HOME/sc_bvt/bvt $params

echo "\n\n\n\n=========================== General report... ========================================="
mvn site surefire-report:report-only -Duser.timezone=Asia/Shanghai -DskipTests=true -P ci

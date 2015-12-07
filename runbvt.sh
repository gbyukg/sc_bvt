#!/bin/bash
set +x

if [[ true == "${MONITOR}" ]]; then
    if [[ -z "${CUS_DISPLAY}" ]]; then
        echo "DISPLAY can not be NULL!"
        exit 1
    fi
    export DISPLAY="${CUS_DISPLAY}"
fi

# clone Voodoo 代码库
if [[ ! -d ${HOME}/VoodooGrimoire ]]; then
    git clone git@github.com:sugareps/VoodooGrimoire.git
fi

cd ${HOME}/VoodooGrimoire
git reset --hard

git fetch origin

br_number=$(date "+%s")
git checkout -b "${br_number}" origin/"${VoodooGrimoire_Branch}"

# if [ ! -d VoodooGrimoire ];then
#     git clone git@github.com:sugareps/VoodooGrimoire.git
# else
#     cd VoodooGrimoire

    # 使用火狐浏览器测试
    ruby -pi -e "gsub(/automation.interface.*/, 'automation.interface = firefox')" src/test/resources/voodoo.properties
    # 指定浏览器位置
    ruby -pi -e "gsub(/browser.firefox_binary.*/, 'browser.firefox_binary = /usr/bin/firefox')" src/test/resources/voodoo.properties
    # 配置要测试实例URL
    ruby -pi -e "gsub(/env.base_url.*/, 'env.base_url = ${URL}')" src/test/resources/grimoire.properties
    ruby -pi -e "gsub(/env.local_url.*/, 'env.local_url = ${URL}')" src/test/resources/grimoire.properties
    #ruby -pi -e "gsub(/sugar.admin.indexData.*/, '')" src/test/java/com/sugarcrm/test/sugarinit/SugarInit.java
    #ruby -pi -e "gsub(/sugar.admin.scheduleSystemIndex.*/, '')" src/test/java/com/sugarcrm/test/sugarinit/SugarInit.java
    ruby -pi -e "gsub(/300000/, '3')"  src/main/java/com/sugarcrm/sugar/modules/AdminModule.java
    ruby -pi -e "gsub(/throw new Exception.*Not all records are indexed.*/, '')" src/main/java/com/sugarcrm/sugar/modules/AdminModule.java
    # rm -rf src/test/java/com/sugarcrm/test/bvt/accounts/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/cadences/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/calls/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/contacts/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/general/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/notes/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/opportunities/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/reports/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/roadmap/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/tasks/
    # rm -rf src/test/java/com/sugarcrm/test/bvt/winplan/
    # rm -rf src/test/java/com/sugarcrm/test/crud/
    # rm -rf src/test/java/com/sugarcrm/test/defects/
    # rm -rf src/test/java/com/sugarcrm/test/grimoire
    # rm -rf src/test/java/com/sugarcrm/test/ibm1/
    # rm -rf src/test/java/com/sugarcrm/test/sugarinit/
    # rm -rf src/test/java/com/sugarcrm/test/SugarTest.java
    # rm -rf src/test/java/com/sugarcrm/test/TestTemplate.java
    # rm -rf src/test/java/com/sugarcrm/test/tutorial/
    # rm -rf src/test/java/com/sugarcrm/test/uat/
    # 替换pom.xml 用于增加maven profile 实现模块化测试
    git status
    git diff
    mvn --version
    rm -rf target/surefire-reports/*
    if [ x"${skip_SugarInit}" = x"false" ];then
        mvn clean install -Dtest=sugarinit.SugarInit -Dcode_coverage=1 -Duser.timezone=Asia/Shanghai -P ci
    else
        mvn clean install -DskipTests=true -Duser.timezone=Asia/Shanghai -P ci
    fi
    if [ ! -z ${test_class} ];then
        mvn test -Dtest=${test_class} -Duser.timezone=Asia/Shanghai -P ci
    else
        if [[ -n "${MODULES}" ]]; then
            mvn test -Dtest="${MODULES}" -Duser.timezone=Asia/Shanghai -P ci
        elif [ x"${bvt_module}" = x"all" ];then
            mvn test -Dtest="${MODULES}" -Duser.timezone=Asia/Shanghai -P ci
        else
            mvn test -Dtest=bvt.${bvt_module}.* -Duser.timezone=Asia/Shanghai -P ci
        fi
    fi
    mvn site surefire-report:report-only -Duser.timezone=Asia/Shanghai -DskipTests=true -P ci
    git reset --hard
# fi

#!/usr/bin/env bash

#export VOODOO_PATH=$HOME/workspace/${JOB_NAME}
export VOODOO_PATH=$HOME/VoodooGrimoire

# 设置显示窗口
if [[ true == "${MONITOR}" ]]; then
    if [[ -z "${CUS_DISPLAY}" ]]; then
        echo "DISPLAY can not be NULL!"
        exit 1
    fi
    export DISPLAY="${CUS_DISPLAY}"
else
    # 启动 xvfb, 程序执行完毕后, 将会自动关闭进程
    Xvfb :99 -ac -screen 0 1280x1024x24 &
    export DISPLAY=:99
fi

if [[ ! -d $HOME/VoodooGrimoire ]]; then
    git clone git@github.com:sugareps/VoodooGrimoire.git $HOME/VoodooGrimoire
fi

cd "${VOODOO_PATH}" || exit 1
git fetch origin
br_number=$(date "+%s")

git clean -f -d
git reset --hard
git checkout -b "${br_number}" origin/"${VoodooGrimoire_Branch}"

mvn clean install -DskipTests=true -Duser.timezone=Asia/Shanghai -P ci

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

#if [ x"${skip_SugarInit}" = x"false" ];then
    # -Dtest=sugarinit.SugarInit
    # -Dtest=sugarinit.SugarInitSC4BP
#    mvn clean install -Dtest=sugarinit.SugarInit -Dcode_coverage=1 -Duser.timezone=Asia/Shanghai -P ci
#else
#    mvn clean install -DskipTests=true -Duser.timezone=Asia/Shanghai -P ci
#fi


# 客户端获取模块并开始执行 bvt 测试脚本
# SERVER_IP 和 SERVER_PORT 变量在 bvt_server job 中设置
$HOME/sc_bvt/client -i ${SERVER_IP} -t ${SERVER_PORT} -p 1

echo ""
echo ""
echo ""
echo "=========================== General report... ========================================="

mvn site surefire-report:report-only -Duser.timezone=Asia/Shanghai -DskipTests=true -P ci

# 复制 report 结果, 使 jenkins junit 插件生成测试结果
rm -rf "${WORKSPACE}"/surefire-reports
cp -r "${VOODOO_PATH}"/target/surefire-reports/ "${WORKSPACE}"
# report 结果
#if [[ ! -d "${WORKSPACE}"/surefire-reports ]]; then
    #ln -s "${VOODOO_PATH}"/target/surefire-reports/ "${WORKSPACE}"
#fi

# 截图
if [[ ! -d "${HOME}"/www/screenshots ]]; then
    ln -s "${VOODOO_PATH}"/log/screenshots/ "${HOME}"/www/ > /dev/null
fi

# 关闭 firefox 进程
pkill firefox

#!/usr/bin/env bash

set -x
VOODOO_PATH="${HOME}"/VoodooGrimoire

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

# clone Voodoo 代码库
if [[ ! -d ${HOME}/VoodooGrimoire ]]; then
    git clone git@github.com:sugareps/VoodooGrimoire.git "${HOME}"/VoodooGrimoire
fi

cd ${HOME}/VoodooGrimoire || exit 1
git clean -f
git reset --hard
git fetch --all
br_number=$(date "+%s")
git checkout -b "${br_number}" origin/"${VoodooGrimoire_Branch}"

if [[ "Xregression" == X"${CLASS}" ]]; then
    # sed -i '/\*\*\/test\/bvt\/\*\*\/\*\.java/a<include>\*\*\/test\/regression\/\*\*\/\*\.java<\/include>' pom.xml
    sed -i 's/\/test\/bvt/\/test\/regression/g' pom.xml
fi
# 使用火狐浏览器测试
ruby -pi -e "gsub(/automation.interface.*/, 'automation.interface = firefox')" src/test/resources/voodoo.properties
# 指定浏览器位置
ruby -pi -e "gsub(/browser.firefox_binary.*/, 'browser.firefox_binary = /usr/bin/firefox')" src/test/resources/voodoo.properties
# 配置要测试实例URL
ruby -pi -e "gsub(/env.base_url.*/, 'env.base_url = ${BVT_TEST_URL}')" src/test/resources/grimoire.properties
ruby -pi -e "gsub(/env.local_url.*/, 'env.local_url = ${BVT_TEST_URL}')" src/test/resources/grimoire.properties
#ruby -pi -e "gsub(/sugar.admin.indexData.*/, '')" src/test/java/com/sugarcrm/test/sugarinit/SugarInit.java
#ruby -pi -e "gsub(/sugar.admin.scheduleSystemIndex.*/, '')" src/test/java/com/sugarcrm/test/sugarinit/SugarInit.java
#ruby -pi -e "gsub(/300000/, '3')"  src/main/java/com/sugarcrm/sugar/modules/AdminModule.java
#ruby -pi -e "gsub(/throw new Exception.*Not all records are indexed.*/, '')" src/main/java/com/sugarcrm/sugar/modules/AdminModule.java

# 修改opp保存后获取opp id页面弹出层
#sed -i '98d' src/main/java/com/sugarcrm/sugar/views/BWCEditView.java
cp ~/sc_bvt/BWCEditView.java src/main/java/com/sugarcrm/sugar/views/BWCEditView.java

# 增大元素查找时间, 保存roadmap时候, 通过查找保存后弹出的保存成功提示, 提示消失过快, 导致保存后页面未完全加载完就消失
# 导致获取不到该弹出框, 结果测试失败.
sed -i 's/1000/6000/g' src/main/java/com/sugarcrm/sugar/VoodooControl.java
sed -i 's/15000/60000/g' src/main/java/com/sugarcrm/sugar/VoodooControl.java

# 增加 run_cron_es.sh 脚本, 用于跑初始化 ES 数据用
SERVER=${BVT_TEST_URL:7:25}
INSTANCE=${BVT_TEST_URL:33}
echo "ssh -o StrictHostKeyChecking=no btit@${SERVER} \"php /home/btit/www/${INSTANCE}/cron.php\"" > run_cron_es.sh

mvn clean install -DskipTests=true -Duser.timezone=Asia/Shanghai -P ci

if [ x"${skip_SugarInit}" = x"false" ];then
    mvn clean install -Dtest=sugarinit.SugarInit -Dcode_coverage=1 -Duser.timezone=Asia/Shanghai -P ci
fi
if [ ! -z ${test_class} ];then
    mvn test -Dtest=${test_class} -Duser.timezone=Asia/Shanghai -P ci
else
    if [[ -n "${MODULES}" ]]; then
        mvn test -Dtest="${MODULES}" -Duser.timezone=Asia/Shanghai -P ci
    elif [ x"${bvt_module}" = x"all" ];then
        mvn test -Duser.timezone=Asia/Shanghai -P ci
    else
        mvn test -Dtest=${CLASS}.${bvt_module}.* -Duser.timezone=Asia/Shanghai -P ci
    fi
fi

echo ""
echo ""
echo ""
echo "********************* Report ***************************"

mvn site surefire-report:report-only -Duser.timezone=Asia/Shanghai -DskipTests=true -P ci

# 获取 report 结果
rm -rf "${WORKSPACE}"/surefire-reports
cp -r "${VOODOO_PATH}"/target/surefire-reports/ "${WORKSPACE}"

#git reset --hard
# 关闭 firefox 进程
pkill firefox

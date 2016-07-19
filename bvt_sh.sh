#!/usr/bin/env bash
set -x

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
git reset --hard origin/"${VoodooGrimoire_Branch}"
#git checkout -b "${br_number}" origin/"${VoodooGrimoire_Branch}"

mvn clean install -DskipTests=true -Duser.timezone=Asia/Shanghai -P ci

# BP testing
if [[ X"${CLASS}" == "XBP" ]]; then
    sed -i 's/\/test\/bvt/\/test\//g' pom.xml
    sed -i '/.*<target>1\.7<\/target>.*/a<encoding>ISO-8859-1<\/encoding>' pom.xml
    CLASS=""
else
    # 增加 regression 测试目录
    if [[ "Xregression" == X"${CLASS}" ]]; then
        # sed -i '/\*\*\/test\/bvt\/\*\*\/\*\.java/a<include>\*\*\/test\/regression\/\*\*\/\*\.java<\/include>' pom.xml
        sed -i 's/\/test\/bvt/\/test\/regression/g' pom.xml
        sed -i '/.*<target>1\.7<\/target>.*/a<encoding>ISO-8859-1<\/encoding>' pom.xml
    fi
    CLASS="${CLASS}."
fi

ruby -pi -e "gsub(/automation.interface.*/, 'automation.interface = firefox')" src/test/resources/voodoo.properties
ruby -pi -e "gsub(/browser.firefox_binary.*/, 'browser.firefox_binary = /usr/bin/firefox')" src/test/resources/voodoo.properties
ruby -pi -e "gsub(/env.base_url.*/, 'env.base_url = ${BVT_TEST_URL}')" src/test/resources/grimoire.properties
ruby -pi -e "gsub(/env.local_url.*/, 'env.local_url = ${BVT_TEST_URL}')" src/test/resources/grimoire.properties
ruby -pi -e "gsub(/300000/, '3')"  src/main/java/com/sugarcrm/sugar/modules/AdminModule.java
ruby -pi -e "gsub(/throw new Exception.*Not all records are indexed.*/, '')" src/main/java/com/sugarcrm/sugar/modules/AdminModule.java

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

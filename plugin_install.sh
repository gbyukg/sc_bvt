#!/usr/bin/env bash

[[ "true" == "${DEBUG}" ]] && set -x

if [[ $# -lt 2 ]]; then
    echo "Wrong paramater"
    exit 1
fi

readonly INSTANCE_PATH=$1 # 安装实例目录
readonly PLUGIN_URL=http://sc2.gnuhub.com/$2
readonly NATIVE_URL=http://sc2.gnuhub.com/226/scm-native-install-SFAMI2.2.6_20150518-0906.tar
readonly PLUGIN_PATH="${INSTANCE_PATH}/plugins"

if [[ ! -d "${INSTANCE_PATH}" ]]; then
    echo "Error: Instanch path [${INSTANCE_PATH}] not exist"
    exit 1
fi

echo "********** Install plugins from ${PLUGIN_URL}... **********"

if [[ ! -d "${PLUGIN_PATH}" ]]; then
    mkdir "${PLUGIN_PATH}"
    echo "Download plugin..."
    wget -nd -A.zip -r --no-parent -P "${PLUGIN_PATH}" "${PLUGIN_URL}/" > /dev/null
    if [[ $? -ne 0 ]]; then
        echo "download plugins from ${PLUGIN_URL} wrong!"
        exit 1
    fi
fi

cd "${INSTANCE_PATH}"

# 表面上看它是一个 tar 压缩文件, 实际上它是一个zip压缩文件按
# 插件安装自动搜索所有 .zip 文件, 只是为了避免搜索到这个文件而已
if [[ ! -f scm-native-install-SFAMI2.2.6_20150518-0906.tar ]]; then
    wget "${NATIVE_URL}"
    [[ $? -ne 0 ]] && echo "wget wrong!" && exit 1
fi

[[ -d native ]] && rm -rf native
unzip scm-native-install-SFAMI2.2.6_20150518-0906.tar > /dev/null

cd native && \
sed -i "s/scm_web/sc_web/g" web/nomad.js && \
mv web ../mobile

cat <<'PLUGIN_INSTALL' > plugin_install.php
<?php

if ($argc < 2) {
    echo "Wrong paramaters";
    exit(1);
}

require_once 'include/SugarInstanceManager.php';

$instance_path = $argv[1];
$plugin_path = $argv[2];

create_datas($instance_path);

$pm = ObjectHandler::instantiate('PluginManager');
$pm->setPathToSugar($instance_path);
$pm->scan($plugin_path);
$plugins = $pm->getValidPlugins();
$order = array();

foreach ($plugins as $plugin){
    array_push($order,basename($plugin, ".zip"));
}

$pm->installPlugins($order);

function create_datas($instance_path)
{
    $sugar_config = SugarUtils::getSugarConfig($instance_path);
    $db_config = SugarUtils::sugarDbToSimDb($sugar_config['dbconfig']);
    $db = DBHelper::getDb($db_config['db_type']);
    $db->setConfig($db_config);
    $db->connect();

    $result = $db->query("select count(*) from EMAIL_ADDR_BEAN_REL where id = 'richardlee10@tst.ibm.com'");
    if ($result) {
        $row = $db->fetch($result);
        if ($row[0] == 0) {
            $sql = "INSERT INTO EMAIL_ADDR_BEAN_REL
(ID, EMAIL_ADDRESS_ID, BEAN_ID, BEAN_MODULE, PRIMARY_ADDRESS, REPLY_TO_ADDRESS, DATE_CREATED, DATE_MODIFIED, DELETED)
VALUES('richardlee10@tst.ibm.com', 'richardlee10@tst.ibm.com', 'fh-level0user0', 'Users', 1, 0, '2012-01-01-00.00.00.000000', '2012-01-01-00.00.00.000000', 0)";
            $db->query($sql);
        }
    }
    $result = $db->query("select count(*) from EMAIL_ADDRESSES where id = 'richardlee10@tst.ibm.com'");
    if ($result) {
        $row = $db->fetch($result);
        if ($row[0] == 0) {
            $sql = "INSERT INTO EMAIL_ADDRESSES
(ID, EMAIL_ADDRESS, EMAIL_ADDRESS_CAPS, INVALID_EMAIL, OPT_OUT, DATE_CREATED, DATE_MODIFIED, DELETED)
VALUES('richardlee10@tst.ibm.com', 'richardlee10@tst.ibm.com', 'richardlee10@tst.ibm.com', 0, 0, '2012-01-01-00.00.00.000000', '2012-01-01-00.00.00.000000', 0)";
            $db->query($sql);
        }
    }
    $result = $db->query("select count(*) from USERS where id = 'richardlee10@tst.ibm.com'");
    if ($result) {
        $row = $db->fetch($result);
        if ($row[0] == 0) {
            $sql = "INSERT INTO USERS (ID, USER_NAME, USER_HASH, SYSTEM_GENERATED_PASSWORD, PWD_LAST_CHANGED, AUTHENTICATE_ID, SUGAR_LOGIN, PICTURE, FIRST_NAME, LAST_NAME, IS_ADMIN, EXTERNAL_AUTH_ONLY, RECEIVE_NOTIFICATIONS, DESCRIPTION, DATE_ENTERED, DATE_MODIFIED, LAST_LOGIN, MODIFIED_USER_ID, CREATED_BY, TITLE, DEPARTMENT, PHONE_HOME, PHONE_MOBILE, PHONE_WORK, PHONE_OTHER, PHONE_FAX, STATUS, ADDRESS_STREET, ADDRESS_CITY, ADDRESS_STATE, ADDRESS_COUNTRY, ADDRESS_POSTALCODE, DEFAULT_TEAM, TEAM_SET_ID, DELETED, PORTAL_ONLY, SHOW_ON_EMPLOYEES, EMPLOYEE_STATUS, MESSENGER_ID, MESSENGER_TYPE, REPORTS_TO_ID, IS_GROUP, PREFERRED_LANGUAGE, FMS_ORG_ROLLUP3, TOPT_BUSINESS_UNIT_NAME, ALT_LANGUAGE_FIRST_NAME, ALT_LANGUAGE_LAST_NAME, PREFERRED_NAME, INACTIVE_AVAILABLE, EMPLOYEE_CNUM, USER_JOB_ROLE, LOTUS_NOTES_ID, TOPT_JOB_ROLE_NAME, IS_MANAGER, ACTIVITY_TRACKING_TEMPLATE, VISIBILITY_TIER2, USER_JOB_ROLE_SKILL_SET, SFA_ROLE, PREFERRED_FIRST_NAME, HR_FIRST_NAME, CALL_UP_NAME, HR_PREFERRED_NAME, ROLE_CTGY1, TIER1_TOP_NODE, LAST_UPDATING_SYSTEM_C, USER_DN, TIER2_TOP_NODE, IS_LCR, LCR_SEGMENT, ROLE_CTGY2, ROLE_GROUP, TOPT_BUSINESS_UNIT_ID, TOPT_JOB_ROLE_ID, VISIBILITY_TIER1)
VALUES
('richardlee10@tst.ibm.com', 'richardlee10@tst.ibm.com', '\$1\$zyXo62x5\$WbfwTd6kYFn.5lfTQOPdb0', 0, null, null, 1, '38fc498a-1ffc-62d9-954b-5566d8b95f9c', null, 'Administrator', 1, 0, 0, null, '2015-05-28 08:46:25.000000', '2015-05-28 10:41:21.000000', '2015-05-29 08:23:05.000000', '1', ' ', 'Administrator', null, null, null, null, null, null, 'Active', null, null, null, '', null, '1', '1', 0, 0, 1, 'Active', null, null, null, 0, null, null, null, null, null, null, 0, ' ', null, null, null, 0, null, null, null, null, null, null, null, null, null, ' ', null, 'abc', ' ', 0, null, null, null, ' ', ' ', null);";
            $db->query($sql);
        }
    }
}
PLUGIN_INSTALL

mv -f plugin_install.php "${INSTANCE_PATH}"/vendor/sugareps/SugarInstanceManager/plugin_install.php
cd "${INSTANCE_PATH}"/vendor/sugareps/SugarInstanceManager

echo "" >> config_override.php
echo "\$sugar_config['dev_functional_id_email'] = 'richardlee10@us.ibm.com';" >> "${INSTANCE_PATH}"/config_override.php
echo "\$sugar_config['functional_id'] = 'richardlee10@tst.ibm.com';" >> "${INSTANCE_PATH}"/config_override.php
echo "\$sugar_config['ieb_connections_ro_base_url'] = 'http://n005192.portsmouth.uk.ibm.com:3470';" >> "${INSTANCE_PATH}"/config_override.php
echo "\$sugar_config['ieb_connections_base_url'] = 'http://n005192.portsmouth.uk.ibm.com:3470';" >> "${INSTANCE_PATH}"/config_override.php 
echo "\$sugar_config['connections_base_url'] ='https://devconnections2.rtp.raleigh.ibm.com';" >> "${INSTANCE_PATH}"/config_override.php
echo "\$sugar_config['connections_http_base_url'] = 'https://devconnections2.rtp.raleigh.ibm.com';" >> "${INSTANCE_PATH}"/config_override.php
echo "\$sugar_config['connections_common_path'] = '/common';" >> "${INSTANCE_PATH}"/config_override.php
echo "\$sugar_config['enable_collab'] = true;" >> "${INSTANCE_PATH}"/config_override.php

for plugin in ${PLUGIN_PATH}/*
do
    echo "**************** Install ${plugin}... *******************"
    php plugin_install.php "${INSTANCE_PATH}" "${plugin}"
    echo ""
done

cd "${INSTANCE_PATH}"
sed -i "s/\s\$bean = BeanFactory::newBean(\$modName);/if (\$modName == 'TrackerSessions') continue;\$bean = BeanFactory::newBean(\$modName);/g" clients/base/api/CurrentUserApi.php
rm -rf custom/include/javascript/generated


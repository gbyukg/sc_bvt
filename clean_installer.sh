#!/usr/bin/env bash

red_echo()
{
    printf "\n\e[31m%s\e[0m\n" "$@"
}

ERRTRAP()
{
    red_echo "[LINE:$1] Error: Command or function exited with status $?"
}
trap 'ERRTRAP $LINENO' ERR

clean_file_dir=${1:-$(date +%Y-%m-%d)}
readonly clean_dir=${HOME}/tmp/"${clean_file_dir}"
readonly web_dir=${HOME}/www

echo "Drop instances in [${clean_file_dir}]"

[[ ! -d "${clean_dir}" ]] && echo "Done" && exit 0

drop_db()
{
    db_name=${1}
    printf "Drop database %s ...\n" "${db_name}"
    db2 connect to "${db_name}" && \
    db2 quiesce database immediate force connections && \
    db2 unquiesce database && \
    db2 connect reset && \
    db2 deactivate db "${db_name}" && \
    db2 "DROP DATABASE ${db_name}" # drop the previously existing database if it exists
}

cd "${clean_dir}"
for del_file in *; do
    printf "\nRemoving %s ...\n" "${del_file}"
    rm -rf "${web_dir}/${del_file}" > /dev/null 2>&1
    drop_db "DB_${del_file}"
done

# 删除日志文件
# 修改日志文件目录到 ${HOME}/log 下, 进行统一管理
cd "$HOME"/www
if [[ -f php_errors.log ]]; then
    cat /dev/null > php_errors.log
fi

rm -rf "${clean_dir}"
rm -rf hotfix_hook_*.sh

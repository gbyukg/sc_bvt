#!/usr/bin/env bash

export local_source=/home/stallman/builds
export local_destination=/var/www/sugarsync
remote_server=ibmserver@74.85.23.218:/builds

[[ -x "${local_source}" ]] || mkdir -p "${local_source}"

file_count=$(ls /home/stallman/builds/ | wc -l)

[[ "${file_count}" -eq 0 ]] && {
    echo "Mount ${remote_server} to ${local_source} ..."
    sshfs ${remote_server} -o workaround=rename -o IdentityFile=/home/stallman/.ssh/ibm_server "${local_source}"
    [[ $? -ne 0 ]] && echo "sshfs wrong!" && exit 0
}

fun_sync()
{
    file=${1#/home/stallman/builds/}
    des_file="${local_destination}/${file}"
    [[ ! -f "${des_file}" ]] && {
        [[ -d "${des_file%/*}" ]] || mkdir -p "${des_file%/*}"
        echo "sync [${1}] => [${des_file}]"
        #cp "${1}" "${local_destination}/${file}"
        rsync -rlvcP --progress "${1}" "${local_destination}/${file}"
    }
}

export -f fun_sync

#find "${local_source}" -ctime -3 -not -path '*/\.*' -type f -exec sh -c 'for i do fun_sync "${i}"; done' sh {} +
echo "Start to sync ..."
find "${local_source}" -ctime -7 -not -path '*/\.*' -type f -exec bash -c 'fun_sync "{}"' \;

echo "Unmount..."
fusermount -u "${local_source}"


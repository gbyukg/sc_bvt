jenkins 脚本执行文件

# server
首先启动 server 服务, 通过 `USR1` 信号让 server 读取 modules.txt 文件获取需要测试的模块

# client
client -i server 的 ip地址 -t 端口号 -p 并行执行进程数(应该为1, 大于1 bvt有可能失败...)

``` sh
#!/usr/bin/env bash

#$HOME/sc_bvt/runbvt.sh
$HOME/sc_bvt/bvt_sh.sh

ssh stallman@9.119.106.38 "mkdir /var/www/bvt_screenshots/${BUILD_NUMBER}"
scp -r /home/btit/VoodooGrimoire/log/screenshots stallman@9.119.106.38:/var/www/bvt_screenshots/${BUILD_NUMBER}

echo "************************* screenshots ********************"
echo "http://9.119.106.38/bvt_screenshots/${BUILD_NUMBER}/screenshots/"
```

### jenkins 触发其他 job 不成功
配置 jenkins 的 `Configure Global Security` -> `Access Control for Builds` ->
`Project default Build Authorization` -> `Run as User who Triggered Build`

### 服务器端的 server 启动后, telnet 不成功:
``` sh
#1.Check iptable rules.
iptables -L

2.Flush iptables
iptables -F

#Try telnet again
```

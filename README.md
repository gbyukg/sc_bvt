jenkins 脚本执行文件

``` sh
#!/usr/bin/env bash

#$HOME/sc_bvt/runbvt.sh
$HOME/sc_bvt/bvt_sh.sh

ssh stallman@9.119.106.38 "mkdir /var/www/bvt_screenshots/${BUILD_NUMBER}"
scp -r /home/btit/VoodooGrimoire/log/screenshots stallman@9.119.106.38:/var/www/bvt_screenshots/${BUILD_NUMBER}

echo "************************* screenshots ********************"
echo "http://9.119.106.38/bvt_screenshots/${BUILD_NUMBER}/screenshots/"
```

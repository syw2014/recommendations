#########################################################################
# File Name: install.sh
# Author: JerryShi
# mail: jerryshi0110@gmail.com
# Created Time: 2015年07月21日 星期二 14时55分03秒
#########################################################################
#!/bin/bash
#You can run this script file to install jsoncpp under any directory,
#It will find the latest version to complie and install,make distclean
#comes first,
cd `dirname $0`
dir=`pwd`

rm -rf $dir/jsoncpp

tar -zxvf jsoncpp-src-0.5.0.tar.gz
cd $dir/jsoncpp-src-0.5.0
make release

cd $dir
mv jsoncpp-src-0.5.0 jsoncpp



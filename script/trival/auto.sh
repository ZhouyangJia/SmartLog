#!/bin/bash
#echo "Analyze bftpd..."
#cd /Users/zhouyangjia/source/bftpd/smartlog-script 
#time ${1}

echo "Analyze httpd..."
cd /Users/zhouyangjia/source/httpd-2.4.10/smartlog-script
time ${1}

echo "Analyze subversion..."
cd /Users/zhouyangjia/source/subversion-1.8.10/smartlog-script
time ${1}


echo "Analyze mysql..."
cd /Users/zhouyangjia/source/mysql-5.6.17/smartlog-script
time ${1}



echo "Analyze postgresql..."
cd /Users/zhouyangjia/source/postgresql-9.3.5/smartlog-script
time ${1}



echo "Analyze gimp..."
cd /Users/zhouyangjia/source/gimp-2.8.0/smartlog-script
time ${1}



echo "Analyze wireshark..."
cd /Users/zhouyangjia/source/wireshark-1.12.2/smartlog-script
time ${1}


#!/bin/bash

## ssh -o StrictHostKeyChecking=no -A -i ~alexey/.ssh/id_rsa root@192.168.125.101

## 
ssh -o StrictHostKeyChecking=no -A -i ~alexey/.ssh/id_rsa root@192.168.123.101 << EOF
mkdir -p /srv/files_for_docker
cd /srv/files_for_docker
touch 11111.txt
apt-get install tree
tree /srv
EOF

scp -r -i ~alexey/.ssh/id_rsa /home/alexey/task10_12_3/docker/ root@192.168.123.101:/srv/files_for_docker/


nmap -p22 192.168.123.101 | grep "52:54:00:FA:D6:E5"



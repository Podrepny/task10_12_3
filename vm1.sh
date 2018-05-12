#!/bin/bash

ssh-keygen -f "/home/alexey/.ssh/known_hosts" -R 192.168.123.101

## files to copy
/home/alexey/task10_12_3/docker/



scp /home/alexey/task10_12_3/docker/* ubuntu@192.168.123.101:~/files_for_docker/*

## 
ssh -A -i ~alexey/.ssh/id_rsa ubuntu@192.168.123.101 << EOF
sudo mkdir -p /home/ubuntu/files_for_docker
cd /home/ubuntu/files_for_docker
EOF

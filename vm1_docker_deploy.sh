#!/bin/bash

SCRIPT_DIR=`dirname $0`
cd ${SCRIPT_DIR}

source config
source function.inc

cd /srv/docker-cfg/



docker-compose up -d

exit 0

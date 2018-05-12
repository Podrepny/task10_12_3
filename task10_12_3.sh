#!/bin/bash

SCRIPT_DIR=`dirname $0`
cd ${SCRIPT_DIR}

source config
source function.inc

WRK_DIR_PATH=$(pwd)
XML_PATH="networks"
CLOUDINIT_CONF_DIR="config-drives"
CLOUDINIT_CONF_DIR_SUFIX="-config"
VI_BR_PREFIX="virbr"
EXT_DHCP_IP_RANGE_BEGIN="2"
EXT_DHCP_IP_RANGE_END="254"
EXT_VM1_MAC=52:54:00:`(date; cat /proc/interrupts) | md5sum | sed -r 's/^(.{6}).*$/\1/; s/([0-9a-f]{2})/\1:/g; s/:$//;'`
#EXT_VIBR_NAME="${VI_BR_PREFIX}${EXTERNAL_NET##*.}"
EXT_VIBR_NAME="${VI_BR_PREFIX}-${VM1_EXTERNAL_IF}"
EXT_XML_PATH="${XML_PATH}/${EXTERNAL_NET_NAME}.xml"
#INT_VIBR_NAME="${VI_BR_PREFIX}${INTERNAL_NET##*.}"
INT_VIBR_NAME="${VI_BR_PREFIX}-${VM1_INTERNAL_IF}"
INT_XML_PATH="${XML_PATH}/${INTERNAL_NET_NAME}.xml"
#MGM_VIBR_NAME="${VI_BR_PREFIX}${MANAGEMENT_NET##*.}"
MGM_VIBR_NAME="${VI_BR_PREFIX}-${VM1_MANAGEMENT_IF}"
MGM_XML_PATH="${XML_PATH}/${MANAGEMENT_NET_NAME}.xml"
MAX_TIMEOUT_FOR_VM1="60"
HOST_NAME=${VM1_NAME}
SSL_DIR="$WRK_DIR_PATH/docker/certs"
SSL_DIR_DOCKER="/etc/nginx/certs"
NGINX_CFG_DIR="$WRK_DIR_PATH/docker/etc"

# path in to vm1
VM1_CFG_AND_CERTS_DIR="/srv"
VM1_NGINX_SSL_DIR="${VM1_CFG_AND_CERTS_DIR}/certs"
VM1_NGINX_CFG_DIR="${VM1_CFG_AND_CERTS_DIR}/nginx"
VM1_NGINX_LOG="${NGINX_LOG_DIR}"
VM1_DOCKER_COMPOSE_FILE="${VM1_CFG_AND_CERTS_DIR}/docker-compose"

NGINX_CFG_FILE="$WRK_DIR_PATH/docker/etc/nginx.conf"
NGINX_CFG_FILE_DOCKER="/etc/nginx/nginx.conf"
NGINX_CFG_TEMPLATE_FILE="$WRK_DIR_PATH/docker/template/nginx.conf.template"
NGINX_LOG_DIR_DOCKER="/var/log/nginx"
DOCKER_SRV_APACHE_NAME="${VM2_VXLAN_IP}"
DOCKER_SRV_APACHE_PORT="${APACHE_PORT}"
DOCKER_SRV_NGINX_NAME="proxy"
DOCKER_SRV_NGINX_PORT="443"
DOCKER_SRV_NGINX_LOG_DIR="/var/log/nginx"
DOCKER_YML_FILE="$WRK_DIR_PATH/docker/docker-compose.yml"
DOCKER_YML_TEMPLATE_FILE="$WRK_DIR_PATH/docker/template/docker-compose.yml.template"

## generate id_rsa
if [ ! -e $SSH_PUB_KEY ]; then 
  ssh-keygen -t rsa -b 4096 -f ${SSH_PUB_KEY%.pub} -q -N ""
fi

## install packages
apt-get update
apt-get -y install ssh openssh-server openssl
apt-get -y install apt-transport-https ca-certificates curl software-properties-common
apt-get -y install qemu-kvm libvirt-bin virtinst virt-viewer bridge-utils genisoimage
apt-get -y install mc virt-top libvirt-doc git

## download virtual mashine
wget -c ${VM_BASE_IMAGE} || exit 1

# create dir tree and files
func_dir_tree_gen

## generate ssl CA and web
func_ssl_gen

## generate nginx config
eval "echo \"$(cat ${NGINX_CFG_TEMPLATE_FILE})\"" > ${NGINX_CFG_FILE}

## generate docker-compose.yml from template
eval "echo \"$(cat ${DOCKER_YML_TEMPLATE_FILE})\"" > ${DOCKER_YML_FILE}

## create netwoks config
## make dir for xml files
mkdir -p ${XML_PATH}
## generate external.xml
func_gen_conf_ext
## generate internal.xml
func_gen_conf_int
## generate management.xml
func_gen_conf_mgm
## define and start all networks
func_create_net ${EXTERNAL_NET_NAME} ${EXT_XML_PATH}
func_create_net ${INTERNAL_NET_NAME} ${INT_XML_PATH}
func_create_net ${MANAGEMENT_NET_NAME} ${MGM_XML_PATH}

# debug 
virsh net-list --all

## make user-data and meta-data  based on config
func_gen_cludinit_conf_vm1 "${CLOUDINIT_CONF_DIR}/${VM1_NAME}${CLOUDINIT_CONF_DIR_SUFIX}"
func_gen_cludinit_conf_vm2 "${CLOUDINIT_CONF_DIR}/${VM2_NAME}${CLOUDINIT_CONF_DIR_SUFIX}"

## deploy vm1
func_deploy_vm ${VM1_NAME} ${VM1_HDD} ${VM1_CONFIG_ISO} ${VM1_NUM_CPU} ${VM1_MB_RAM} "--network network=${EXTERNAL_NET_NAME},model=virtio,mac=${EXT_VM1_MAC}"

## wait dhcp for vm1
func_wait_dhcp ${MAX_TIMEOUT_FOR_VM1}

## deploy vm1
func_deploy_vm ${VM2_NAME} ${VM2_HDD} ${VM2_CONFIG_ISO} ${VM2_NUM_CPU} ${VM2_MB_RAM}

# debug
virsh list --all

exit 0

#!/bin/bash

SCRIPT_DIR=`dirname $0`
cd ${SCRIPT_DIR}

source config
source function.inc

XML_PATH="networks"
CLOUDINIT_CONF_DIR="config-drives"
CLOUDINIT_CONF_DIR_SUFIX="-config"
VI_BR_PREFIX="virbr"
EXT_DHCP_IP_RANGE_BEGIN="2"
EXT_DHCP_IP_RANGE_END="254"
EXT_VIBR_NAME="${VI_BR_PREFIX}${EXTERNAL_NET##*.}"
EXT_XML_PATH="${XML_PATH}/${EXTERNAL_NET_NAME}.xml"
INT_VIBR_NAME="${VI_BR_PREFIX}${INTERNAL_NET##*.}"
INT_XML_PATH="${XML_PATH}/${INTERNAL_NET_NAME}.xml"
MGM_VIBR_NAME="${VI_BR_PREFIX}${MANAGEMENT_NET##*.}"
MGM_XML_PATH="${XML_PATH}/${MANAGEMENT_NET_NAME}.xml"
SSL_DIR="$WRK_DIR_PATH/docker/certs"
SSL_DIR_DOCKER="/etc/nginx/certs"
NGINX_CFG_DIR="$WRK_DIR_PATH/docker/etc"
NGINX_CFG_FILE="$WRK_DIR_PATH/docker/etc/nginx.conf"
NGINX_CFG_FILE_DOCKER="/etc/nginx/nginx.conf"
NGINX_CFG_TEMPLATE_FILE="$WRK_DIR_PATH/docker/template/nginx.conf.template"
NGINX_LOG_DIR_DOCKER=/var/log/nginx
DOCKER_SRV_APACHE_NAME="web"
DOCKER_SRV_APACHE_PORT="80"
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
apt-get -y install ssh openssh-server
apt-get -y install qemu-kvm libvirt-bin virtinst virt-viewer bridge-utils genisoimage
apt-get -y install mc virt-top libvirt-doc git

# create dir tree and files
func_dir_tree_gen

## generate ssl CA and web
func_ssl_gen

## generate nginx config
eval "echo \"$(cat ${NGINX_CFG_TEMPLATE_FILE})\"" > ${NGINX_CFG_FILE}
#command: /bin/bash -c "envsubst < /etc/nginx/conf.d/mysite.template > /etc/nginx/conf.d/default.conf && nginx -g 'daemon off;'"

## generate docker-compose.yml from template
eval "echo \"$(cat ${DOCKER_YML_TEMPLATE_FILE})\"" > ${DOCKER_YML_FILE}

## download virtual mashine
wget -c ${VM_BASE_IMAGE} || exit 1

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

## deploy vm`s
func_deploy_vm ${VM1_NAME} ${VM1_HDD} ${VM1_CONFIG_ISO} "--network network=${EXTERNAL_NET_NAME},model=virtio"
func_deploy_vm ${VM2_NAME} ${VM2_HDD} ${VM2_CONFIG_ISO}

# debug
virsh list --all

exit 0

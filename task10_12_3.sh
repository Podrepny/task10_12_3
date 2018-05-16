#!/bin/bash

SCRIPT_DIR=`dirname $0`
cd ${SCRIPT_DIR}

source config
source function.inc

## ========================================================================
## ------------------------
## host variable
## ------------------------
HOST_SCRIPT_WRK_DIR=$(pwd)    ## path to script - dir
HOST_NETWORK_XML_CFG_DIR="networks"    ## virtual network config - dir
HOST_CLOUDINIT_CFG_DIR="config-drives"    ## cloud-init config - dir 
HOST_CLOUDINIT_CFG_DIR_SUFIX="-config"    ## sufix for subdir - sufix
HOST_CLOUDINIT_CFG_DRV_VM1_DIR="${HOST_CLOUDINIT_CFG_DIR}/${VM1_NAME}${HOST_CLOUDINIT_CFG_DIR_SUFIX}"    ## vm1 cloud init config (meta-data, user-data) - dir
HOST_CLOUDINIT_CFG_DRV_VM2_DIR="${HOST_CLOUDINIT_CFG_DIR}/${VM2_NAME}${HOST_CLOUDINIT_CFG_DIR_SUFIX}"    ## vm2 cloud init config (meta-data, user-data) - dir
HOST_DOCKER_CFG_DIR="${HOST_SCRIPT_WRK_DIR}/docker"    ## on host docker config root - dir
HOST_SSL_DIR="${HOST_DOCKER_CFG_DIR}/certs"    ## ssl key and certificate - dir
HOST_NGINX_CFG_DIR="${HOST_DOCKER_CFG_DIR}/etc"    ## nginx config - dir
HOST_NGINX_CFG_FILE="${HOST_NGINX_CFG_DIR}/nginx.conf"    ## nginx config - file
HOST_NGINX_CFG_TEMPLATE_DIR="${HOST_SCRIPT_WRK_DIR}/template"
HOST_NGINX_CFG_TEMPLATE_FILE="${HOST_NGINX_CFG_TEMPLATE_DIR}/nginx.conf.template"    ## nginx config template - file
HOST_VM1_DOCKER_NGINX_SRV_NAME="proxy"    ## nginx on vm1 docker container - name
HOST_VM1_DOCKER_NGINX_SRV_INT_PORT="443"    ## nginx on vm1 docker container local - port
HOST_VM2_DOCKER_APACHE_SRV_NAME="web"    ## nginx on vm1 docker container - name
HOST_VM2_DOCKER_APACHE_SRV_INT_PORT="80"    ## nginx on vm1 docker container local - port
HOST_VM1_DOCKER_NGINX_YML_FILE="${HOST_DOCKER_CFG_DIR}/vm1-docker-compose.yml"    ## nginx docker-compose yml - file
HOST_VM2_DOCKER_APACHE_YML_FILE="${HOST_DOCKER_CFG_DIR}/vm2-docker-compose.yml"    ## apache docker-compose yml - file
HOST_VM1_DOCKER_NGINX_YML_TEMPLATE_FILE="${HOST_NGINX_CFG_TEMPLATE_DIR}/vm1-docker-compose.yml.template"    ## nginx docker-compose yml template - file
HOST_VM2_DOCKER_APACHE_YML_TEMPLATE_FILE="${HOST_NGINX_CFG_TEMPLATE_DIR}/vm2-docker-compose.yml.template"    ## apache docker-compose yml template - file
SSL_HOST_NAME=${VM1_NAME}    ## host name for ssl certificate - name
## ------------------------
## virsh network variable
## ------------------------
VI_BR_PREFIX="virbr"    ## prefix for virtual bridge - name
EXT_NETWORK_DHCP_RANGE_BEGIN_IP="2"    ## start ip range for external network dhcp - ip
EXT_NETWORK_DHCP_RANGE_END_IP="254"    ## end ip range for external network dhcp - ip
EXT_NETWORK_VM1_MAC=52:54:00:`(date; cat /proc/interrupts) | md5sum | sed -r 's/^(.{6}).*$/\1/; s/([0-9a-f]{2})/\1:/g; s/:$//;'`    ## generate mac address for vm1 in virtual external network - mac
EXT_NETWORK_VIBR_NAME="${VI_BR_PREFIX}-${VM1_EXTERNAL_IF}"    ## virtual external network - name
HOST_EXT_NETWORK_CFG_FILE="${HOST_NETWORK_XML_CFG_DIR}/${EXTERNAL_NET_NAME}.xml"    ## external virtual network config xml - file
INT_NETWORK_VIBR_NAME="${VI_BR_PREFIX}-${VM1_INTERNAL_IF}"    ## virtual internal network - name 
HOST_INT_NETWORK_CFG_FILE="${HOST_NETWORK_XML_CFG_DIR}/${INTERNAL_NET_NAME}.xml"    ## internal virtual network config xml - file
MGM_NETWORK_VIBR_NAME="${VI_BR_PREFIX}-${VM1_MANAGEMENT_IF}"    ## virtual managment network - name
HOST_MGM_NETWORK_CFG_FILE="${HOST_NETWORK_XML_CFG_DIR}/${MANAGEMENT_NET_NAME}.xml"    ## managment virtual network config xml - file
VM1_MAX_TIMEOUT="60"    ## timeout for wait vm1 start services - second
## ------------------------
## vm1 host variable
## ------------------------
VM1_HOST_CFG_AND_CERTS_DIR="/srv"    ## on vm1 config docker, nginx and certifitace - dir
VM1_HOST_DOCKER_CFG_DIR="${VM1_HOST_CFG_AND_CERTS_DIR}/docker-cfg"    ## on vm1 docker config ylm - dir
VM1_HOST_DOCKER_CFG_FILE="${VM1_HOST_DOCKER_CFG_DIR}/docker-compose.yml"    ## on vm1 docker config ylm - dir
VM1_HOST_NGINX_SSL_DIR="${VM1_HOST_CFG_AND_CERTS_DIR}/certs"    ## on vm1 ssl key and sertificate - dir
VM1_HOST_NGINX_CFG_DIR="${VM1_HOST_CFG_AND_CERTS_DIR}/nginx-cfg"    ## on vm1 nginx config - dir
VM1_HOST_NGINX_CFG_FILE="${VM1_HOST_NGINX_CFG_DIR}/nginx.conf"    ## on vm1 nginx.conf config - file
VM1_HOST_NGINX_LOG="${NGINX_LOG_DIR}"    ## on vm1 nginx log - dir
## ------------------------
## vm1 - docker - nginx variable
## ------------------------
VM1_DOCKER_NGINX_SSL_DIR="/etc/nginx/certs"    ## nginx docker container on vm1 ssl key and certificate - dir
VM1_DOCKER_NGINX_CFG_FILE="/etc/nginx/nginx.conf"    ## nginx docker container on vm1 config - file
VM1_DOCKER_NGINX_LOG_DIR="/var/log/nginx"    ## nginx on vm1 docker container log - dir
## ------------------------
## vm1 host variable
## ------------------------
VM2_HOST_CFG_AND_CERTS_DIR="/srv"    ## on vm2 docker config - dir
VM2_HOST_DOCKER_CFG_DIR="${VM2_HOST_CFG_AND_CERTS_DIR}/docker-cfg"    ## on vm2 docker config ylm - dir
VM2_HOST_DOCKER_CFG_FILE="${VM2_HOST_DOCKER_CFG_DIR}/docker-compose.yml"    ## on vm2 docker config ylm - dir
## ------------------------
## ========================================================================

## ------------------------
## generate id_rsa
## ------------------------
if [ ! -e "$SSH_PUB_KEY" ]; then 
  ssh-keygen -t rsa -b 4096 -f "${SSH_PUB_KEY%.pub}" -q -N ""
fi

## ------------------------
## install packages
## ------------------------
apt-get update
apt-get -y install ssh openssh-server openssl
apt-get -y install apt-transport-https ca-certificates curl software-properties-common
apt-get -y install qemu-kvm libvirt-bin virtinst virt-viewer bridge-utils genisoimage
apt-get -y install mc virt-top libvirt-doc git nmap tree

## ------------------------
## download virtual mashine
## ------------------------
wget -c "${VM_BASE_IMAGE}" || exit 1

## ------------------------
## create dir tree
## ------------------------
func_gen_dir_tree

## generate ssl CA and web
func_ssl_gen "${HOST_SSL_DIR}" "${SSL_HOST_NAME}" "${VM1_EXTERNAL_IP}"

## generate nginx config
eval "echo \"$(cat ${HOST_NGINX_CFG_TEMPLATE_FILE})\"" > "${HOST_NGINX_CFG_FILE}"

## generate docker-compose.yml from template
#eval "echo \"$(cat ${DOCKER_YML_TEMPLATE_FILE})\"" > ${DOCKER_YML_FILE}
eval "echo \"$(cat ${HOST_VM1_DOCKER_NGINX_YML_TEMPLATE_FILE})\"" > "${HOST_VM1_DOCKER_NGINX_YML_FILE}"
eval "echo \"$(cat ${HOST_VM2_DOCKER_APACHE_YML_TEMPLATE_FILE})\"" > "${HOST_VM2_DOCKER_APACHE_YML_FILE}"

## create netwoks config
## generate external.xml
func_gen_conf_ext
## generate internal.xml
func_gen_conf_int
## generate management.xml
func_gen_conf_mgm

## define and start all networks
func_create_net "${EXTERNAL_NET_NAME}" "${HOST_EXT_NETWORK_CFG_FILE}"
func_create_net "${INTERNAL_NET_NAME}" "${HOST_INT_NETWORK_CFG_FILE}"
func_create_net "${MANAGEMENT_NET_NAME}" "${HOST_MGM_NETWORK_CFG_FILE}"

# debug 
virsh net-list --all

## make user-data and meta-data  based on config
func_gen_cludinit_conf_vm1 "${HOST_CLOUDINIT_CFG_DRV_VM1_DIR}"
func_gen_cludinit_conf_vm2 "${HOST_CLOUDINIT_CFG_DRV_VM2_DIR}"

## make cloud-init config iso for vm1 and vm2
genisoimage -output "${VM1_CONFIG_ISO}" -volid cidata -joliet -input-charset utf-8 -rock "${HOST_CLOUDINIT_CFG_DRV_VM1_DIR}"/{user-data,meta-data}
genisoimage -output "${VM2_CONFIG_ISO}" -volid cidata -joliet -input-charset utf-8 -rock "${HOST_CLOUDINIT_CFG_DRV_VM2_DIR}"/{user-data,meta-data}

## ------------------------
## deploy vm1
## ------------------------
func_deploy_vm "${VM1_NAME}" "${VM1_HDD}" "${VM1_CONFIG_ISO}" "${VM1_NUM_CPU}" "${VM1_MB_RAM}" "--network network=${EXTERNAL_NET_NAME},model=virtio,mac=${EXT_NETWORK_VM1_MAC}"

## wait for vm1 ready
## wait dhcp for vm1
func_wait_dhcp "${VM1_MAX_TIMEOUT}" "${EXTERNAL_NET_NAME}" "${EXT_NETWORK_VM1_MAC}" "${VM1_NAME}"
## waiting for a response from the service ssh on port 22
func_wait_ssh "${VM1_MAX_TIMEOUT}" "${VM1_EXTERNAL_IP}" 22 "${VM1_NAME}"
## wait authorized_keys copy to root
func_wait_root_id_rsa "${VM1_MAX_TIMEOUT}" "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "${VM1_NAME}"

## make dir for docker + nginx config on vm1
func_vm_ssh_cmd "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "mkdir -p ${VM1_HOST_DOCKER_CFG_DIR}"
func_vm_ssh_cmd "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "mkdir -p ${VM1_HOST_NGINX_CFG_DIR}"
func_vm_ssh_cmd "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "mkdir -p ${VM1_HOST_NGINX_SSL_DIR}"
func_vm_ssh_cmd "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "mkdir -p ${NGINX_LOG_DIR}"
## copy config files to vm1
func_vm_scp "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "${HOST_VM1_DOCKER_NGINX_YML_FILE}" "${VM1_HOST_DOCKER_CFG_FILE}"
func_vm_scp "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "${HOST_NGINX_CFG_FILE}" "${VM1_HOST_NGINX_CFG_FILE}"
func_vm_scp "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" "${HOST_SSL_DIR}" "${VM1_HOST_CFG_AND_CERTS_DIR}"

## wait for docker install finish



## start docker nginx
#func_vm_ssh_cmd "${SSH_PUB_KEY%.pub}" "${VM1_MANAGEMENT_IP}" 'docker-compose -f "${VM1_HOST_DOCKER_CFG_FILE}" up -d'

## ------------------------
## deploy vm2
## ------------------------
func_deploy_vm "${VM2_NAME}" "${VM2_HDD}" "${VM2_CONFIG_ISO}" "${VM2_NUM_CPU}" "${VM2_MB_RAM}"

## wait for vm2 ready
## wait authorized_keys copy to root
func_wait_root_id_rsa "${VM1_MAX_TIMEOUT}" "${SSH_PUB_KEY%.pub}" "${VM2_MANAGEMENT_IP}" "${VM2_NAME}"

## wait for docker install finish



## make dir for docker config on vm2
func_vm_ssh_cmd "${SSH_PUB_KEY%.pub}" "${VM2_MANAGEMENT_IP}" "mkdir -p ${VM2_HOST_DOCKER_CFG_DIR}"
## copy config files to vm2
func_vm_scp "${SSH_PUB_KEY%.pub}" "${VM2_MANAGEMENT_IP}" "${HOST_VM2_DOCKER_APACHE_YML_FILE}" "${VM2_HOST_DOCKER_CFG_FILE}"

# debug
virsh list --all
curl --cacert ~alexey/task10_12_3/docker/certs/root-ca.crt https://192.168.123.101:17080

exit 0

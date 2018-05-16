function func_gen_dir_tree {
## path to host ssl - dir
if [ ! -d "${HOST_SSL_DIR}" ]; then
  mkdir -p "${HOST_SSL_DIR}"
fi
## path to nginx.conf - dir
if [ ! -e "${HOST_NGINX_CFG_DIR}" ]; then
  mkdir -p "${HOST_NGINX_CFG_DIR}"
fi
## path to vm1 cloud-init files - dir
if [ ! -e "${HOST_CLOUDINIT_CFG_DRV_VM1_DIR}" ]; then
  mkdir -p "${HOST_CLOUDINIT_CFG_DRV_VM1_DIR}"
fi
## path to vm2 cloud-init files - dir
if [ ! -e "${HOST_CLOUDINIT_CFG_DRV_VM2_DIR}" ]; then
  mkdir -p "${HOST_CLOUDINIT_CFG_DRV_VM2_DIR}"
fi
## path to network exl config files - dir
if [ ! -e "${HOST_NETWORK_XML_CFG_DIR}" ]; then
  mkdir -p "${HOST_NETWORK_XML_CFG_DIR}"
fi
## vm1 hdd path - dir
if [ ! -e "${VM1_HDD%\/*}" ]; then
  mkdir -p "${VM1_HDD%\/*}"
fi
## vm2 hdd path - dir
if [ ! -e "${VM2_HDD%\/*}" ]; then
  mkdir -p "${VM2_HDD%\/*}"
fi
}

function func_gen_conf_ext {
cat <<EOF > ${HOST_EXT_NETWORK_CFG_FILE}
<network>
  <name>${EXTERNAL_NET_NAME}</name>
  <forward mode='nat'/>
  <bridge name='${EXT_NETWORK_VIBR_NAME}' stp='on' delay='0'/>
  <ip address='${EXTERNAL_NET_HOST_IP}' netmask='${EXTERNAL_NET_MASK}'>
    <dhcp>
      <range start='${EXTERNAL_NET}.${EXT_NETWORK_DHCP_RANGE_BEGIN_IP}' end='${EXTERNAL_NET}.${EXT_NETWORK_DHCP_RANGE_END_IP}'/>
      <host mac='${EXT_NETWORK_VM1_MAC}' name='${VM1_NAME}' ip='${VM1_EXTERNAL_IP}'/>
    </dhcp>
  </ip>
</network>
EOF
}

function func_gen_conf_int {
cat <<EOF > ${HOST_INT_NETWORK_CFG_FILE}
<network>
  <name>${INTERNAL_NET_NAME}</name>
  <bridge name='${INT_NETWORK_VIBR_NAME}'/>
</network>
EOF
}

function func_gen_conf_mgm {
cat <<EOF > ${HOST_MGM_NETWORK_CFG_FILE}
<network>
  <name>${MANAGEMENT_NET_NAME}</name>
  <bridge name='${MGM_NETWORK_VIBR_NAME}'/>
  <ip address='${MANAGEMENT_HOST_IP}' netmask='${MANAGEMENT_NET_MASK}'/>
</network>
EOF
}

function func_create_net {
  ## $1=${EXTERNAL_NET_NAME}
  ## $2=${HOST_EXT_NETWORK_CFG_FILE}
  virsh net-define $2
  virsh net-autostart $1
  virsh net-start $1
}

function func_gen_cludinit_conf_vm1 {
## meta-data
cat <<EOF > $1/meta-data
instance-id: iid-$VM1_NAME
hostname: $VM1_NAME
local-hostname: $VM1_NAME
network-interfaces: |
  auto $VM1_EXTERNAL_IF
  iface $VM1_EXTERNAL_IF inet dhcp

  auto $VM1_INTERNAL_IF
  iface $VM1_INTERNAL_IF inet static
  address $VM1_INTERNAL_IP
  network $INTERNAL_NET_IP
  netmask $INTERNAL_NET_MASK

  auto $VM1_MANAGEMENT_IF
  iface $VM1_MANAGEMENT_IF inet static
  address $VM1_MANAGEMENT_IP
  network $MANAGEMENT_NET_IP
  netmask $MANAGEMENT_NET_MASK
EOF

## user-data
cat <<EOF > $1/user-data
#cloud-config
#password: qwerty
#chpasswd: { expire: False }
ssh_authorized_keys:
  - $(cat $SSH_PUB_KEY)

runcmd:
 - [ sh, -c, "/sbin/sysctl net.ipv4.ip_forward=1" ]
 - [ sh, -c, "/sbin/iptables -t nat -A POSTROUTING -s ${VM2_INTERNAL_IP} -o ${VM1_EXTERNAL_IF} -j MASQUERADE" ]
 - [ sh, -c, "/sbin/ip link add ${VXLAN_IF} type vxlan id ${VID} remote ${VM2_INTERNAL_IP} local ${VM1_INTERNAL_IP} dstport 4789" ]
 - [ sh, -c, "/sbin/ip link set ${VXLAN_IF} up" ]
 - [ sh, -c, "/sbin/ip addr add ${VXLAN_NET}.${VM1_INTERNAL_IP##*.}/24 dev $VXLAN_IF" ]
 - [ sh, -c, "/usr/bin/curl -fsSL https://download.docker.com/linux/ubuntu/gpg | /usr/bin/apt-key add -" ]
 - [ sh, -c, '/usr/bin/add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"' ]
 - [ sh, -c, "/usr/bin/apt-get update" ]
 - [ sh, -c, "/usr/bin/apt-get -y install docker-ce docker-compose bridge-utils tree" ]
 - [ sh, -c, "mkdir -p /root/.ssh" ]
 - [ sh, -c, "cat /home/ubuntu/.ssh/authorized_keys > /root/.ssh/authorized_keys" ]
 - [ sh, -c, "chmod 600 /root/.ssh/authorized_keys" ]
EOF
}

function func_gen_cludinit_conf_vm2 {
## meta-data
cat <<EOF > $1/meta-data
instance-id: iid-$VM2_NAME
hostname: $VM2_NAME
local-hostname: $VM2_NAME
network-interfaces: |
  auto $VM2_INTERNAL_IF
  iface $VM2_INTERNAL_IF inet static
  address $VM2_INTERNAL_IP
  network $INTERNAL_NET_IP
  netmask $INTERNAL_NET_MASK
  gateway $VM1_INTERNAL_IP
  dns-nameservers $VM_DNS

  auto ens4
  iface ens4 inet static
  address $VM2_MANAGEMENT_IP
  network $MANAGEMENT_NET_IP
  netmask $MANAGEMENT_NET_MASK
EOF

## user-data
cat <<EOF > $1/user-data
#cloud-config
#password: qwerty
#chpasswd: { expire: False }
ssh_authorized_keys:
  - $(cat $SSH_PUB_KEY)

runcmd:
 - [ sh, -c, "/sbin/ip link add ${VXLAN_IF} type vxlan id ${VID} remote ${VM1_INTERNAL_IP} local ${VM2_INTERNAL_IP} dstport 4789" ]
 - [ sh, -c, "/sbin/ip link set ${VXLAN_IF} up" ]
 - [ sh, -c, "/sbin/ip addr add ${VXLAN_NET}.${VM2_INTERNAL_IP##*.}/24 dev $VXLAN_IF" ]
 - [ sh, -c, "/usr/bin/curl -fsSL https://download.docker.com/linux/ubuntu/gpg | /usr/bin/apt-key add -" ]
 - [ sh, -c, '/usr/bin/add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"' ]
 - [ sh, -c, "/usr/bin/apt-get update" ]
 - [ sh, -c, "/usr/bin/apt-get -y install docker-ce docker-compose bridge-utils tree" ]
 - [ sh, -c, "mkdir -p /root/.ssh" ]
 - [ sh, -c, "cat /home/ubuntu/.ssh/authorized_keys > /root/.ssh/authorized_keys" ]
 - [ sh, -c, "chmod 600 /root/.ssh/authorized_keys" ]
EOF
}

function func_wait_dhcp {
  echo "start wait dhcp, limit = $1 sec"
  local WAIT_DELAY_DHCP="$1"
  local WAIT_NET_NAME_DCHP="$2"
  local WAIT_MAC_DHCP="$3"
  local WAIT_HOST_NAME_DHCP="$4"
  while [[ ${WAIT_COUNTER_DHCP} -lt ${WAIT_DELAY_DHCP} ]]
  do
      (( WAIT_COUNTER_DHCP++ ))
      virsh net-dhcp-leases ${WAIT_NET_NAME_DCHP} | grep "${WAIT_MAC_DHCP}"
      if [[ $? -eq 0 ]]; then
          WAIT_COUNTER_DHCP="${WAIT_DELAY_DHCP}"
      fi
      sleep 1s
      echo "wait DHCP for ${WAIT_HOST_NAME_DHCP}: ${WAIT_COUNTER_DHCP} of ${WAIT_DELAY_DHCP}"
  done
  echo "stop wait dhcp"
}

function func_wait_ssh {
  echo "start wait for ssh ervice, linit = $1 sec"
  local WAIT_DELAY_SSH="$1"
  local WAIT_IP_SSH="$2"
  local WAIT_PORT_SSH="$3"
  local WAIT_HOST_NAME_SSH="$4"
  while [[ ${WAIT_COUNTER_SSH} -lt ${WAIT_DELAY_SSH} ]]
  do
      (( WAIT_COUNTER_SSH++ ))
      nmap -p${WAIT_PORT_SSH} ${WAIT_IP_SSH} | grep "ssh"
      if [[ $? -eq 0 ]]; then
          WAIT_COUNTER_SSH="${WAIT_DELAY_SSH}"
      fi
      sleep 1s
      echo "wait SSH for ${WAIT_HOST_NAME_SSH}: ${WAIT_COUNTER_SSH} of ${WAIT_DELAY_SSH}"
  done
  echo "stop wait for ssh"
}

function func_wait_root_id_rsa {
  echo "start wait for root id_rsa, limit = $1 sec"
  local WAIT_DELAY_ID_RSA="$1"
  local WAIT_KEY_ID_RSA="$2"
  local WAIT_IP_ID_RSA="$3"
  local WAIT_HOST_NAME_ID_RSA="$4"
  local 
  while [[ ${WAIT_COUNTER_ID_RSA} -lt ${WAIT_DELAY_ID_RSA} ]]
  do
      (( WAIT_COUNTER_ID_RSA++ ))
ssh -o StrictHostKeyChecking=no -i ${WAIT_KEY_ID_RSA} root@${WAIT_IP_ID_RSA} "cat /root/.ssh/authorized_keys" | grep "ssh-rsa"
      if [[ $? -eq 0 ]]; then
          WAIT_COUNTER_ID_RSA="${WAIT_DELAY_ID_RSA}"
      fi
      sleep 1s
      echo "wait AUTORIZED KEY for ${WAIT_HOST_NAME_ID_RSA}: ${WAIT_COUNTER_ID_RSA} of ${WAIT_DELAY_ID_RSA}"
  done
  echo "stop wait for root id_rsa"
  WAIT_COUNTER_ID_RSA=0
}

function func_wait_docker {
echo docker
}


function func_deploy_vm {
  local FN_VM_NAME=$1
  local FN_VM_HDD=$2
  local FN_VM_CONFIG_ISO=$3
  local FN_VM_NUM_CPU=$4
  local FN_VM_MB_RAM=$5
  local FN_VM_EXTERNAL_NET=$6

  qemu-img convert -O qcow2 xenial-server-cloudimg-amd64-disk1.img $FN_VM_HDD

  # Create vm
  virt-install \
    --os-type=linux \
    --os-variant=ubuntu16.04 \
    --graphics vnc,port=-1,listen=0.0.0.0 \
    --noautoconsole \
    --${VM_TYPE} \
    --virt-type ${VM_VIRT_TYPE} \
    --ram ${FN_VM_MB_RAM} \
    --vcpus=${FN_VM_NUM_CPU} \
    --name $FN_VM_NAME \
    --disk path=$FN_VM_HDD,format=qcow2,bus=virtio,cache=none \
    --disk path=$FN_VM_CONFIG_ISO,device=cdrom \
    $FN_VM_EXTERNAL_NET \
    --network network=$INTERNAL_NET_NAME,model=virtio \
    --network network=$MANAGEMENT_NET_NAME,model=virtio
}

function func_ssl_gen {
local FN_SSL_GEN_DIR="$1"    ## output dir
local FN_SSL_GEN_HOST_NAME="$2"    ## host name
local FN_SSL_GEN_IP="$3"    ## ip address

## generate root CA key
openssl genrsa -out ${FN_SSL_GEN_DIR}/root-ca.key 4096

## generate root CA certivicate
openssl req -x509 -new -nodes -key ${FN_SSL_GEN_DIR}/root-ca.key -sha256 -days 365 -out ${FN_SSL_GEN_DIR}/root-ca.crt -subj "/C=UA/ST=Kharkov/L=Kharkov/O=Podrepny/OU=web/CN=root_cert/"

## generate nginx key
openssl genrsa -out ${FN_SSL_GEN_DIR}/web.key 2048

## generate nginx certificate signing request
openssl req -new -out ${FN_SSL_GEN_DIR}/web.csr -key ${FN_SSL_GEN_DIR}/web.key -subj "/C=UA/ST=Kharkov/L=Kharkov/O=Podrepny/OU=web/CN=${FN_SSL_GEN_HOST_NAME}/"

## signing a nginx CSR with a root certificate
openssl x509 -req -in ${FN_SSL_GEN_DIR}/web.csr -CA ${FN_SSL_GEN_DIR}/root-ca.crt -CAkey ${FN_SSL_GEN_DIR}/root-ca.key -CAcreateserial -out ${FN_SSL_GEN_DIR}/web.crt -days 365 -sha256 -extfile <(echo -e "authorityKeyIdentifier=keyid,issuer\nbasicConstraints=CA:FALSE\nkeyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment\nsubjectAltName = @alt_names\n[ alt_names ]\nDNS.1 = ${FN_SSL_GEN_HOST_NAME}\nIP.1 = ${FN_SSL_GEN_IP}")

## combining two certificates (nginx and root CA) to web.pem
cat ${FN_SSL_GEN_DIR}/web.crt ${FN_SSL_GEN_DIR}/root-ca.crt > ${FN_SSL_GEN_DIR}/web.pem
}

function func_vm_ssh_cmd {
local FN_VM_SSH_KEY_ID_RSA="$1"    ## host local key id_rsa
local FN_VM_SSH_IP="$2"            ## destanation ip
local FN_VM_SSH_CMD="$3"           ## comand for destanation host
ssh -o StrictHostKeyChecking=no -i "${FN_VM_SSH_KEY_ID_RSA}" "root@${FN_VM_SSH_IP}" "${FN_VM_SSH_CMD}"
}

function func_vm_scp {
local FN_VM_SCP_KEY_ID_RSA="$1"   ## host local key id_rsa
local FN_VM_SCP_IP="$2"           ## destanation ip
local FN_VM_SCP_SOURCE="$3"       ## host local source path
local FN_VM_SCP_DESTANATION="$4"  ## remote destanation path
scp -o StrictHostKeyChecking=no -r -i "${FN_VM_SCP_KEY_ID_RSA}" "${FN_VM_SCP_SOURCE}" "root@${FN_VM_SCP_IP}:${FN_VM_SCP_DESTANATION}"
}

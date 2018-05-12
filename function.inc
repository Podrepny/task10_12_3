function func_gen_conf_ext {
cat <<EOF > ${EXT_XML_PATH}
<network>
  <name>${EXTERNAL_NET_NAME}</name>
  <forward mode='nat'/>
  <bridge name='${EXT_VIBR_NAME}' stp='on' delay='0'/>
  <ip address='${EXTERNAL_NET_HOST_IP}' netmask='${EXTERNAL_NET_MASK}'>
    <dhcp>
      <range start='${EXTERNAL_NET}.${EXT_DHCP_IP_RANGE_BEGIN}' end='${EXTERNAL_NET}.${EXT_DHCP_IP_RANGE_END}'/>
      <host mac='${EXT_VM1_MAC}' name='${VM1_NAME}' ip='${VM1_EXTERNAL_IP}'/>
    </dhcp>
  </ip>
</network>
EOF
}

function func_gen_conf_int {
cat <<EOF > ${INT_XML_PATH}
<network>
  <name>${INTERNAL_NET_NAME}</name>
  <bridge name='${INT_VIBR_NAME}'/>
</network>
EOF
}

function func_gen_conf_mgm {
cat <<EOF > ${MGM_XML_PATH}
<network>
  <name>${MANAGEMENT_NET_NAME}</name>
  <bridge name='${MGM_VIBR_NAME}'/>
  <ip address="${MANAGEMENT_HOST_IP}" netmask="${MANAGEMENT_NET_MASK}"/>
</network>
EOF
}

function func_create_net {
  ## $1=${EXTERNAL_NET_NAME}
  ## $2=${EXT_XML_PATH}
  virsh net-define $2
  virsh net-autostart $1
  virsh net-start $1
}

function func_gen_cludinit_conf_vm1 {
  mkdir -p $1

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
 - [ sh, -c, "/usr/bin/apt-get -y install docker-ce docker-compose bridge-utils" ]
# - [ sh, -c, "/bin/mount -o ro /dev/sr0 /media" ]
# - [ sh, -c, "/bin/mkdir -p ${VM1_NGINX_SSL_DIR} ${VM1_NGINX_CFG_DIR} ${VM1_DOCKER_COMPOSE_FILE}" ]
# - [ sh, -c, "/bin/mkdir -p ${NGINX_LOG_DIR}" ]
# - [ sh, -c, "/bin/cp -fr /media/certs/* ${VM1_NGINX_SSL_DIR}" ]
# - [ sh, -c, "/bin/cp -f /media/etc/nginx.conf ${VM1_NGINX_CFG_DIR}/" ]
# - [ sh, -c, "/bin/cp -f /media/docker-compose.yml ${VM1_DOCKER_COMPOSE_FILE}/" ]
# - [ sh, -c, "/bin/umount /media" ]
# - [ sh, -c, "/bin/echo OOKK" ]
# - [ sh, -c, "cd ${VM1_DOCKER_COMPOSE_FILE} ; /bin/pwd ; /usr/bin/docker-compose --verbose up -d" ]
# - [ sh, -c, "/usr/bin/docker-compose --verbose up -d" ]
# - [ sh, -c, "/bin/sleep 25" ]
# - [ sh, -c, "CNT=0; while :; do /usr/bin/docker-compose ps | /bin/grep Up > /dev/null ; if [ $? -eq \"0\" ]; then /bin/echo Yes ; if [ \"${CNT}\" -ge \"240\" ]; then /bin/echo \"TIMEOUT\" ; break; fi ; else /bin/echo NO ; break ; fi ; /bin/sleep 0.5 ; ((CNT++)) ; /bin/echo \"CNT: ${CNT}\" ; done" ]
# - [ sh, -c, "/bin/sleep 5 ; /usr/bin/docker ps ; /bin/date" ]

#!/bin/bash
mount /dev/cdrom /mnt/
#mkdir -p /root/.ssh
#cat /home/ubuntu/.ssh/authorized_keys > /root/.ssh/authorized_keys
#chmod 600 /root/.ssh/authorized_keys
mkdir -p /srv/rrrrr
EOF
}

function func_gen_cludinit_conf_vm2 {
  mkdir -p $1

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
 - [ sh, -c, "/usr/bin/apt-get -y install docker-ce" ]
EOF
}

function func_wait_dhcp {
  local WAIT_DELAY=$1
  while [[ ${WAIT_COUNTER} -lt ${WAIT_DELAY} ]]
  do
      (( WAIT_COUNTER++ ))
      virsh net-dhcp-leases ${EXTERNAL_NET_NAME} | grep "${EXT_VM1_MAC}"
      if [[ $? -eq 0 ]]; then
          WAIT_COUNTER="${WAIT_DELAY}"
      fi
      sleep 1s
      echo "wait DHCP for ${VM1_NAME}: ${WAIT_COUNTER} of ${WAIT_DELAY}"
  done
}

function func_deploy_vm {
  local VM_NAME=$1
  local VM_HDD=$2
  local VM_HDD_DIR="${VM_HDD%\/*}"
  local VM_CONFIG_ISO=$3
  local VM_NUM_CPU=$4
  local VM_MB_RAM=$5
  local EXTERNAL_NET=$6

  # Generate meta-data iso file
  mkdir -p $VM_HDD_DIR

  genisoimage -output $VM_CONFIG_ISO -volid cidata -joliet -input-charset utf-8 -rock $CLOUDINIT_CONF_DIR/$VM_NAME-config/{user-data,meta-data} $WRK_DIR_PATH/docker/

  qemu-img convert -O qcow2 xenial-server-cloudimg-amd64-disk1.img $VM_HDD

  # Create vm
  virt-install \
    --os-type=linux \
    --os-variant=ubuntu16.04 \
    --graphics vnc,port=-1,listen=0.0.0.0 \
    --noautoconsole \
    --${VM_TYPE} \
    --virt-type ${VM_VIRT_TYPE} \
    --ram ${VM_MB_RAM} \
    --vcpus=${VM_NUM_CPU} \
    --name $VM_NAME \
    --disk path=$VM_HDD,format=qcow2,bus=virtio,cache=none \
    --disk path=$VM_CONFIG_ISO,device=cdrom \
    $EXTERNAL_NET \
    --network network=$INTERNAL_NET_NAME,model=virtio \
    --network network=$MANAGEMENT_NET_NAME,model=virtio
}

function func_dir_tree_gen {
if [ ! -d "${SSL_DIR}" ]; then
  mkdir -p "${SSL_DIR}"
fi

if [ ! -e "${NGINX_CFG_FILE}" ]; then
  mkdir -p "${NGINX_CFG_DIR}"
fi
touch "${NGINX_CFG_FILE}"

#if [ ! -d "${NGINX_LOG_DIR}" ]; then
#  mkdir -p "${NGINX_LOG_DIR}"
#fi
}

function func_ssl_gen {
## generate root CA key
openssl genrsa -out $SSL_DIR/root-ca.key 4096

## generate root CA certivicate
openssl req -x509 -new -nodes -key $SSL_DIR/root-ca.key -sha256 -days 365 -out $SSL_DIR/root-ca.crt -subj "/C=UA/ST=Kharkov/L=Kharkov/O=Podrepny/OU=web/CN=root_cert/"

## generate nginx key
openssl genrsa -out $SSL_DIR/web.key 2048

## generate nginx certificate signing request
openssl req -new -out $SSL_DIR/web.csr -key $SSL_DIR/web.key -subj "/C=UA/ST=Kharkov/L=Kharkov/O=Podrepny/OU=web/CN=$HOST_NAME/"

## signing a nginx CSR with a root certificate
openssl x509 -req -in $SSL_DIR/web.csr -CA $SSL_DIR/root-ca.crt -CAkey $SSL_DIR/root-ca.key -CAcreateserial -out $SSL_DIR/web.crt -days 365 -sha256 -extfile <(echo -e "authorityKeyIdentifier=keyid,issuer\nbasicConstraints=CA:FALSE\nkeyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment\nsubjectAltName = @alt_names\n[ alt_names ]\nDNS.1 = $HOST_NAME\nIP.1 = $VM1_EXTERNAL_IP")

## combining two certificates (nginx and root CA) to web.pem
cat $SSL_DIR/web.crt $SSL_DIR/root-ca.crt > $SSL_DIR/web.pem
}

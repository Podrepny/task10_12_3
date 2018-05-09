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
openssl x509 -req -in $SSL_DIR/web.csr -CA $SSL_DIR/root-ca.crt -CAkey $SSL_DIR/root-ca.key -CAcreateserial -out $SSL_DIR/web.crt -days 365 -sha256 -extfile <(echo -e "authorityKeyIdentifier=keyid,issuer\nbasicConstraints=CA:FALSE\nkeyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment\nsubjectAltName = @alt_names\n[ alt_names ]\nDNS.1 = $HOST_NAME\nIP.1 = $EXTERNAL_IP")
## combining two certificates (nginx and root CA) to web.pem
cat $SSL_DIR/web.crt $SSL_DIR/root-ca.crt > $SSL_DIR/web.pem
}

function func_dir_tree_gen {
if [ ! -d "${SSL_DIR}" ]; then
  mkdir -p "${SSL_DIR}"
fi

if [ ! -e "${NGINX_CFG_FILE}" ]; then
  mkdir -p "${NGINX_CFG_DIR}"
fi
touch "${NGINX_CFG_FILE}"

if [ ! -d ${NGINX_LOG_DIR} ]; then
  mkdir -p ${NGINX_LOG_DIR}
fi
}

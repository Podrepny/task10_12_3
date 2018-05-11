#!/bin/bash
virsh destroy vm1
virsh undefine vm1
virsh destroy vm2
virsh undefine vm2
virsh list --all
rm -rf /var/lib/libvirt/images/vm1/ /var/lib/libvirt/images/vm2/

for i in external internal management; do
    virsh net-destroy $i
    virsh net-undefine $i
done
virsh net-list --all

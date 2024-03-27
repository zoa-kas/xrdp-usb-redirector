#!/bin/sh

modprobe vhci-hcd

FILE="/sys/devices/platform/vhci_hcd.0/attach"

# Check if the group 'kur_users' exists, create it if it does not
if ! getent group kur_users > /dev/null; then
    groupadd --system kur_users
fi

# Change the file's group to 'kur_users'
chgrp kur_users "$FILE"

# Add read and write permissions for the group, without changing other set permissions
chmod ug+rw "$FILE"

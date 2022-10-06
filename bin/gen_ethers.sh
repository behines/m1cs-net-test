#!/bin/bash
#
# This script uses arp to generate a static /etc/ethers file for all of the segments.
# The benchmark.sh script should be run in the background before using this script in 
# order to get the mac address for every ip address.
sudo arp -a -n | grep enp2s0 | sed "s/^? (//" | sed "s/) at//" | sed "s/\[.*//" | awk ' { t = $1; $1 = $2; $2 = t; print; } '

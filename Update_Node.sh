#!/bin/bash

# Update CWallance Node (Raspberry)
# Command: ./Update_Node.sh

# To avoid to type the password, install the RSA Public/Private Key
# Generate RSA Public/Private Key: ssh-keygen -t rsa # ENTER to every field
# Copy Key to target: ssh-copy-id pi@ipaddr

# Find All Node IPs
IPs=$(sudo nmap -sn $(ip -o -f inet addr show | awk '{print $4}' | grep '192') | awk '/Nmap scan report for/{printf $5;}/MAC Address:/{print " => "$3;}' | grep "B8:27:EB" | awk '{print $1}')

for ip in $IPs
do
	echo "********** Update Node $ip **********"
	scp CWallance_Node.zip pi@$ip:/home/pi/
	ssh pi@$ip 'unzip CWallanceDDS_Node.zip && sudo ./Install_CWallanceDDS_Node.sh'
done
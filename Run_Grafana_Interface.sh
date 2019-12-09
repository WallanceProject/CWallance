#!/bin/bash

# Launch CWallance Grafana Interface (PC)
# Command: ./Run_Grafana_Interface.sh

# Init Grafana Server
if [ $(service grafana-server status | grep Active | awk '{print $2}') == "inactive" ]
then
	sudo service grafana-server restart
fi

# Init MYSQL
if [ $(service mysql status | grep Active | awk '{print $2}') == "inactive" ]
then
	sudo service mysql restart
fi

# Display Node's ID
echo "-------------------------"
echo "YOU ARE GRAFANA INTERFACE"
echo "-------------------------"

# Start Grafana Interface (Firefox)
/usr/bin/firefox -new-window http://localhost:3000/dashboard/script/CWallance_Dashboard.js &

# Start CWallance Node (Raspberry)
./Start_Node.sh

# Start Grafana Interface process
cd Grafana
./Grafana_Interface.bin

# Stop Grafana Interface (Firefox)
killall -q firefox

# Stop CWallance Node (Raspberry)
cd ../
./Stop_Node.sh

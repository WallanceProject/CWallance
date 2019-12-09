#!/bin/bash

# Compile & Export CWallance Project
# Command: ./Compile_CWallance.sh


# -------------
# CLEAN PROJECT
# -------------
rm -R -f Pipes
rm -f Consensus/*.db

for i in $(ls SmartContract)
do
	rm -f SmartContract/$i/*.bin
done

# SQLite3 Compilation Option
export CFLAGS='-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1'
mkdir -p Pipes


# ----------------
# NODE (RASPBERRY)
# ----------------
arm-linux-gnueabihf-g++ -o Node/Node.bin Node/Node.cpp Consensus/Consensus.cpp Consensus/SHA256.cpp ../SQLite3/sqlite3.o -lpthread -ldl


# ----------------------
# GRAFANA INTERFACE (PC)
# ----------------------
g++ -o Grafana/Grafana_Interface.bin Grafana/Grafana_Interface.cpp Consensus/SHA256.cpp


# ----------------------------------------
# GRAFANA INTERFACE - DASHBOARD PARAMETERS
# ----------------------------------------

# MySQLAccess.sh - MAJORITY_THRESHOLD
Majority_Threshold=$(grep '#define MAJORITY_THRESHOLD' Consensus/Consensus.h | cut -f 5)	# Delimiter: tab (x4 +1)
Numerator=$(echo $Majority_Threshold | cut -f2 -d '(' | cut -f1 -d '/')
Denominator=$(echo $Majority_Threshold | cut -f2 -d '/' | cut -f1 -d ')')
sed -i "s/MAJORITY_THRESHOLD=.*/MAJORITY_THRESHOLD=$Numerator\/$Denominator/g" Grafana/MySQLAccess.sh

# MySQLAccess.sh - DIFFICULTY
Difficulty=$(grep '#define DIFFICULTY' Consensus/Consensus.h | cut -f 7)	# Delimiter: tab (x6 +1)
sed -i "s/DIFFICULTY=.*/DIFFICULTY=$Difficulty/g" Grafana/MySQLAccess.sh

# MySQLAccess.sh - DCOIN_RATE
DCoin_Rate=$(grep '#define DCOIN_RATE' Consensus/Consensus.h | cut -f 7)	# Delimiter: tab (x6 +1)
sed -i "s/DCOIN_RATE=.*/DCOIN_RATE=$DCoin_Rate/g" Grafana/MySQLAccess.sh

# MySQLAccess.sh - DCOIN_REWARD
DCoin_Reward=$(grep '#define DCOIN_REWARD' Consensus/Consensus.h | cut -f 6)	# Delimiter: tab (x5 +1)
sed -i "s/DCOIN_REWARD=.*/DCOIN_REWARD=$DCoin_Reward/g" Grafana/MySQLAccess.sh

# MySQLAccess.sh - GENESIS_STATE
Genesis_State=$(grep '#define GENESIS_STATE' Consensus/Consensus.h | cut -f 6)	# Delimiter: tab (x5 +1)
sed -i "s/GENESIS_STATE=.*/GENESIS_STATE=$Genesis_State/g" Grafana/MySQLAccess.sh

# MySQLAccess.sh - TRANSACTION_OUTDATE
Transaction_Outdate=$(grep '#define TRANSACTION_OUTDATE' Consensus/Consensus.h | cut -f 5)	# Delimiter: tab (x4 +1)
sed -i "s/TRANSACTION_OUTDATE=.*/TRANSACTION_OUTDATE=$Transaction_Outdate/g" Grafana/MySQLAccess.sh

# CWallance_Dashboard.js - DCOIN_RATE
sed -i "s/SELECT .* AS DCOIN_RATE/SELECT $DCoin_Rate AS DCOIN_RATE/g" Grafana/CWallance_Dashboard.js


# -----------------------------------
# SENSOR & SMARTCONTRACTS (RASPBERRY)
# -----------------------------------

# Sensor
arm-linux-gnueabihf-g++ -o Sensor/Sensor.bin Sensor/Sensor.cpp

for i in $(ls SmartContract)
do
	rm -f SmartContract/$i/*.bin
	arm-linux-gnueabihf-g++ -o SmartContract/$i/$i.bin SmartContract/$i/$i.cpp
done


# --------------------------------------
# EXPORT CWALLANCE NODE - ARM (RASPBERRY)
# --------------------------------------

# Consensus (Repository Only)
zip CWallance_Node.zip Consensus

# Pipes (Repository Only)
zip -u CWallance_Node.zip Pipes

# CWallance Node Service
zip -u CWallance_Node.zip CWallance_Node.service

# Launcher Scripts
zip -u CWallance_Node.zip Run_Node.sh

# Node
zip -u CWallance_Node.zip Node/Node.bin

# Sensor
zip -u CWallance_Node.zip Sensor/Sensor.bin

# SmartContracts
zip -x SmartContract/*/*.c* -x SmartContract/*/*.h \
-u CWallance_Node.zip SmartContract/*/*

# Create Install CWallance Node Script
MyCMD='#!/bin/bash\n\n'
echo -e $MyCMD > Install_CWallance_Node.sh
echo -e "sudo apt-get update\nsudo apt-get install -y net-tools" >> Install_CWallance_Node.sh
echo "find \$(pwd) -type f -iname \"*.sh\" -exec chmod +x {} \;" >> Install_CWallance_Node.sh
echo "sudo mv CWallance_Node.service /etc/systemd/system/" >> Install_CWallance_Node.sh
echo "sudo systemctl daemon-reload" >> Install_CWallance_Node.sh
echo "sudo rm -f -R CWallance && mkdir CWallance" >> Install_CWallance_Node.sh
echo "mv Run_Node.sh Consensus Node SmartContract Sensor Pipes CWallance" >> Install_CWallance_Node.sh
echo "rm -f CWallance_Node.zip" >> Install_CWallance_Node.sh
echo "rm -f Install_CWallance_Node.sh" >> Install_CWallance_Node.sh
chmod +x Install_CWallance_Node.sh
zip -u CWallance_Node.zip Install_CWallance_Node.sh
rm Install_CWallance_Node.sh

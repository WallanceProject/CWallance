#!/bin/bash

# Grafana Database Access (mysql)
#
# $1 Mode:
#	0: Init CWALLANCE Database & Tables 
#	1: Update Wallet Counter
#	2: Add New Request Transaction
#	3: Add New Consensus Transaction
#	4: Consensus
#	5: Generate New Buying Request (From Web Interface To Grafana Interface)
#	6: Clear Database & Tables


# Parameters from Consensus.h
MAJORITY_THRESHOLD=2.0/3.0
DIFFICULTY=1
DCOIN_RATE=2
DCOIN_REWARD=2
GENESIS_STATE="0000000000000000000000000000000000000000000000000000000000000000"
TRANSACTION_OUTDATE=5



# Init CWALLANCE Database & Tables
if [ $1 -eq 0 ]; then
	mysql -u grafanaReader -e "CREATE DATABASE IF NOT EXISTS CWALLANCE;"
	mysql -u grafanaReader -e "SET @@SESSION.TIME_ZONE = '+00:00';"
	mysql -u grafanaReader -D CWALLANCE -e "CREATE TABLE IF NOT EXISTS WALLET (PUBLISHER VARCHAR(32), COUNTER INTEGER, STATE VARCHAR(64));"
	mysql -u grafanaReader -D CWALLANCE -e "CREATE TABLE IF NOT EXISTS REQUEST_TRANSACTIONS (PUBLISHER VARCHAR(32), SMARTCONTRACT VARCHAR(32), PRICE INTEGER, TIME INTEGER, PREVSTATE VARCHAR(64), OUTDATE INTEGER, UNIQUE(PUBLISHER, PREVSTATE));"
	mysql -u grafanaReader -D CWALLANCE -e "CREATE TABLE IF NOT EXISTS CONSENSUS_TRANSACTIONS (SUBSCRIBER VARCHAR(32), PUBLISHER VARCHAR(32), SMARTCONTRACT VARCHAR(32), PRICE INTEGER, TIME INTEGER, PREVSTATE VARCHAR(64), DCOIN INTEGER, OUTDATE INTEGER);"

	# Create SmartContract Database
	mysql -u grafanaReader -D CWALLANCE -e "CREATE TABLE IF NOT EXISTS SMARTCONTRACT (NAME VARCHAR(32) UNIQUE, PRICE INTEGER);"
	
	for i in $(ls ../SmartContract/)
	do
		if [ -f ../SmartContract/$i/$i.bin ]; then
			Name=$(echo $i | cut -f1 -d '_')
			Price=$(echo $i | cut -f2 -d '_' | cut -f1 -d '.')
			mysql -u grafanaReader -D CWALLANCE -e "INSERT INTO SMARTCONTRACT (NAME,PRICE) VALUES('$Name',$Price);"
		fi
	done


# Update Wallet Counter
elif [ $1 -eq 1 ]; then
	mysql -u grafanaReader -D CWALLANCE -e "SELECT COALESCE( (SELECT COUNTER FROM WALLET WHERE PUBLISHER='$2')+1,1) INTO @CPT; \
											SELECT COALESCE( (SELECT STATE FROM WALLET WHERE PUBLISHER='$2'),'$GENESIS_STATE') INTO @ST; \
											PREPARE STMT FROM 'INSERT INTO WALLET (PUBLISHER,COUNTER,STATE) VALUES (\'$2\', ?, ?)'; EXECUTE STMT USING @CPT,@ST;"
	mysql -u grafanaReader -D CWALLANCE -e "SELECT MAX(COUNTER) FROM WALLET WHERE PUBLISHER='$2' INTO @CPT; PREPARE STMT FROM 'DELETE FROM WALLET WHERE PUBLISHER=\'$2\' AND COUNTER <?'; EXECUTE STMT USING @CPT;"


# Add New Request Transaction
elif [ $1 -eq 2 ]; then
	# Compute LPoW (Subscriber - Publisher - SmartContract - Price - Time - PrevState - DCoin - Nonce)
	LPoW=$(echo -n $2$3$4$5$6$7$8$9 | sha256sum | cut -f1 -d ' ')

	# Check Light PoW && Existing SmartContract
	if [[ ${LPoW:0:$DIFFICULTY} == $(printf "%0.s0" $(seq 1 $DIFFICULTY)) ]] && [ -f ../SmartContract/$4'_'$5/$4'_'$5.bin ]; then

		# Manage old Consensus Responses of Requester
		mysql -u grafanaReader -D CWALLANCE -e "UPDATE CONSENSUS_TRANSACTIONS SET OUTDATE = OUTDATE-1 WHERE PUBLISHER='$3';"
		mysql -u grafanaReader -D CWALLANCE -e "DELETE FROM CONSENSUS_TRANSACTIONS WHERE OUTDATE <= 0;"

		# Manage old Request Transaction of Requester
		mysql -u grafanaReader -D CWALLANCE -e "UPDATE REQUEST_TRANSACTIONS SET OUTDATE = OUTDATE-1 WHERE PUBLISHER='$3';"
		mysql -u grafanaReader -D CWALLANCE -e "DELETE FROM REQUEST_TRANSACTIONS WHERE OUTDATE <= 0;"

		# Insert Request Transaction
		mysql -u grafanaReader -D CWALLANCE -e "INSERT INTO REQUEST_TRANSACTIONS (PUBLISHER,SMARTCONTRACT,PRICE,TIME,PREVSTATE,OUTDATE) VALUES ('$3','$4',$5,$6,'$7',$TRANSACTION_OUTDATE);"
	fi


# Add New Consensus Transaction
elif [ $1 -eq 3 ]; then
	# Compute LPoW (Subscriber - Publisher - SmartContract - Price - Time - PrevState - DCoin - Nonce)
	LPoW=$(echo -n $2$3$4$5$6$7$8$9 | sha256sum | cut -f1 -d ' ')

	# Check Light PoW && Existing SmartContract
	if [[ ${LPoW:0:$DIFFICULTY} == $(printf "%0.s0" $(seq 1 $DIFFICULTY)) ]] && [ -f ../SmartContract/$4'_'$5/$4'_'$5.bin ]; then

		# Insert Consensus Transaction
		mysql -u grafanaReader -D CWALLANCE -e "INSERT INTO CONSENSUS_TRANSACTIONS (SUBSCRIBER,PUBLISHER,SMARTCONTRACT,PRICE,TIME,PREVSTATE,DCOIN,OUTDATE) VALUES ('$2','$3','$4',$5,$6,'$7',$8,$TRANSACTION_OUTDATE);"
	fi


# Consensus
elif [ $1 -eq 4 ]; then
	# Find Majority
	while [ 1 -eq $(mysql -u grafanaReader -D CWALLANCE -s -e "SELECT EXISTS (SELECT 1 FROM CONSENSUS_TRANSACTIONS GROUP BY PUBLISHER, SMARTCONTRACT, PRICE, TIME, PREVSTATE, DCOIN HAVING PREVSTATE=(SELECT STATE FROM WALLET WHERE PUBLISHER=CONSENSUS_TRANSACTIONS.PUBLISHER) AND ROUND((SELECT COUNT(DISTINCT PUBLISHER) FROM WALLET)*($MAJORITY_THRESHOLD),0) <= COUNT(DISTINCT SUBSCRIBER) LIMIT 1);") ]
	do
		# Recover values of Majority group
		Result=$(mysql -u grafanaReader -D CWALLANCE -s -e "SELECT PUBLISHER, SMARTCONTRACT, PRICE, TIME, PREVSTATE, DCOIN FROM CONSENSUS_TRANSACTIONS GROUP BY PUBLISHER, SMARTCONTRACT, PRICE, TIME, PREVSTATE, DCOIN HAVING PREVSTATE=(SELECT STATE FROM WALLET WHERE PUBLISHER=CONSENSUS_TRANSACTIONS.PUBLISHER) AND ROUND((SELECT COUNT(DISTINCT PUBLISHER) FROM WALLET)*($MAJORITY_THRESHOLD),0) <= COUNT(DISTINCT SUBSCRIBER) LIMIT 1;")
	
		# Extract Publisher
		Publisher=$(echo $Result | cut -f1 -d ' ')

		# Extract SmartContract
		SmartContract=$(echo $Result | cut -f2 -d ' ')

		# Extract Price
		Price=$(echo $Result | cut -f3 -d ' ')

		# Extract Time
		Time=$(echo $Result | cut -f4 -d ' ')

		# Extract PrevState
		PrevState=$(echo $Result | cut -f5 -d ' ')

		# Extract DCoin
		DCoin=$(echo $Result | cut -f6 -d ' ')

		# Reward Participants (only first Participants && which have already shared sensor data)
		Limit=$(mysql -u grafanaReader -D CWALLANCE -s -e "SELECT ROUND((SELECT COUNT(DISTINCT PUBLISHER) FROM WALLET)*($MAJORITY_THRESHOLD),0);")
		mysql -u grafanaReader -D CWALLANCE -e "UPDATE WALLET SET COUNTER=COUNTER+$DCOIN_REWARD WHERE PUBLISHER IN (SELECT SUBSCRIBER FROM CONSENSUS_TRANSACTIONS WHERE PUBLISHER='$Publisher' AND SMARTCONTRACT='$SmartContract' AND PRICE=$Price AND Time=$Time AND PREVSTATE='$PrevState' AND DCOIN=$DCoin) LIMIT $Limit;"

		# Remove used Consensus Responses & Request Transactions
		mysql -u grafanaReader -D CWALLANCE -e "DELETE FROM CONSENSUS_TRANSACTIONS WHERE PUBLISHER='$Publisher' AND PREVSTATE='$PrevState';"
		mysql -u grafanaReader -D CWALLANCE -e "DELETE FROM REQUEST_TRANSACTIONS WHERE PUBLISHER='$Publisher' AND PREVSTATE='$PrevState';"

		# Update Wallet (State & Counter) of Publisher after Majority
		# Compute NewState (Publisher - SmartContract - Time - PrevState - DCoin)
		NewState=$(echo -n $Publisher$SmartContract$Price$Time$PrevState$DCoin | sha256sum | cut -f1 -d ' ')

		mysql -u grafanaReader -D CWALLANCE -e "DELETE FROM WALLET WHERE PUBLISHER='$Publisher'; \
												INSERT INTO WALLET (PUBLISHER,COUNTER,STATE) VALUES ('$Publisher', $DCoin*$DCOIN_RATE, '$NewState');"

		# Execute SmartContract (DEMO ONLY)
		#../SmartContract/$SmartContract_$Price/$SmartContract_$Price.bin $Publisher &
		/usr/bin/firefox -new-tab file:$HOME/WallanceProject/CWallance/SmartContract/Nespresso_2/Nespresso_2.png &

	done


# Generate New Request Transaction (From Web Interface To Grafana Interface)
# Input Format:  app:NAME_PRICE_PUBLISHER
# Output Format: NAME_PRICE_PUBLISHER_PREVSTATE
elif [ $1 -eq 5 ]; then

	# Extract PUBLISHER
	Publisher=$(echo $2 | cut -f3 -d '_')

	# Recover PrevState
	PrevState=$(mysql -u grafanaReader -D CWALLANCE -s -e "SELECT STATE FROM WALLET WHERE PUBLISHER='$Publisher';")

	# Send New Buying Request
	if [[ ! -z $PrevState ]]; then
		echo -n $(echo $2 | cut -f2 -d ':')_$PrevState >> $HOME/WallanceProject/CWallance/Pipes/Pipe_Web_to_Grafana
	fi


# Clear Database & Tables
else

 	mysql -u grafanaReader -D CWALLANCE -e "DROP TABLE IF EXISTS SMARTCONTRACT;"
 	mysql -u grafanaReader -D CWALLANCE -e "DROP TABLE IF EXISTS WALLET;"
 	mysql -u grafanaReader -D CWALLANCE -e "DROP TABLE IF EXISTS REQUEST_TRANSACTIONS;"
 	mysql -u grafanaReader -D CWALLANCE -e "DROP TABLE IF EXISTS CONSENSUS_TRANSACTIONS;"
 	mysql -u grafanaReader -D CWALLANCE -e "DROP DATABASE IF EXISTS CWALLANCE;"
fi
#include "Consensus.h"



/********** GLOBAL VARIABLES **********/

// ID of this Node
string MyID;

// SQLite3 Access
sqlite3* DB;






/********** CWALLANCE PROTOTYPE DEFINITIONS **********/

// Init CWallance Database
// argv: 	Launcher Command Arguments
// Return:	ERROR (-1) / OK (0)
int Init_CWallance(char **argv)
{
	// Create Node ID from Launcher Command Arguments
	MyID = argv[1];

	// Create SQLite3 Database
	if (sqlite3_open(DATABASE, &DB))
		return -1;

	// Create Table of Wallet
	if (sqlite3_exec(DB, INIT_WALLET, NULL, NULL, NULL) != SQLITE_OK)
		return -1;

	// Create Table of Buying Request
	if (sqlite3_exec(DB, INIT_REQUEST_TRANSACTION, NULL, NULL, NULL) != SQLITE_OK)
		return -1;

	// Create Table of Consensus
	if (sqlite3_exec(DB, INIT_CONSENSUS_TRANSACTION, NULL, NULL, NULL) != SQLITE_OK)
		return -1;

	return 0;
}



// Check if Publisher is already in the Database (Callback)
// MyResult:  Address to store the result
// Called by: Update_Wallet_Counter(string Publisher)
int SQLite_Exist_Publisher_Callback(void *MyResult, int NotUsed, char **Values, char **NotUsed2)
{
	// Check Correct Responses of SQLite
	// BOOL
	if (Values[0] == NULL)
		return -1;

	// New / Exist
	*((int*)MyResult) = atoi(Values[0]);
	return 0;
}



// Update Wallet Counter of Publisher for sharing a New Sensor value
// Publisher: Publisher ID of New Sensor value
void Update_Wallet_Counter(string Publisher)
{
	char MyCMD[DATA_FORMATING_LENGTH];
	int New = -1;

	snprintf(MyCMD, DATA_FORMATING_LENGTH, "SELECT COUNTER FROM WALLET WHERE PUBLISHER='%s';", Publisher.c_str());
	sqlite3_exec(DB, MyCMD, SQLite_Exist_Publisher_Callback, &New, NULL);

	// New Publisher
	if (New == -1)
	{
		// Create Publisher's Wallet
		snprintf(MyCMD, DATA_FORMATING_LENGTH, "INSERT INTO WALLET (PUBLISHER,COUNTER,STATE) VALUES('%s', 1, '%s');", Publisher.c_str(), GENESIS_STATE);
	}

	// Update Publisher's Wallet
	else
	{
		// Increase Wallet value by 1
		snprintf(MyCMD, DATA_FORMATING_LENGTH, "UPDATE WALLET SET COUNTER=COUNTER+1 WHERE PUBLISHER='%s';", Publisher.c_str());
	}

	sqlite3_exec(DB, MyCMD, NULL, NULL, NULL);
}



// Check the Hash of Light PoW according to the Difficulty (Number of first nibble to 0)
// MyString: Hash to check
// Return:	 ERROR (-1) / OK (0)
int Valid_Hash(string MyString)
{
	for (int i=0; i<DIFFICULTY; i++)
	{
	    if (MyString[i] != '0')
	        return -1;
	}
	
	return 0;
}



// Compute the Light Proof of Work (SHA256 of Transaction)
// MyTX:   Transaction to hash
// Return: Correct Nonce value
int Compute_Light_PoW(Transaction MyTX)
{	
	char MyCMD[DATA_FORMATING_LENGTH];
	string MyHash;

	// Init Nonce
	MyTX.Nonce = 0;

	// Compute Light PoW (LPoW)
	while(1)
	{
		MyHash.clear();

		// Formats all inputs in string format (Subscriber - Publisher - SmartContract - Price - Time - PrevState - DCoin - Nonce)
		snprintf(MyCMD, DATA_FORMATING_LENGTH, "%s%s%s%d%d%s%d%d", MyTX.Subscriber.c_str(), MyTX.Publisher.c_str(), MyTX.SmartContract.c_str(), MyTX.Price, MyTX.Time, MyTX.PrevState.c_str(), MyTX.DCoin, MyTX.Nonce);

		// Compute SHA256
		MyHash = sha256(MyCMD);	

		// Check the Hash of Light PoW according to the Difficulty (Number of first nibble to 0)
		if (Valid_Hash(MyHash) == 0)
			return MyTX.Nonce;
		else
			MyTX.Nonce++;
	}
}



// Check the Light Proof of Work (SHA256 of Transaction)
// MyTX:   Transaction to verify
// Return: ERROR (-1) / OK (0)
int Check_Light_PoW(Transaction MyTX)
{
	char MyCMD[DATA_FORMATING_LENGTH];
	string MyHash;

	MyHash.clear();

	// Formats all inputs in string format (Subscriber - Publisher - SmartContract - Price - Time - PrevState - DCoin - Nonce)
	snprintf(MyCMD, DATA_FORMATING_LENGTH, "%s%s%s%d%d%s%d%d", MyTX.Subscriber.c_str(), MyTX.Publisher.c_str(), MyTX.SmartContract.c_str(), MyTX.Price, MyTX.Time, MyTX.PrevState.c_str(), MyTX.DCoin, MyTX.Nonce);

	// Compute SHA256
	MyHash = sha256(MyCMD);	

	// Check the Hash of Light PoW according to the difficulty (Number of first nibble to 0)
	return Valid_Hash(MyHash);
}



// Check Existing SmartContract (SmartContract Format: NAME_PRICE)
// SmartContract: SmartContract's name to verify
// Price:		  Price of SmartContract 
// Return: 		  ERROR (-1) / OK (0)
int Check_SmartContract(string SmartContract, int Price)
{
	char MyCMD[DATA_FORMATING_LENGTH];
	struct stat buffer;
	snprintf(MyCMD, DATA_FORMATING_LENGTH, "%s%s_%d/%s_%d.bin", SMARTCONTRACT_DIR, SmartContract.c_str(), Price, SmartContract.c_str(), Price);
	return stat(MyCMD, &buffer);
}



// Add New Transaction (Request / Consensus Transaction)
// MyTX: 	New Request / Consensus Transaction to add
// Return:	ERROR (-1) / Type of Transaction (Request / Consensus Transaction)
int Add_Transaction(Transaction MyTX)
{
	char MyCMD[DATA_FORMATING_LENGTH];

	// Check Light PoW && Existing SmartContract
	if ( (Check_Light_PoW(MyTX) == 0) && (Check_SmartContract(MyTX.SmartContract, MyTX.Price) == 0) )
	{
		// New Request Transaction received (Add Request Transaction && Manage old Consensus Transaction of Requester)
		if (MyTX.Subscriber == MyTX.Publisher)
		{
			snprintf(MyCMD, DATA_FORMATING_LENGTH, "UPDATE CONSENSUS_TRANSACTION SET OUTDATE = OUTDATE-1 WHERE PUBLISHER='%s'; DELETE FROM CONSENSUS_TRANSACTION WHERE OUTDATE <= 0; " \
												   "UPDATE REQUEST_TRANSACTION SET OUTDATE = OUTDATE-1 WHERE PUBLISHER='%s'; DELETE FROM REQUEST_TRANSACTION WHERE OUTDATE <= 0; " \
												   "INSERT INTO REQUEST_TRANSACTION (PUBLISHER,SMARTCONTRACT,PRICE,TIME,PREVSTATE,OUTDATE) " \
												   "VALUES ('%s','%s',%d,%d,'%s',%d);", MyTX.Publisher.c_str(), MyTX.Publisher.c_str(), MyTX.Publisher.c_str(), MyTX.SmartContract.c_str(), MyTX.Price, MyTX.Time, MyTX.PrevState.c_str(), TRANSACTION_OUTDATE);
			sqlite3_exec(DB, MyCMD, NULL, NULL, NULL);
			//Display_Request_Transaction();
			return REQUEST_TRANSACTION_TYPE;
		}

		// Add New Consensus Transaction received
		else
		{
			snprintf(MyCMD, DATA_FORMATING_LENGTH, "INSERT INTO CONSENSUS_TRANSACTION (SUBSCRIBER,PUBLISHER,SMARTCONTRACT,PRICE,TIME,PREVSTATE,DCOIN,OUTDATE) "
												   "VALUES ('%s','%s','%s',%d,%d,'%s',%d,%d);", MyTX.Subscriber.c_str(), MyTX.Publisher.c_str(), MyTX.SmartContract.c_str(), MyTX.Price, MyTX.Time, MyTX.PrevState.c_str(), MyTX.DCoin, TRANSACTION_OUTDATE);
			sqlite3_exec(DB, MyCMD, NULL, NULL, NULL);
			//Display_Consensus_Transaction();
			return CONSENSUS_TRANSACTION_TYPE;
		}
	}
	return -1;
}



// Find Request Transaction values from SQLite (Callback)
// MyTX:  	  Address to store the result
// Called by: Generate_Consensus_Transaction(Transaction* Consensus_Transaction)
int SQLite_Find_Request_Transaction_Callback(void *MyTX, int NotUsed, char **Values, char **NotUsed2)
{
	// Check Correct Responses of SQLite
	// PUBLISHER, SMARTCONTRACT, PRICE, TIME, PREVSTATE
	if ( (Values[0] == NULL) || (Values[1] == NULL) || (Values[2] == NULL) || (Values[3] == NULL) || (Values[4] == NULL) )
		return -1;

	// Transaction - Publisher
	(*(Transaction*)MyTX).Publisher = Values[0];

	// Transaction - SmartContract
	(*(Transaction*)MyTX).SmartContract = Values[1];

	// Transaction - Price
	(*(Transaction*)MyTX).Price = atoi(Values[2]);

	// Transaction - Time
	(*(Transaction*)MyTX).Time = atoi(Values[3]);

	// Transaction - PrevState
	(*(Transaction*)MyTX).PrevState = Values[4];
	return 0;
}



// Get Publisher's DCoin value (Callback)
// MyResult:  Address to store the result
// Called by: Generate_Consensus_Transaction(Transaction* Consensus_Transaction)
int SQLite_Get_Publisher_DCoin(void *MyResult, int NotUsed, char **Values, char **NotUsed2)
{
	// Check Correct Responses of SQLite
	// DCOIN
	if (Values[0] == NULL)
		return -1;

	// DCoin
	*((int*)MyResult) = atoi(Values[0]);
	return 0;
}



// Generate Consensus Transaction
// Consensus_Transaction: New Consensus Transaction generated
// Return: 	  	       	  ERROR (-1) / New Consensus Transaction available (0)
int Generate_Consensus_Transaction(Transaction* Consensus_Transaction)
{
	char MyCMD[DATA_FORMATING_LENGTH];

	// Clear Consensus_Transaction
	Consensus_Transaction->Subscriber.clear();
	Consensus_Transaction->Publisher.clear();
	Consensus_Transaction->SmartContract.clear();
	(*Consensus_Transaction).Price = -1;
	(*Consensus_Transaction).Time = -1;
	Consensus_Transaction->PrevState.clear();
	(*Consensus_Transaction).DCoin = -1;
	(*Consensus_Transaction).Nonce = -1;

	// Find Request Transaction
	sqlite3_exec(DB, FIND_REQUEST_TRANSACTION, SQLite_Find_Request_Transaction_Callback, Consensus_Transaction, NULL);

	// Generate Consensus Transaction (Time as data ready flag)
	if (Consensus_Transaction->Time != -1)
	{
		// Remove used Request Transaction
		snprintf(MyCMD, DATA_FORMATING_LENGTH, "DELETE FROM REQUEST_TRANSACTION WHERE PUBLISHER='%s' AND " \
											   "PREVSTATE='%s';", Consensus_Transaction->Publisher.c_str(), Consensus_Transaction->PrevState.c_str());
		sqlite3_exec(DB, MyCMD, NULL, NULL, NULL);

		// Sender of Request Transaction does not send Consensus Transaction (else generate new & wrong Request Transaction)
		if (Consensus_Transaction->Publisher == MyID)
			return -1;

		// Get New DCoin value of Publisher
		snprintf(MyCMD, DATA_FORMATING_LENGTH, "SELECT (COUNTER-(%d*%d))/%d FROM WALLET WHERE PUBLISHER='%s';", Consensus_Transaction->Price, DCOIN_RATE, DCOIN_RATE, Consensus_Transaction->Publisher.c_str());
		
		sqlite3_exec(DB, MyCMD, SQLite_Get_Publisher_DCoin, &Consensus_Transaction->DCoin, NULL);

		// Set Subscriber (ID of this Node)
		Consensus_Transaction->Subscriber = MyID;

		// Compute Light PoW (set Nonce value)
		(*Consensus_Transaction).Nonce = Compute_Light_PoW(*Consensus_Transaction);
		return 0;
	}

	else
		return -1;
}



// Find Majority values from SQLite (Callback)
// MyTX:  	  Address to store the result
// Called by: int Consensus_Process(void)
int SQLite_Find_Majority_Callback(void *MyTX, int NotUsed, char **Values, char **NotUsed2)
{
	// Check Correct Responses of SQLite
	// PUBLISHER, SMARTCONTRACT, PRICE, TIME, PREVSTATE, DCOIN
	if ( (Values[0] == NULL) || (Values[1] == NULL) || (Values[2] == NULL) || (Values[3] == NULL) || (Values[4] == NULL) || (Values[5] == NULL) )
		return -1;

	// Transaction - Publisher
	(*(Transaction*)MyTX).Publisher = Values[0];

	// Transaction - SmartContract
	(*(Transaction*)MyTX).SmartContract = Values[1];

	// Transaction - Price
	(*(Transaction*)MyTX).Price = atoi(Values[2]);

	// Transaction - Time
	(*(Transaction*)MyTX).Time = atoi(Values[3]);

	// Transaction - PrevState
	(*(Transaction*)MyTX).PrevState = Values[4];

	// Transaction - DCoin
	(*(Transaction*)MyTX).DCoin = atoi(Values[5]);
	return 0;
}



// Update Wallet (State & Counter) of Publisher after Majority
// MyTX: All information to compute new state of Publisher
void Update_Wallet(Transaction MyTX)
{
	char MyCMD[DATA_FORMATING_LENGTH];
	string NewState;

	NewState.clear();

	// Formats all inputs in string format (Publisher - SmartContract - Price - Time - PrevState - DCoin)
	snprintf(MyCMD, DATA_FORMATING_LENGTH, "%s%s%d%d%s%d", MyTX.Publisher.c_str(), MyTX.SmartContract.c_str(), MyTX.Price, MyTX.Time, MyTX.PrevState.c_str(), MyTX.DCoin);

	// Compute SHA256
	NewState = sha256(MyCMD);

	// Update Counter & State after Majority
	snprintf(MyCMD, DATA_FORMATING_LENGTH, "DELETE FROM WALLET WHERE PUBLISHER='%s'; " \
										   "INSERT INTO WALLET (PUBLISHER,COUNTER,STATE) " \
										   "VALUES ('%s', %d, '%s');", MyTX.Publisher.c_str(), MyTX.Publisher.c_str(), (MyTX.DCoin*DCOIN_RATE), NewState.c_str());
	
	sqlite3_exec(DB, MyCMD, NULL, NULL, NULL);
}



// Start SmartContract (Format: ../Path/NAME_PRICE.bin + PUBLISHER_PREVSTATE_TIME)
// Params: SmartContract parameters (Name, Price, Publisher, PrevState)
void Start_SmartContract(Transaction Params)
{
	char MyCMD[DATA_FORMATING_LENGTH];
	snprintf(MyCMD, DATA_FORMATING_LENGTH, "%s%s_%d/%s_%d.bin %s_%s_%d &", SMARTCONTRACT_DIR, Params.SmartContract.c_str(), Params.Price, Params.SmartContract.c_str(), Params.Price, Params.Publisher.c_str(), Params.PrevState.c_str(), Params.Time);
	system(MyCMD);
}



// Compute the Consensus Process
// Return: No Majority (-1) / Majority (0)
int Consensus_Process(void)
{
	char MyCMD[DATA_FORMATING_LENGTH];
	Transaction ConsTX = {.Subscriber={0}, .Publisher={0}, .SmartContract={0}, .Price=-1, .Time=-1, .PrevState={0}, .DCoin=-1, .Nonce=-1};

	// Find Majority
	sqlite3_exec(DB, FIND_MAJORITY, SQLite_Find_Majority_Callback, &ConsTX, NULL);

	// Available Majority (Time as data ready flag)
	if (ConsTX.Time != -1)
	{
		// Reward Participants (only first Participants && which have already shared sensor data)
		snprintf(MyCMD, DATA_FORMATING_LENGTH,"UPDATE WALLET SET COUNTER=COUNTER+%d WHERE PUBLISHER IN " \
											   "(SELECT SUBSCRIBER FROM CONSENSUS_TRANSACTION WHERE PUBLISHER='%s' AND " \
											   "SMARTCONTRACT='%s' AND PRICE=%d AND TIME=%d AND PREVSTATE='%s' AND DCOIN=%d LIMIT ROUND((SELECT COUNT(DISTINCT PUBLISHER) FROM WALLET)*%.2f,0));", DCOIN_REWARD, ConsTX.Publisher.c_str(), ConsTX.SmartContract.c_str(), ConsTX.Price, ConsTX.Time, ConsTX.PrevState.c_str(), ConsTX.DCoin, MAJORITY_THRESHOLD);
		sqlite3_exec(DB, MyCMD, NULL, NULL, NULL);

		// Remove used Consensus Transaction & Request Transaction not used yet
		snprintf(MyCMD, DATA_FORMATING_LENGTH, "DELETE FROM CONSENSUS_TRANSACTION WHERE PUBLISHER='%s' AND PREVSTATE='%s'; " \
											   "DELETE FROM REQUEST_TRANSACTION WHERE PUBLISHER='%s' AND PREVSTATE='%s';", ConsTX.Publisher.c_str(), ConsTX.PrevState.c_str(), ConsTX.Publisher.c_str(), ConsTX.PrevState.c_str());
		sqlite3_exec(DB, MyCMD, NULL, NULL, NULL);

		// Update Wallet (State & Counter) of Publisher after Majority
		Update_Wallet(ConsTX);

		// Start SmartContract
		if (MyID == ConsTX.Publisher)
			Start_SmartContract(ConsTX);

		// Display all States
		//Display_Request_Transaction();
		//Display_Consensus_Transaction();
		//Display_Wallets();

		// For Next Majority
		return 0;
	}

	// No Majority
	else
		return -1;
}



// Count the Remaining Requestion Transaction
// Return: Number of Remaing Request Transactions
int Remaining_Request_Transaction(void)
{
	int RemaingRequestTx = 0;

	// Use Publisher DCoin SQLite Callback (integer value)
	sqlite3_exec(DB, REMAINING_REQUEST_TRANSACTION, SQLite_Get_Publisher_DCoin, &RemaingRequestTx, NULL);
	return RemaingRequestTx;
}






/********** DISPLAY PART **********/

// Display Callback
// Called by: Display_Wallets(void)
// Called by: Display_Request_Transaction(void)
// Called by: Display_Consensus_Transaction(void)
int Display_Callback(void *NotUsed, int ColNumber, char **Values, char **Column)
{
	printf("------------------------------\n");

	// Display Column - Value
	for(int i = 0; i < ColNumber; i++)
	{
		// Format Display
		if (strlen(Column[i]) > 6)
			printf("%s:\t%s\n", Column[i], Values[i]);
		else
			printf("%s:\t\t%s\n", Column[i], Values[i]);
	}

	printf("------------------------------\n");

	return 0;
}



// Display Wallets
void Display_Wallets(void)
{

	printf("\n\n---------- WALLETS -----------\n");
	sqlite3_exec(DB, DISPLAY_WALLET, Display_Callback, NULL, NULL);
}



// Display Request Transaction
void Display_Request_Transaction(void)
{

	printf("\n\n---- REQUEST TRANSACTIONS ----\n");
	sqlite3_exec(DB, DISPLAY_REQUEST_TRANSACTION, Display_Callback, NULL, NULL);
}



// Display Consensus Transaction
void Display_Consensus_Transaction(void)
{

	printf("\n\n--- CONSENSUS TRANSACTIONS ---\n");
	sqlite3_exec(DB, DISPLAY_CONSENSUS_TRANSACTION, Display_Callback, NULL, NULL);
}
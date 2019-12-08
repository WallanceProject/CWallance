#include <ctime>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include "SHA256.h"
#include "../../SQLite3/sqlite3.h"

using namespace std;



/************************/
/* SYSTEM CONFIGURATION */
/************************/
#define MAJORITY_THRESHOLD				2.0/3.0		// Must be > 1/2
#define DIFFICULTY						1			// Number of nipples set to '0' at the beginning of Hash
#define DCOIN_RATE						5			// Number of shared Sensor value for 1 DCoin
#define DCOIN_REWARD					2			// Number of Sub-division of DCoin for the participation of Consensus
#define GENESIS_STATE					"0000000000000000000000000000000000000000000000000000000000000000"
#define TRANSACTION_OUTDATE				5			// Number of Request Transaction of Publisher before removing its old Consensus Transaction


/******************/
/* DATA FORMATING */
/******************/
#define DATA_FORMATING_LENGTH			600
#define DATA_DELIMITER					"_"


/********************/
/* MACRO CONVERSION */
/********************/
#define INT_TO_STR(x) 					#x
#define STR(x) 							INT_TO_STR(x)


/**************************************/
/* DATABASE & SMARTCONTRACT DIRECTORY */
/**************************************/
#define DATABASE 						"../Consensus/Consensus.db"
#define SMARTCONTRACT_DIR				"../SmartContract/"				// Format of SmartContract: NAME_PRICE


/***************************/
/* DATABASE TABLE - WALLET */
/***************************/
#define INIT_WALLET						"CREATE TABLE IF NOT EXISTS WALLET (PUBLISHER TEXT, COUNTER INTEGER, STATE TEXT);"
#define DISPLAY_WALLET					"SELECT PUBLISHER, COUNTER/" STR(DCOIN_RATE) " AS DCOIN, STATE FROM WALLET GROUP BY PUBLISHER;"


/****************************************/
/* DATABASE TABLE - REQUEST TRANSACTION */
/****************************************/
#define INIT_REQUEST_TRANSACTION		"CREATE TABLE IF NOT EXISTS REQUEST_TRANSACTION (PUBLISHER TEXT, SMARTCONTRACT TEXT, PRICE INTEGER, TIME INTEGER, PREVSTATE TEXT, OUTDATE INTEGER, UNIQUE(PUBLISHER, PREVSTATE));"
#define DISPLAY_REQUEST_TRANSACTION		"SELECT * FROM REQUEST_TRANSACTION;"
#define FIND_REQUEST_TRANSACTION		"SELECT * FROM REQUEST_TRANSACTION WHERE " \
										"PRICE<=(SELECT COUNTER/" STR(DCOIN_RATE) " FROM WALLET WHERE PUBLISHER=REQUEST_TRANSACTION.PUBLISHER) AND " \
										"PREVSTATE=(SELECT STATE FROM WALLET WHERE PUBLISHER=REQUEST_TRANSACTION.PUBLISHER) LIMIT 1;"


/******************************************/
/* DATABASE TABLE - CONSENSUS TRANSACTION */
/******************************************/
#define INIT_CONSENSUS_TRANSACTION		"CREATE TABLE IF NOT EXISTS CONSENSUS_TRANSACTION (SUBSCRIBER TEXT, PUBLISHER TEXT, SMARTCONTRACT TEXT, PRICE INTEGER, TIME INTEGER, PREVSTATE TEXT, DCOIN INTEGER, OUTDATE INTEGER);"
#define DISPLAY_CONSENSUS_TRANSACTION	"SELECT * FROM CONSENSUS_TRANSACTION;"
#define FIND_MAJORITY					"SELECT PUBLISHER, SMARTCONTRACT, PRICE, TIME, PREVSTATE, DCOIN FROM CONSENSUS_TRANSACTION " \
										"GROUP BY PUBLISHER, SMARTCONTRACT, PRICE, TIME, PREVSTATE, DCOIN HAVING " \
										"PREVSTATE=(SELECT STATE FROM WALLET WHERE PUBLISHER=CONSENSUS_TRANSACTION.PUBLISHER) AND " \
										"ROUND((SELECT COUNT(DISTINCT PUBLISHER) FROM WALLET)*(" STR(MAJORITY_THRESHOLD) "),0) <= COUNT(DISTINCT SUBSCRIBER) LIMIT 1;"






/********** TRANSACTIONS **********/

typedef struct Transaction Transaction;
struct Transaction
{
	string Subscriber;
	string Publisher;
	string SmartContract;
	int Price;
	int Time;
	string PrevState;
	int DCoin;
	int Nonce;
};






/********** CWALLANCE PROTOTYPE DEFINITIONS **********/

// Init CWallance Database
// argv: 	Launcher Command Arguments
// Return:	ERROR (-1) / OK (0)
int Init_CWallance(char **argv);



// Update Wallet Counter of Publisher for sharing a New Sensor value
// Publisher: Publisher ID of New Sensor value
void Update_Wallet_Counter(string Publisher);



// Check the Hash of Light PoW according to the difficulty (Number of first nibble to 0)
// MyString: Hash to check
// Return:	 ERROR (-1) / OK (0)
int Valid_Hash(string MyString);



// Compute the Light Proof of Work (SHA256 of Transaction)
// MyTX:   Transaction to hash
// Return: Correct Nonce value
int Compute_Light_PoW(Transaction MyTX);



// Check the Light Proof of Work (SHA256 of Transaction)
// MyTX:   Transaction to verify
// Return: ERROR (-1) / OK (0)
int Check_Light_PoW(Transaction MyTX);



// Check Existing SmartContract (SmartContract Format: NAME_PRICE)
// SmartContract: SmartContract's name to verify
// Price:		  Price of SmartContract 
// Return: 		  ERROR (-1) / OK (0)
int Check_SmartContract(string SmartContract, int Price);



// Add New Transaction (Request / Consensus Transaction)
// MyTX: New Request / Consensus Transaction to add
void Add_Transaction(Transaction MyTX);



// Find Request Transaction values from SQLite (Callback)
// MyTX:  	  Address to store the result
// Called by: Generate_Consensus_Transaction(Transaction* Consensus_Transaction)
int SQLite_Find_Request_Transaction_Callback(void *MyTX, int NotUsed, char **Values, char **NotUsed2);



// Get Publisher's DCoin value (Callback)
// MyResult:  Address to store the result
// Called by: Generate_Consensus_Transaction(Transaction* Consensus_Transaction)
int SQLite_Get_Publisher_DCoin(void *MyResult, int NotUsed, char **Values, char **NotUsed2);



// Generate Consensus Transaction
// Consensus_Transaction: New Consensus Transaction generated
// Return: 	  	       	  ERROR (-1) / New Consensus Response available (0)
int Generate_Consensus_Transaction(Transaction* Consensus_Transaction);



// Find Majority values from SQLite (Callback)
// MyTX:  	  Address to store the result
// Called by: int Consensus_Process(void)
int SQLite_Find_Majority_Callback(void *MyTX, int NotUsed, char **Values, char **NotUsed2);



// Update Wallet (State & Counter) of Publisher after Majority
// MyTX: All information to compute new state of Publisher
void Update_Wallet(Transaction MyTX);



// Start SmartContract (Format: ../Path/NAME_PRICE.bin + PUBLISHER_PREVSTATE_TIME)
// Params: SmartContract parameters (Name, Price, Publisher, PrevState)
void Start_SmartContract(Transaction Params);



// Compute the Consensus Process
// Return: No Majority (-1) / Majority (0)
int Consensus_Process(void);






/********** DISPLAY PART **********/

// Display Callback
// Called by: Display_Wallets(void)
// Called by: Display_Request_Transaction(void)
// Called by: Display_Consensus_Transaction(void)
int Display_Callback(void *NotUsed, int ColNumber, char **Values, char **Column);



// Display Wallets
void Display_Wallets(void);



// Display Request Transaction
void Display_Request_Transaction(void);



// Display Consensus Transaction
void Display_Consensus_Transaction(void);

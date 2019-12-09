#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../Consensus/Consensus.h"
#include "../Sensor/Sensor.h"

// Set Socket Configuration Type
typedef struct sockaddr_in SOCKADDR_IN;

// Network Configuration
#define MULTICAST_IP				"224.168.0.1"
#define MULTICAST_PORT 				10000
#define MULTICAST_TTL				1
#define MULTICAST_TIMEOUT			2


typedef struct SensorTransaction SensorTransaction;
struct SensorTransaction
{
	string Publisher;
	int SensorData;
};


// Catch SIGINT Signal (CTRL+C)
void CatchSignal(int signo);


// Initialization of RX Socket Network (1 per topic)
// TopicIPAddr: IP Address of the Topic to listen
// Return:      ERROR (-1) / Reception Socket File Descriptor
int NetworkInitRXSocket(void);


// Initialization of Network 
// Return:  ERROR (-1) / Transmission Socket File Descriptor
int NetworkInitTXSocket(void);


// Send Sensor Data to the Network
// TXSocket:    Transmission Socket File Descriptor
// SensorData:  Sensor Data to transmit
void NetworkSend_SensorData(int TXSocket, int SensorData);


// Send Transaction to the Network
// TXSocket:    Transmission Socket File Descriptor
// MyTX:        Transaction to transmit
void NetworkSend_Transaction(int TXSocket, Transaction MyTX);


// Receive Data from Network
// RXSocket:    Reception Socket File Descriptor
// Return:      No Data Available (-1) / Data Available
int NetworkReceive(int RXSocket);

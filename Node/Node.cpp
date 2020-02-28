#include "Node.h"



/********** GLOBAL VARIABLES **********/

// Override of SIGINT signal to stop the execution of the program
static volatile int Running = 1;

// ID of this Node
extern string MyID;

// SQLite3 Access
extern sqlite3* DB;




/********** PROTOTYPE DEFINITIONS **********/


// Catch SIGINT Signal (CTRL+C)
void CatchSignal(int signo)
{
    Running = 0;
}


// Initialization of RX Socket Network (1 per topic)
// TopicIPAddr: IP Address of the Topic to listen
// Return:      ERROR (-1) / Reception Socket File Descriptor
int NetworkInitRXSocket(void)
{
    // RX Socket Configuration
    int RXSocket;
    SOCKADDR_IN RXConfig;

    // MulticastGroup (IP Multicast Address of the Group, Local IP Address of Interface)
    struct ip_mreq MulticastGroup = {inet_addr(MULTICAST_IP), htonl(INADDR_ANY)};

    // Multicast Timeout (Timeout in Second, Timeout in uSecond)
    struct timeval MulticastTimeout = {MULTICAST_TIMEOUT, 0};

    // Create Reception Socket
    RXSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (RXSocket < 0)
        return -1;

    // Configuration of RXSocket
    memset(&RXConfig, 0, sizeof(RXConfig));
    RXConfig.sin_family = AF_INET;
    RXConfig.sin_port = htons(MULTICAST_PORT);
    RXConfig.sin_addr.s_addr = htonl(INADDR_ANY);

    // Set Multicast Receive Timeout
    if (setsockopt(RXSocket, SOL_SOCKET, SO_RCVTIMEO, (char*) &MulticastTimeout, sizeof(MulticastTimeout)) < 0)
        return -1;

    // Join the MulticastGroup
    if (setsockopt(RXSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &MulticastGroup, sizeof(MulticastGroup)) < 0)
        return -1;

    // Bind RXSocket
    if (bind(RXSocket, (struct sockaddr*) &RXConfig, sizeof(RXConfig)) < 0)
        return -1;

    return RXSocket;   
}


// Initialization of Network 
// Return:  ERROR (-1) / Transmission Socket File Descriptor
int NetworkInitTXSocket(void)
{
    // TX Socket Configuration
    int TXSocket;

    // Multicast TTL Configuration
    u_char TTL = MULTICAST_TTL;

    // Multicast Timeout (Timeout in Second, Timeout in USecond)
    struct timeval MulticastTimeout = {MULTICAST_TIMEOUT, 0};
    
    // Create Transmission Socket
    TXSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (TXSocket < 0)
        return -1;

    // Configure Multicast TTL
    if (setsockopt(TXSocket, IPPROTO_IP, IP_MULTICAST_TTL, &TTL, sizeof(TTL)) < 0)
        return -1;

    // Set Multicast Transmit Timeout
    if (setsockopt(TXSocket, SOL_SOCKET, SO_SNDTIMEO, (char*) &MulticastTimeout, sizeof(MulticastTimeout)) < 0)
        return -1;
    return TXSocket;
}


// Send Sensor Data to the Network
// TXSocket:    Transmission Socket File Descriptor
// SensorData:  Sensor Data to transmit
void NetworkSend_SensorData(int TXSocket, int SensorData)
{
    // TXSocket Configuration
    SOCKADDR_IN TXConfig;
    memset(&TXConfig, 0, sizeof(TXConfig));
    TXConfig.sin_family = AF_INET;
    TXConfig.sin_port = htons(MULTICAST_PORT);
    TXConfig.sin_addr.s_addr = inet_addr(MULTICAST_IP);

    // Format Sensor Transaction
    char MySerialTX[DATA_FORMATING_LENGTH] = {0};
    snprintf(MySerialTX, DATA_FORMATING_LENGTH, "%c%s%s%s%d", 'S', DATA_DELIMITER, MyID.c_str(), DATA_DELIMITER, SensorData);

    // Send Data to the Network
    sendto(TXSocket, MySerialTX, strlen(MySerialTX), 0, (struct sockaddr*) &TXConfig, sizeof(TXConfig));
}


// Send Transaction to the Network
// TXSocket:    Transmission Socket File Descriptor
// MyTX:        Transaction to transmit
void NetworkSend_Transaction(int TXSocket, Transaction MyTX)
{
    // TXSocket Configuration
    SOCKADDR_IN TXConfig;
    memset(&TXConfig, 0, sizeof(TXConfig));
    TXConfig.sin_family = AF_INET;
    TXConfig.sin_port = htons(MULTICAST_PORT);
    TXConfig.sin_addr.s_addr = inet_addr(MULTICAST_IP);

    // Serialize Transaction
    char MySerialTX[DATA_FORMATING_LENGTH];
    snprintf(MySerialTX, DATA_FORMATING_LENGTH, "%s%s%s%s%s%s%d%s%d%s%s%s%d%s%d", \
            MyTX.Subscriber.c_str(), DATA_DELIMITER, MyTX.Publisher.c_str(), DATA_DELIMITER, MyTX.SmartContract.c_str(), DATA_DELIMITER, MyTX.Price, DATA_DELIMITER, MyTX.Time, DATA_DELIMITER, MyTX.PrevState.c_str(), DATA_DELIMITER, MyTX.DCoin, DATA_DELIMITER, MyTX.Nonce);

    // Send Transaction to the Network
    sendto(TXSocket, MySerialTX, strlen(MySerialTX), 0, (struct sockaddr*) &TXConfig, sizeof(TXConfig));
}


// Receive Data from Network
// RXSocket:    Reception Socket File Descriptor
// Return:      No Data Available (-1) / Data Available
int NetworkReceive(int RXSocket)
{
    SOCKADDR_IN ClientConfig;
    socklen_t Length = sizeof(ClientConfig);

    // Parse MySerialTX
    char MySerialTX[DATA_FORMATING_LENGTH] = {0};
    char *Parser = MySerialTX;

    Transaction MyTX;

    if (recvfrom(RXSocket, &MySerialTX, sizeof(MySerialTX), 0, (struct sockaddr*) &ClientConfig, &Length) != -1)
    {
        // Sensor Topic
        if ((MySerialTX[0] == 'S') && (MySerialTX[1] == *DATA_DELIMITER))
        {
            // Find Publisher
            Parser = MySerialTX+2;
            while( (*Parser != 0) && (*Parser != *DATA_DELIMITER) )
                Parser++;

            // Set End of First Data Part
            if (*Parser == *DATA_DELIMITER)
            {
                *Parser = 0;
                Update_Wallet_Counter(MySerialTX+2);
                return 0;
            }
            else
                return -1;
        }

        else
        {
            Parser = strtok(MySerialTX, DATA_DELIMITER);
            MyTX.Subscriber = Parser;

            Parser = strtok(NULL, DATA_DELIMITER);
            MyTX.Publisher = Parser;
                
            Parser = strtok(NULL, DATA_DELIMITER);
            MyTX.SmartContract = Parser;

            Parser = strtok(NULL, DATA_DELIMITER);
            MyTX.Price = atoi(Parser);

            Parser = strtok(NULL, DATA_DELIMITER);
            MyTX.Time = atoi(Parser);

            Parser = strtok(NULL, DATA_DELIMITER);
            MyTX.PrevState = Parser;

            Parser = strtok(NULL, DATA_DELIMITER);
            MyTX.DCoin = atoi(Parser);

            Parser = strtok(NULL, DATA_DELIMITER);
            MyTX.Nonce = atoi(Parser);

            // Add Request/Consensus Transaction
            // Return ERROR (-1) / Type of Transaction (Request Transaction / Consensus Transaction)
            return Add_Transaction(MyTX);
        }
    }
    else
        return -1;
}



/********** MAIN PART **********/

int main (int argc, char **argv)
{   
    // Reception of Sensor value
    int MySensor;

    // Local Transaction
    int MyRX = -1;
    int AnyRequest = 0;
    Transaction MyTX;

    // Network
    int RXSocket;
    int TXSocket;

    // Reading Pipes: Sensor -> DDS
    mkfifo(PIPE_SENSOR_TO_NODE, 0777);
    int Pipe_sensor_to_node = open(PIPE_SENSOR_TO_NODE, O_RDONLY | O_NONBLOCK);
    if (Pipe_sensor_to_node == -1)
    {
        printf("ERROR OPENNING PIPE SENSOR TO NODE : %s\n", strerror(errno));
        return -1;
    }


    // Init Consensus Database & Tables
    if (Init_CWallance(argv) == -1)
        return -1;


    // Init Network
    RXSocket = NetworkInitRXSocket();
    TXSocket = NetworkInitTXSocket();

    if ( (RXSocket == -1) || (TXSocket == -1) )
        return -1;

    // Wait for other Participants
    sleep(5);

    // Launch Sensor process
    system("cd ../Sensor && ./Sensor.bin 8 &");

    signal(SIGINT, CatchSignal);
    while (Running)
    {

        // Read Pipe: Data from Sensor to send on network
        if (read(Pipe_sensor_to_node, &MySensor, sizeof(int)))
            NetworkSend_SensorData(TXSocket, MySensor);

        // Listen Network
        MyRX = NetworkReceive(RXSocket);

        // New Request Transaction available
        if (MyRX == REQUEST_TRANSACTION_TYPE)
            AnyRequest++;

        // Try to Generate Consensus Transaction
        if (AnyRequest > 0)
        {
            if(Generate_Consensus_Transaction(&MyTX) != -1)
            {
                // Send Request Transaction
                NetworkSend_Transaction(TXSocket, MyTX);
                AnyRequest--;
            }
        }

        // Execute Consensus
        if (MyRX == CONSENSUS_TRANSACTION_TYPE)
        {
            while(Consensus_Process() != -1);
            AnyRequest = Remaining_Request_Transaction();
        }
    }

    // Close Pipe, Sockets & Database
    close(Pipe_sensor_to_node);
    close(RXSocket);
    close(TXSocket);
    sqlite3_close(DB);

    printf("By Node\n");
    return 0;
}
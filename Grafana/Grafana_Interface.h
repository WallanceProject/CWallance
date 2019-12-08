#include "../Node/Node.h"

// Communication between Grafana Web Interface & Grafana Interface bin
#define PIPE_WEB_INTERFACE_TO_GRAFANA   "../Pipes/Pipe_Web_to_Grafana"


// Send Request Transaction to the Network
// TXSocket:    Transmission Socket File Descriptor
// MyCMD:       Request Transaction to transmit (string format)
void NetworkSend_RequestTransaction(int TXSocket, char* MyCMD);
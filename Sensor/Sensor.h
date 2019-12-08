#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


// Pipe: Sensor -> Node
#define PIPE_SENSOR_TO_NODE 	"../Pipes/Pipe_sensor_to_node"






/********** PROTOTYPES **********/

void catch_function(int signo);
ssize_t Write_Pipe(const char* Pipe_Name, const void *mydata, size_t count);
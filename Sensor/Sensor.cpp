#include "Sensor.h"


/********** GLOBAL VARIABLES **********/

// Override of SIGINT signal to stop the execution of the program
int interrupt_detected = 0;






/********** PROTOTYPE DEFINITIONS **********/

void catch_function(int signo) {
    printf("\nInterrupt SIGINT detected. Terminating process...\n");
    interrupt_detected = 1;
}



ssize_t Write_Pipe(const char* Pipe_Name, const void *mydata, size_t count)
{
    ssize_t Result = -1;

    int MyPipe = open(Pipe_Name, O_WRONLY | O_NONBLOCK);
    if (MyPipe != -1)
    {
        // Write into Pipe
        Result = write(MyPipe, mydata, count);
        close(MyPipe);
    }
    else
        printf("ERROR OPENNING %s : %s\n", Pipe_Name, strerror(errno));

    return Result;
}






/********** MAIN PART **********/

int main (int argc, char **argv)
{
	// Init Random Generator
	srand(time(NULL));

	int Value = 0;

	signal(SIGINT, catch_function);
	while (interrupt_detected != 1)
	{
		// Wait beetween 1 to 20s
		sleep(rand() % 20 + 1);

		Value += 1;

		Write_Pipe(PIPE_SENSOR_TO_NODE, &Value, sizeof(int));
	}

	printf("By Sensor\n");
	return 0;
}
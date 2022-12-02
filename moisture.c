#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include <pthread.h>
#include<string.h>
#include<sys/utsname.h>
#include<stdlib.h>

#define LOOP 5
#define MAX_LENGTH 40

/*Define the Moisture Sensor Pins here*/
#define ANALOG_IN 1 /*Analog input pin

/*Define the L293D IC Pins here*/
#define PWM 3
#define INPUT1 48
#define INPUT2 51

#define MOIST_THRESHOLD 250

// structure of thread with sensors and state
struct auto_irrigation {
  int soil_sensor;
  int pwm;
  int motor_ip1;
  int motor_ip2;
  int motorState;
  int moist_threshold;
  //pthread_mutex_t mutex;
};
//initialize the mutex mutex1
//pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

//declare two threads, each for each traffic signal
struct auto_irrigation irrigation;

//function to turn-on the GPIO pins
void turnOnGPIO(int gpio){
	printf("turning on %d \n",gpio);
    char str[80],str2[80];
	sprintf(str, "/sys/class/gpio/gpio%d/direction", gpio);
	sprintf(str2, "/sys/class/gpio/gpio%d/value", gpio);
	int gpioIOFile = open(str,O_RDWR);
	write(gpioIOFile,"out",3);
	gpioIOFile = open(str2,O_WRONLY);
	write(gpioIOFile,"1",1);
}

//function to turn-off the GPIO pins
void turnOffGPIO(int gpioPin){
    char str[80],str2[80];
    sprintf(str, "/sys/class/gpio/gpio%d/direction", gpioPin);
    sprintf(str2, "/sys/class/gpio/gpio%d/value", gpioPin);
	int gpioIOFile = open(str,O_RDWR);
    write(gpioIOFile,"out",3);
    close(gpioIOFile);
    gpioIOFile = open(str2,O_WRONLY);
    write(gpioIOFile,"0",1);
    close(gpioIOFile);
}

//function to enable the analog pin
void turnOnAnalogPin(int analogPin){
    char buttonGPIO[80];
    sprintf(buttonGPIO, "/sys/bus/iio/devices/iio:device0/scan_elements/in_voltage%d_en", analogPin);
    FILE * f = fopen(buttonGPIO, "w");
    fprintf(f,"%d",1);
    fclose(f);
}

//function to disable the analog pin
void turnOffAnalogPin(int analogPin){
    char buttonAnalog[80];
    sprintf(buttonAnalog, "/sys/bus/iio/devices/iio:device0/scan_elements/in_voltage%d_en", analogPin);
    FILE * f = fopen(buttonAnalog, "w");
    fprintf(f,"%d",0);
    fclose(f);
}

//funtion to read the sensor readings from the moisture sensor(Analog input)
int getMoistureSensorReadings(int val, int pin){
    char buttonAnalog[80];
    sprintf(buttonAnalog, "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw", pin);
    FILE * file = fopen(buttonAnalog, "r");
    (void) fscanf(file, "%d", &val);
    fclose(file);
    return val;
}

//invokes the pub.sh script by passing moisture sensor data as argument
void sendDataToServer(int val){
	char moist[6];
	char bar[100]="./pub.sh ";
	sprintf(moist,"%d",val);
    strcat(bar, moist);
    system(bar);
}

//main program to execute traffic light sequence
int main (void)
{   printf("**********************************");
    //define the pins for each sensor input and output
    irrigation.soil_sensor = ANALOG_IN;
    irrigation.pwm = PWM;
    irrigation.motor_ip1 = INPUT1;
    irrigation.motor_ip2 = INPUT2;
    
	//set the motor state.
    irrigation.motorState = 0;
    
	//set the moisture sensor threshold
    irrigation.moist_threshold = MOIST_THRESHOLD;
    
    //turn off all the Sensors
    turnOffAnalogPin(irrigation.soil_sensor);
    turnOffGPIO(irrigation.pwm);
    turnOffGPIO(irrigation.motor_ip1);
    turnOffGPIO(irrigation.motor_ip2);
    
    //turning soil sensor on
    turnOnAnalogPin(irrigation.soil_sensor);
    
    //moisture level
    int val=0;
    
	//Loop which runs continuously to monitor moisture level
    while (1)
    {
		//read moisture level
        val = getMoistureSensorReadings(val, irrigation.soil_sensor);
        printf("Moisture Level : %d \n",val);
		
		//turn on the pump if the moisture level is lower than threshold and motor state is off. Else, turn off all the motor pins        
        if(val<irrigation.moist_threshold && irrigation.motorState==0){
          turnOnGPIO(irrigation.pwm);
          turnOffGPIO(irrigation.motor_ip2);
          turnOnGPIO(irrigation.motor_ip1);
          
          sleep(5);
          
          irrigation.motorState = 0;
          
        }
        else{
          irrigation.motorState = 0;
          turnOffGPIO(irrigation.pwm);
          turnOffGPIO(irrigation.motor_ip1);
          turnOffGPIO(irrigation.motor_ip2);
        }
        
        
        //IOT- send the stream of moisture level data to the server
        sendDataToServer(val);
        
        sleep(5);
    }
    
    //pthread_t pushThread;
    //pthread_create(&pushThread, NULL, pushButtonListener, &args);
    //pthread_join(pushThread, NULL);

    return 0;

} // end of main

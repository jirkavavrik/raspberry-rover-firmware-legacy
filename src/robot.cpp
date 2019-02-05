#include <wiringPi.h>
#include <wiringSerial.h>
#include <softPwm.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


#define FW true //dopředu
#define BW false //dozadu

void motorLeft(unsigned char, bool);
void motorRight(unsigned char, bool);

void goForward(unsigned char);
void turnLeft(unsigned char);
void turnRight(unsigned char);
void goBack(unsigned char);
void STOP();

void fw(unsigned char, unsigned char); //go forwards with possible turning
void bw(unsigned char, unsigned char); //go backwards with possible turning

unsigned char speednow = 50;

void STOP() {
 
 motorLeft(0, FW);
 motorRight(0, FW);
  
}

void turnRight(unsigned char pwm) {
  
 motorRight(0, FW);
motorLeft(pwm, FW); 
  
}

void turnLeft(unsigned char pwm) {
  
 motorLeft(0, FW);
 motorRight(pwm, FW); 
  
}

void goForward(unsigned char pwm) {
 
 motorLeft(pwm, FW);
 motorRight(pwm, FW);
  
}

void motorRight(unsigned char value, bool dir = FW) {
    if(dir == FW)
      digitalWrite(23, LOW);
    else
      digitalWrite(23, HIGH);
    
    if(dir == FW) {
      if(value == 0)
		softPwmWrite(24, 0);
      else      
        softPwmWrite(24, value*0.94); 
    }
    else
      softPwmWrite(24, (100 - value)*0.94); //reverse value so 0 stays min and 100 stays max
}
void motorLeft(unsigned char value, bool dir = FW) {
    if(dir == FW)
      digitalWrite(21, LOW);
    else
      digitalWrite(21, HIGH);
    
    if(dir == FW) {
      if(value == 0)
        softPwmWrite(22, 0);
      else      
        softPwmWrite(22, value);
    }
    else
      softPwmWrite(22, 100 - value); //reverse value so 0 stays min and 100 stays max
}

void goBack (unsigned char pwm) {
  digitalWrite(21, HIGH);//left
  softPwmWrite(22, 100-pwm);
  digitalWrite(23, HIGH);//right
  softPwmWrite(24, 100-pwm*0.94);
}

void fw(unsigned char l, unsigned char r) {
 
 motorLeft(l, FW);
 motorRight(r, FW);
  
}

void bw(unsigned char l, unsigned char r){
  
  motorLeft(l, BW);
  motorRight(r, BW);
  
}

int main(int argc, char *argv[]) {
	wiringPiSetup();
	
	//H-bridge pins
	pinMode(21, OUTPUT);
	pinMode(22, OUTPUT);
	pinMode(23, OUTPUT);
	pinMode(24, OUTPUT);
	softPwmCreate (22, 0, 100);
	softPwmCreate (24, 0, 100);
	
	//shutdown button
	pinMode(29, INPUT);
	pullUpDnControl (29, PUD_UP);
	int shutdownButton;
	
	int server_socket, client_socket, portno;
	int disconnect = 0;
	char message;
	struct sockaddr_in server_address;

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
     }
     
	//parse port number
	portno = atoi(argv[1]);
	
	//create socket, if unsuccessful, print error message
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) 
		fprintf(stderr, "ERROR opening socket\n");
	
	//assign server address
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(portno);
	
	if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) 
		fprintf(stderr, "ERROR on binding");

	listen(server_socket,5);
	while(true) {//network loop
		disconnect = 0;
		client_socket = accept(server_socket, NULL, NULL);
		if (client_socket < 0) 
			fprintf(stderr, "ERROR on accept");
	
		while(true) {//control loop
			shutdownButton = digitalRead(29);
			if(shutdownButton == LOW) {
				system("sudo shutdown -h now &");
				break;
			}		
			if(recv(client_socket, &message, sizeof(message),MSG_DONTWAIT) != -1) {
				printf("message: %c\n", message);
			
				switch(message){
					case 'X': system("python3 /home/pi/programy/webcamera.py &"); break;
					case 'x': system("pkill -15 -f ""webcamera.py"""); break;
					
					case '0': disconnect = 1; break;
					
					case 'F': 
						goForward(speednow); break;
      
					case 'G':
						fw(speednow/2, speednow); break;
		
					case 'I':
						fw(speednow, speednow/2); break;
		
					case 'S':
						STOP(); break;
      
					case 'L': 
						turnLeft(speednow);  break;
      
					case 'R': 
						turnRight(speednow);  break;
      
					case 'B': 
						goBack(speednow); break;
      
					case 'H':
						bw(speednow/2, speednow); break;
				
					case 'J':
						bw(speednow, speednow/2);	break;
						
					case '+':
						if(speednow <=95)
							speednow += 5;
						break;
					case'-':
						if(speednow >=5)
							speednow -= 5;
						break;
      
				}//end of switch
				if(disconnect == 1) {
					printf("disconnecting now...\n");break;
				}			
			}//end of if recv
			char speednow_string[3];
			int n = sprintf(speednow_string, "%i", speednow);
			send(client_socket, speednow_string, n, 0);
			delay(100);	
		}//end of control loop
		
		close(client_socket);
		printf("client socket closed.\n");
		
	}
	return 0;
}
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#define FILENAME "test"
#define PACKET_SIZE 128

FILE *f;
int clientSocket;
char buffer[1024];
struct sockaddr_in serverAddr;
socklen_t addressSize;
struct stat fileStatus;
int packetNumber;
int damage;
struct hostent *hp;
char serverIP;
struct in_addr ip;

/*---- Connects to the Server ----*/
int serverConnect() {
   printf("Requesting Server authentication...");
   
   bzero((char *) &serverAddr, addressSize);
   
   clientSocket = socket(PF_INET, SOCK_DGRAM, 0); // Create the socket with UDP
   
   // Configure Server address settings
   serverAddr.sin_family = AF_INET; // Set address family to internet
   serverAddr.sin_port = htons(10041); // Set port number using htons for correct byte order
   serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Set IP address to localhost
   
   // Get desired IP from user
   printf("\nPlease enter the desired IP to connect to: ");
   scanf("%s", &serverIP);
   
   const char *ipstr = (const char *)&serverIP;
      
   if(!(inet_aton(ipstr, &ip))) {
      errx(1, "Can't parse IP address %s", ipstr);
   }
   
   if((hp = gethostbyaddr((const void *)&ip, sizeof(ip), AF_INET)) == NULL) {
      errx(1, "No name associated with %s", ipstr);
   }

	// Connect the socket to the server
   addressSize = sizeof(serverAddr);
   bcopy(hp->h_addr, &(serverAddr.sin_addr), hp->h_length);

   // Prints after connected to the server
   printf("\nAuthenticated by Server!\n");
   return 0;
}

/*---- Creates and sends a packet with the provided file data ----*/
int createPacket() {
   // Variable declarations / initializations
   int lossAmt;
   packetNumber = 1;
   char msg[PACKET_SIZE];
   char c;
   char seqNum = '0';
   int numCharsRead = 0;
   int bufferPosition = 11;
   
   // open file for reading purposes
   f = fopen(FILENAME, "r"); 

	// Create checksum
   msg[1] = '3';
   msg[2] = '3';
   msg[3] = '3';
   msg[4] = '3';
   msg[5] = '3';
   msg[6] = '3';
   msg[7] = '3';
   msg[8] = '3';
   msg[9] = '0'; // Remainder, if divided by 3
   msg[10] = 'A'; // For ACK, or N for NACK
   
   // Fill packet with data and send
   while((c = fgetc(f)) != EOF) {
      if(numCharsRead == 0) {
         printf("Writing data to packet %d...\n", packetNumber);
      }
      
      msg[0] = seqNum;
      numCharsRead++;
      msg[bufferPosition] = c;
      bufferPosition++;
   
      // 100 bytes reached, packet is ready to be sent
      if(numCharsRead == 116) {
         printf("Finished filling packet %d.\n", packetNumber);
         printf("Message reads:\n%s(%lu bytes).", msg, sizeof(msg));
         printf("\nNow sending through Gremlin...\n");
         int val = gremlin(msg);
         if(val == 1) {
            printf("TimeOut! The packet has been resent.\n");
            sendto(clientSocket, msg, PACKET_SIZE, 0, (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
         } 
         else {
            sendto(clientSocket, msg, PACKET_SIZE, 0, (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
         }
         printf("Packet %d sent!\n\n", packetNumber);
         
         numCharsRead = 0;
         bufferPosition = 11;
                       
         recvfrom(clientSocket, msg, PACKET_SIZE, 0, (struct sockaddr *) &serverAddr, &addressSize);
         
         // Sequence number not correct, packet is corrupted
         while(msg[10] == 'N') {
            packetNumber++;
            printf("Corrupted packet caught!\n");
            msg[0] = seqNum;
            msg[1] = '3';
            msg[2] = '3';
            msg[3] = '3';
            msg[4] = '3';
            msg[5] = '3';
            msg[6] = '3';
            msg[7] = '3';
            msg[8] = '3';
            msg[9] = '0'; // Remainder, if divided by 3
            msg[10] = 'A'; // For ACK, or N for NACK
            
            // Reset numCharsRead and bufferPosition
            printf("Finished filling packet %d.\n", packetNumber);
            printf("Message reads:\n%s(%lu bytes).\n", msg, sizeof(msg));
            printf("Now sending through Gremlin...\n");
            gremlin(msg);
            sendto(clientSocket, msg, PACKET_SIZE, 0, (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
            printf("Packet %d sent!\n\n\n", packetNumber);
            
            // Receives packet and increases number of packets
            recvfrom(clientSocket, msg, PACKET_SIZE, 0, (struct sockaddr *) &serverAddr, &addressSize);
         }
         packetNumber++;
      }
      if(seqNum == '0') {
         seqNum = '1';
      } else {
         seqNum = '0';
      }
   }
   
   // If the packet is not completely filled
   if((bufferPosition != 11) || (bufferPosition != 127)) {
      int i;
      int nullCharsCount = 0;
      
      // get number of null characters left in packet
      for(i = PACKET_SIZE - 1; i > (bufferPosition - 1); i--) {
         nullCharsCount++;
         msg[i] = '\0';
      }
   
      // if amount greater than 0, fill packet with null characters until size limit is reached
      if(nullCharsCount) {
         msg[127] = '*';
         printf("There is not enough data to completely fill the packet!\n");
         printf("Inserted %d null characters to pad the packet...\n", nullCharsCount);
         printf("The last packet reads:\n");
         sendto(clientSocket, msg, PACKET_SIZE, 0, (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
         printf(msg);
         printf("\nPacket %d sent!\n\n\n", packetNumber);
      }
   }

   fclose(f); // close file
   return 0;
}

 /*---- Reads a file (file name defined above as FILENAME) ----*/
int readFile() {
  // Handles errors finding/opening the file
   if(stat(FILENAME, &fileStatus) != 0) {
      perror("ERROR: Could not load file -- file may not exist");
   }
   
   // Prints file name and file size to screen
   printf("\nFile name: %s\n", FILENAME);
   printf("File size: %d\n\n\n", ((intmax_t)fileStatus.st_size));
   f = fopen(FILENAME, "r"); // opens file
   char *buffer = (char *)malloc(fileStatus.st_size + 1);

   // Handles errors reading the file
   if(!(fread(buffer, fileStatus.st_size, 1, f))) {
      perror("ERROR: Could not read file");
   }

   buffer[fileStatus.st_size +1] = '\0';
   createPacket(); // calls the createPacket() function to create a packet

   free(buffer); // free the buffer space
   return 0;
}

/*---- Gremlin function implementation ----*/
int gremlin(char data[]) {
   // Variable initializations
   int timeout = rand() % 10;
   int amount = rand() % 10;
   int val = rand() % 10;
   
   if(damage > val) {
      if(amount < 7) {
         data[1] = '5';
         printf("One bit has been changed...\n");
      } 
      else if(7 <= amount < 9) {
         data[1] = '5';
         data[2] = '5';
         printf("Two bits have been changed...\n");
      }
      else {
         data[1] = '5';
         data[2] = '5';
         data[3] = '5';
         printf("Three bits have been changed...\n");
      }
   }
   
   if(timeout <= 8) {
      return 1;
   }
   return 0;
}

/*---- Main function ----*/
int main() {
   serverConnect();
   
   printf("The packet loss value is .2\n");
   printf("Please enter the amount of damage (0 - 10): ");
   scanf("%d", &damage);
   
   readFile(); // Read file
   

   return 0;
}

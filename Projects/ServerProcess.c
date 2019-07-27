#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#define PACKET_SIZE 128

FILE *of;
int serverSocket, newSocket;
char buffer[1024];
struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
struct sockaddr_storage serverStorage;
socklen_t addressSize;
socklen_t cLength = sizeof(clientAddr);

/*---- Connects to the Client ----*/
int clientConnect() {   

   if((serverSocket = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
      errx(1, "Error creating the socket!!");
      exit(EXIT_FAILURE);
   }
   
   // Configure Server address settings
   serverAddr.sin_family = AF_INET; // Set address family to internet
   serverAddr.sin_port = htons(10041); // Set port number using htons for correct byte order
   serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically filled with current host's IP address
   
   // Bind the address to the socket
   if((bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) == -1) {
      errx(1, "Error binding the address to the socket!!!");
      exit(EXIT_FAILURE);
   }

   
	// Creates a new socket for the incoming connection and connect them
   addressSize = sizeof(serverStorage);
   
   return 0;
}

/*---- Calculates the checksum ----*/
int calculateChecksum(char packet[]) {
   // Variable declarations
   int sum = 0;
   int i;
   
   // Calculates the sum
   for(i = 1; i < 10; i++) {
      sum += packet[i];
   }
   
   // Returns true (1) if checksum contains all 3's; otherwise, returns false (0)
   if(sum % 3 == 0) {
      return 1;
   } 
   else {
      return 0;
   }
}

/*---- Receives a packet with the data from the file ----*/
int receiveMessage() {
   // Variable declarations
   char c;
   int packetNumber = 1;
   
   of = fopen("write_file", "w");
   
   // Checks if file opened properly
   if(of == NULL) {
      printf("Can't open output file");
      exit(EXIT_FAILURE);
   }
   
   int n = 240 * 1024;
   setsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));
   
   while(c != '*') {
      int passed;
      if (recvfrom(serverSocket, buffer, PACKET_SIZE, 0, (struct sockaddr *) &clientAddr, &cLength) == -1) {
         errx(1, "Error receiving message!!!");
         exit(EXIT_FAILURE);
      }
      
      printf("\n\nReceived packet %d...\n", packetNumber);
      packetNumber++;
      printf("The packet is being checked for errors...\n");
      passed = calculateChecksum(buffer);
      if(passed == 1) {
         printf("The packet was valid!\n");
         printf("Message reads:\n%s(%lu bytes).", buffer, sizeof(buffer));
         int i = 0;
         fflush(of);
         for(i = 11; i < PACKET_SIZE; i++) {
            putc(buffer[i], of);
            if(c == '\0') {
               c = buffer[i + 1];
            }
            c = buffer[i];
         }
         if((sendto(serverSocket, buffer, PACKET_SIZE, 0, (struct sockaddr *) &clientAddr, cLength)) == -1) {
            errx(1, "Error sending message!!!");
            exit(EXIT_FAILURE);
         }
      } 
      else { // Packet corrupted
         printf("The packet was corrupted!\n");
         printf("\nMessage reads:\n%s(%lu bytes).", buffer, sizeof(buffer));
         buffer[10] = 'N';
         if((sendto(serverSocket, buffer, PACKET_SIZE, 0, (struct sockaddr *) &clientAddr, cLength)) == -1) {
            errx(1, "Error sending message!!!");
            exit(EXIT_FAILURE);
         }
      }
      // sleep(1);
   }
   
   fclose(of); // Close file
   return 0;
}

/*---- Main function ----*/
int main() {
   clientConnect();
   receiveMessage();
   
   // Variable declarations
   char c;
   int pos = 0;
   
   of = fopen("write_file", "r");
   
   // Checks if file opened properly
   if(of == NULL) {
      printf("Can't open output file");
      exit(1);
   }
   
   fseek(of, 0L, SEEK_END);
   fseek(of, 0L, SEEK_SET);
   printf("\n\n\nData from file:\n");
   
   char oMsg[99999];
   int i = 0;
   
   while ((c = fgetc(of)) != '*') {
      if(c == '\0') {
         c = ' ';
      }
      oMsg[i] = c;
      i++;
   }
 
   
   printf("\nThe last packet contains: \n%s\n\n", oMsg); // formatting
   fclose(of); // closes file

   return 0;
}

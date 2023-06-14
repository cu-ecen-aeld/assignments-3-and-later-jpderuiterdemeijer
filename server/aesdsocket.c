#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define REC_CHUNK_SIZE (20)

int   packetBufferOffset = 0;
int   localSocket        = -1;
int   remoteSocket       = -1;
int   logFile            = -1;
char* pPacketBuffer      = NULL;

void prepareClose() {

   packetBufferOffset = 0;
   if (pPacketBuffer != NULL) {
      free(pPacketBuffer);
      pPacketBuffer = NULL;
   }
   if (remoteSocket != -1) {
      shutdown(remoteSocket, SHUT_RDWR);
      close(remoteSocket);
      remoteSocket = -1;
   }
}

void prepareExit() {
   prepareClose();

   if (localSocket != -1) {
      close(localSocket);
   }
   if (logFile != -1) {
      close(logFile);
      logFile = -1;
   }
   remove("/var/tmp/aesdsocketdata");

   closelog();
}

void signalsCallback(int signal) {

   if (signal == SIGINT) {
      syslog(LOG_DEBUG, "SIGINT caught, exiting\n");
      prepareExit();
      exit(0);
   }
   if (signal == SIGTERM) {
      syslog(LOG_DEBUG, "SIGTERM caught, exiting\n");
      prepareExit();
      exit(0);
   }
}

int main(int argc, char* argv[]) {

   openlog("Assignement5", 0, LOG_USER);

   struct sigaction sigActions;
   sigActions.sa_handler = signalsCallback;
   sigemptyset(&sigActions.sa_mask);
   sigActions.sa_flags = 0;
   sigaction(SIGINT, &sigActions, 0);
   sigaction(SIGTERM, &sigActions, 0);

   if (argc > 2) {
      syslog(LOG_ERR, "Only one argument allowed\n");
      closelog();
      exit(1);
   }

   if (argc == 2) {

      if (strcmp("-d", argv[1]) == 0) {
         syslog(LOG_DEBUG, "Running as daemon\n");

         pid_t pid = 0;
         pid_t sid = 0;

         pid = fork();
         if (pid < 0) {
            syslog(LOG_ERR, "fork() failed\n");
            closelog();
            exit(1);
         }
         if (pid > 0) {
            // We are parent, nothing to do
            closelog();
            exit(0);
         }

         // This is the child

         // Set file permissions to default (0666)
         umask(0);

         // Create new session
         sid = setsid();

         if (sid < 0) {
            syslog(LOG_ERR, "setsid() failed\n");
            closelog();
            exit(1);
         }

         // close all std file descriptors
         close(0);
         close(1);
         close(2);

         // Open stdin as /dev/null;
         int std_handle = open("/dev/null", O_RDWR);
         // Copy to stdout
         (void)dup(std_handle);
         // Copy to stderr
         (void)dup(std_handle);

         // chdir to working dir
         int ret = chdir("/");
         if (ret < 0) {
            syslog(LOG_ERR, "chdir() failed\n");
            closelog();
            exit(1);
         }

         syslog(LOG_DEBUG, "Daemon PID=%d\n", getpid());

      } else {
         syslog(LOG_ERR, "Invalid argument, use -d\n");
         closelog();
         exit(1);
      }
   }

   // Socket Server implementation

   struct sockaddr_in localAddress;
   struct sockaddr_in remoteAddress;

   localSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (localSocket == -1) {
      syslog(LOG_ERR, "socket() failed\n");
      closelog();
      exit(1);
   }

   localAddress.sin_family      = AF_INET;
   localAddress.sin_port        = htons(9000);
   localAddress.sin_addr.s_addr = inet_addr("0.0.0.0");

   // Bind to IPv4 port 9000
   if (bind(localSocket, (struct sockaddr*)&localAddress, sizeof(localAddress)) == -1) {
      syslog(LOG_ERR, "Could not bind to port 9000");
      prepareExit();
      exit(1);
   }

   if (listen(localSocket, 1)) {
      syslog(LOG_ERR, "listen() failed\n");
      prepareExit();
      exit(1);
   }

   logFile = open("/var/tmp/aesdsocketdata", O_CREAT | O_TRUNC | O_RDWR, 0666);
   if (logFile == -1) {
      syslog(LOG_ERR, "open logfile failed\n");
      prepareExit();
      exit(1);
   }

   syslog(LOG_DEBUG, "Waiting for remote socket....\n");

   while (1) {

      unsigned int len = sizeof(remoteAddress);
      remoteSocket     = accept(localSocket, (struct sockaddr*)&remoteAddress, &len);
      if (remoteSocket == -1) {
         syslog(LOG_ERR, "accept failed\n");
         prepareExit();
         exit(1);
      }
      syslog(LOG_DEBUG, "Accepted connection from %s\n", inet_ntoa(remoteAddress.sin_addr));

      // init packet buffer size
      int packetBufferSize     = REC_CHUNK_SIZE;
      int receivedPacketLength = 0;
      packetBufferOffset       = 0;

      while (1) {

         // Resize packet buffer
         pPacketBuffer = (char*)realloc(pPacketBuffer, packetBufferSize);
         if (pPacketBuffer == NULL) {
            syslog(LOG_ERR, "realloc() failed\n");
            prepareExit();
            exit(1);
         }

         // Get received data (max REC_CHUNK_SIZE).
         int recvLen = recv(remoteSocket, pPacketBuffer + packetBufferOffset, REC_CHUNK_SIZE, 0);
         if (recvLen == 0 || recvLen == -1) {
            syslog(LOG_DEBUG, "Closed connection from %s\n", inet_ntoa(remoteAddress.sin_addr));
            prepareClose();
            break;
         }

         // See if End Of Packet is received
         for (int i = packetBufferOffset; i < packetBufferOffset + recvLen; i++) {
            if (pPacketBuffer[i] == '\n') {
               receivedPacketLength = i + 1;
               break;
            }
         }

         packetBufferOffset = packetBufferOffset + recvLen;
         if (receivedPacketLength > 0) {
            // End Of Packet was received
            int writeLen = write(logFile, pPacketBuffer, receivedPacketLength);
            if (writeLen == -1) {
               syslog(LOG_DEBUG, "write of Packet Buffer to logfile failed\n");
               prepareClose();
               break;
            }

            // Start reading logfile from the beginning
            if (lseek(logFile, 0, SEEK_SET) == -1) {
               syslog(LOG_DEBUG, "Seek to strt of logfile failed\n");
               prepareClose();
               break;
            }

            char sendBuffer[REC_CHUNK_SIZE + 1];
            int  readLength = 0;
            while ((readLength = read(logFile, sendBuffer, REC_CHUNK_SIZE))) {
               if (send(remoteSocket, sendBuffer, readLength, 0) == -1) {
                  syslog(LOG_DEBUG, "Sending a packet failed\n");
                  prepareClose();
                  break;
               }
            }

            // Reset buffer for new packet
            packetBufferSize     = REC_CHUNK_SIZE;
            receivedPacketLength = 0;
            packetBufferOffset   = 0;
         } else {
            // Waiting for more data, increase buffer
            packetBufferSize = packetBufferSize + REC_CHUNK_SIZE;
         }
      }
   }
}

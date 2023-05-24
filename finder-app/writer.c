#include <stdio.h>
#include <syslog.h>

int main (int argc, char *argv[]) {

   // Initialize connection with syslog
   openlog("assignement2", LOG_PID | LOG_CONS, LOG_USER);

   if (argc != 3) {
      // Exit when less or more then 2 parameters are passed
      // Note: executable name is first argv item
      syslog(LOG_ERR, "Invalid number of parameters");
      return 1;
   }

   // Create dynamic log message
   char buffer[100];
   sprintf(buffer, "Writing %s to %s", argv[2], argv[1]);

   // Write log message to syslog
   syslog(LOG_DEBUG, buffer);

   // Create and open file for writing
   FILE* writer_file = fopen (argv[1], "w");
   if (writer_file == NULL)
   {
      // Exit when file could not be opened
      syslog(LOG_ERR, "Could not open file for writing");
      return 1;
   }

   // Write second parameter to file
   fprintf(writer_file, argv[2]);

   // Gracefully flose file
   fclose(writer_file);

   // return success
   return 0;
}

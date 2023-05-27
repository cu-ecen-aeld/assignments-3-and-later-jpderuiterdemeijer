#include "systemcalls.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    // Call the command with system(), and store return value
    int result = system(cmd);

    // Determine result, and return accordingly
    return (result == 0) ? true : false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    va_end(args);

    // Create a new process
    pid_t fork_pid = fork();
    int status = 0;

    if (fork_pid >= 0) {
        // Process created succesfully

        if (fork_pid >= 0) {
            // The process we are in is the child process launched by the parent process

            // Since this is the child process, execute the command
            execv(command[0], &command[1]);

            // execv() only returns in case of error, so return 'false' 
            return false;
        } else {
           // The process we are in is the parent process

           // Wait for the child process to end
           wait(&status);

           // When wait returns, the child process was ended (or changed state)
        }
    } else {
       // An error occured when creating a new process, return false
       return false;
    }

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


    va_end(args);

    // Create and open the redirect file for writing
    int redirect_file = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);

    if (redirect_file < 0) {
        // Creating/Opening the redirect file failed, return false
       return false;
    }

    // Create a new process
    pid_t fork_pid = fork();
    int status = 0;

    if (fork_pid >= 0) {
        // Process created succesfully

        if (fork_pid >= 0) {
            // The process we are in is the child process launched by the parent process

            // Redirect stdout from the child process to the stdout of the parent
            if (dup2(redirect_file, 1) < 0) {
                // Redirecting filed
                return false;
            }

            close(redirect_file);

            // Since this is the child process, execute the command
            execv(command[0], &command[1]);

            // execv() only returns in case of error, so return 'false' 
            return false;
        } else {
           // The process we are in is the parent process

           close(redirect_file);

           // Wait for the child process to end
           wait(&status);

           // When wait returns, the child process was ended (or changed state)
        }
    } else {
       // An error occured when creating a new process, return false

       close(redirect_file);

       return false;
    }

    return true;
}

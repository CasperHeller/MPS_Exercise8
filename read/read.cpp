#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <linux/input.h>

/**********************************************************
 * Program for reading a single a char from a char device.
 *
 * First argument is which file to read
 * Second argument defines many bytes it should attempt to read
 *
 * Made by TeamMPS
 **********************************************************/

const int BUFFER_SIZE = 100;

int main(int narg, char *argp[])
{
    int err;

    // Check arguments
    if (narg < 2 || narg > 3)
    {
        if (narg < 3)
            printf("Not enough arguments.\n");
        else
            printf("Too many arguments.\n");

        printf("Please specify which file to read,\n");
        printf("followed by how many bytes to read. (max 100)\n");
        printf("Example: ./read /dev/random 1\n");
        return EXIT_FAILURE;
    }

    printf("Reading from file: %s\n", argp[1]);

    // Open Files
    ssize_t fileHandle;
    if((fileHandle = open(argp[1], O_RDONLY | O_NONBLOCK)) < 0) {
        err = errno;
        printf("Error opening %s, %s\n", argp[1], strerror(err));
        return err;
    }

    char readValue[BUFFER_SIZE];
    ssize_t reader;

    // if amount to read is specified:
    int readAmount;
    if (narg == 3)
    {
        readAmount = (atoi(argp[2]) <= BUFFER_SIZE) ? atoi(argp[2]) : BUFFER_SIZE;
    }
    // Else just read a single char:
    else
    {
        readAmount = 1;
    }

    // Read amount specified
    reader = read(fileHandle, &readValue, readAmount);
    if (reader == -1)
    {
        err = errno;
        printf("Error reading: %s\n", strerror(err));
    }
    else
    {
        printf("%s\n", readValue);
    }

    close(fileHandle);
    return 0;
}

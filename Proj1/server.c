#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>

#define MAXLEN 1024

int readCPULoad() {
   long int prevIdle, idle, nonIdle, prevTotal, total, totalD, idleD;
   double cpuPercentage;
   int i = 0, j = 0;
   char str[100];
   char *output;

    while (i < 2) {
        FILE *fp = fopen("/proc/stat", "r");
        fgets(str, 100, fp);
        fclose(fp);
        output = strtok(str, " ");

        idle = nonIdle = 0;
        j = 0;
        while (output != NULL && j < 8) {
           output = strtok(NULL, " ");

           if (output != NULL) {

               // idle = idle + iowait
               if (j == 3 || j == 4) {
                   idle += atoi(output);
               } else {
                   nonIdle += atoi(output);
               }
               j++;
           }
        }

        total = idle + nonIdle;
        if (i == 0) {
           prevIdle = idle;
           prevTotal = total;
        }

        usleep(500000);
        i++;
    }
    totalD = total - prevTotal;
    idleD = idle - prevIdle;
    cpuPercentage = (double)(totalD - idleD) / (double)totalD;

    return (int)(cpuPercentage*100.0);
}

char * readShellOutput(char *command) {
    FILE *fp;
    // allocating buffer
    char * buffer = malloc(sizeof(char) * MAXLEN);
    if (buffer == NULL) {
        fprintf(stderr, "Allocation failed...");
        exit(1);
    }
    else if (strcmp(command, "cat /proc/stat")==0) {
        // calls CPU usage function

        snprintf(buffer, 50, "%i", readCPULoad());
        strcat(buffer, "%\n");
    }
    else {
        // other commands that can be called straight from CLI
        fp = popen(command,"r");
        fgets(buffer, MAXLEN, fp);
        fclose(fp);
    }

    return buffer;
}

char * readInput(char *buffer) {
    char * output = NULL;

    // sends a command to CLI (in case of proc stat it calls a separate function)
    if (strstr(buffer, "GET /hostname ")) {
        output = readShellOutput("hostname");
    }
    else if (strstr(buffer, "GET /cpu-name ")) {
    	// other implementations (merlin was missing Adress sizes: row in lscpu so getting 13th row did not work 		on ubuntu)
        //lscpu | sed -n '13 p' | awk '{printf("%s %s %s %s %s %s %s\n",$3,$4,$5, $6, $7, $8, $9)}'
        //output = readShellOutput("lscpu | sed -n '13 p' | awk '{printf(\"%s %s %s %s %s %s %s\\n\",$3,$4,$5, 		$6, $7, $8, $9)}'");
        output = readShellOutput("lscpu | grep \"Model name:\" | sed -r 's/Model name:\\s{1,}//g'");
    }
    else if (strstr(buffer, "GET /load ")) {
        output = readShellOutput("cat /proc/stat");
    } else {
        output = strdup("err");
    }

    return output;
}

char *formatOutput(char *message, int messageLength) {
    char * buffer = malloc(sizeof(char) * MAXLEN * 2);
    if (buffer == NULL) {
        fprintf(stderr, "Allocation failed...");
        exit(1);
    }

    char messageLenBuffer[32];

    // creating a response message, err sends 400 Bad Request
    if (strcmp(message, "err") == 0) {
        strcpy(buffer, "HTTP/1.1 400 Bad Request\nContent-Type: text/plain; charset=utf-8\r\n\r\nBad request\r\n");
    }else {
        strcpy(buffer, "HTTP/1.1 200 OK\nContent-Type: text/plain; charset=utf-8\nContent-Length:");
        sprintf(messageLenBuffer, "%i", messageLength);
        strcat(buffer, messageLenBuffer);
        strcat(buffer, "\r\n\r\n");
        strcat(buffer, message);
        strcat(buffer, "\r\n");
    }

    free(message);

    return buffer;
}

int main(int argc, char *argv[]) {
    int port = 0; // init value
    int server_socket;

    struct sockaddr_in address;

    if (argc == 2) {
        // checking if arg is a number
        for (int i = 0; argv[1][i]!= '\0'; i++)
        {
            if (isdigit(argv[1][i]) == 0) {
                fprintf(stderr, "Invalid port format.\n");
                exit(1);
            }
        }

        port = atoi(argv[1]);

    } else {
        fprintf(stderr, "Incorrect amount of arguments.");
        return 1;
    }

    // creation of the server_socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // address specification
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );

    // if socket is reused
    int one = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // server_socket binding
    int bind_state = bind(server_socket, (struct sockaddr*) &address, sizeof(address));

    if (bind_state == -1) {
        fprintf(stderr, "Binding failed...");
        return 1;
    }

    // listening for connections
    int listen_state = listen(server_socket, 3);

    if (listen_state == -1) {
        fprintf(stderr, "Listening failed...");
        return 1;
    }

    while(1)
    {
        int addrlen = sizeof(address);
        int new_socket;

        if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("Accept failed...");
            return 1;
        }

        char buffer[MAXLEN] = {0};
        recv( new_socket , buffer, MAXLEN, 0);

        // read command that will be processed
        char *returnValue = readInput(buffer);

        // response message creation
        char *data = formatOutput(returnValue, strlen(returnValue));

        send(new_socket , data , strlen(data), 0);

        free(data);
        close(new_socket);
    }
}


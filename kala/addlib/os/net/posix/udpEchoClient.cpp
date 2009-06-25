#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char output[1500];
char input[1500];

int main(int argc, char* argv[])
{
    int s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s == -1)
    {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("192.168.2.40");
    serverAddr.sin_port = htons(7);

    strcpy(output, "abc");
    int n = sendto(s, output, strlen(output), 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
    if (n == -1)
    {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    n = recvfrom(s, input, 1500, 0, (struct sockaddr*) &clientAddr, &clientLen);
    if (0 < n)
    {
        printf("'%s', %d\n", input, ntohs(clientAddr.sin_port));
    }
    else
    {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }

    close(s);
}

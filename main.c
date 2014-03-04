/*
    Filename: main.c
    This file defines the entry point for the application.

    Copyright (C) 2014  Corey Shuman <ctshumancode@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#define BUFLEN 512
#define PORT 2002

void err(char *str)
{
    perror(str);
    exit(1);
}

int main(int argc, char **argv)
{
    struct sockaddr_in my_addr, cli_addr;
    int sockfd, i; 
    socklen_t slen=sizeof(cli_addr);
    unsigned char buf[BUFLEN];
    int fd;
    int daemonize = 1;

    while(1) {
      int c;
      int option_index;

      static struct option long_options[] = {
        { "debug",	no_argument,	0, 'd' },
        { "help",	no_argument,	0, 'h' },
        { 0,		0,		0, 0   }
      };
      c = getopt_long(argc, argv, "d", long_options, &option_index);
      if (c == -1) {
        break;
      } else if (c == 'd') {
        daemonize = 0;
      } else if (c == 'h') {
        printf("\nUsage: %s <options>\n\n"
                 "Options:\n"
                 " --debug      Run in local mode (don't daemonize)\n"
		 " --help       Show this prompt.\n",
		 argv[0] );
      } else {
        err("Invalid Parameters\n");
      }
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("socket");

    fd = open("/dev/servoblaster", O_RDWR);
    if(fd < 0) {
      err("open /dev/servoblaster");
    }

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1)
      err("bind");

    if(daemonize && daemon(0,1) < 0)
      err("Failed to daemonize process\n");

    while(1)
    {
        int cnt;
        cnt = recvfrom(sockfd, (char*)buf, BUFLEN, 0, (struct sockaddr*)&cli_addr, &slen);
        if(cnt == -1)
           err("recvfrom()");
        if(!daemonize)
          printf("\n%s:%d ",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

	// data format is 3 bytes (x,y,z), each representing 0-100% servo rotation
	if(cnt == 3)
	{
	   char str[32];
	   sprintf(str, "0=%u%%\n1=%u%%\n2=%u%%\n", (unsigned char)((float)buf[0]/2.55),
                 (unsigned char)((float)buf[1] / 2.55),
                 (unsigned char)((float)buf[2] / 2.55) );
           //if(lseek(fd, 1, SEEK_SET) < 0) {
           //  err("lseek");
           //}
           if(write(fd, str, strlen(str)) < 0) {
             err("device write failed");
           }
           if(!daemonize)
             printf("x=%u%% y=%u%%, z=%u%%",(unsigned char)((float)buf[0]/2.55),
                 (unsigned char)((float)buf[1]/2.55), (unsigned char)((float)buf[2]/2.55) );
	} 
    }

    close(sockfd);
    return 0;
}

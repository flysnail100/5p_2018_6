//usage: ./test_1 <ip_serv> <port_serv>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <pthread.h>
#include <assert.h>

#define SPP_BUFF 752
#define PAYLOAD_LEN 740
#define PAYLOAD_LEN_TM 734

struct sockaddr_in6 sock_serv, sock_from;

struct packet_t
{
	//header_t header
	unsigned int ver:3;
	unsigned int type:1;
	unsigned int flag:1;
	unsigned int APID_1:11;
	unsigned int seq_flag:2;
	unsigned int packet_seq_count:14;
	unsigned int packet_len:16;

	//header_assinstant_t header_assistant
	unsigned int free_1:1;
	unsigned int TM_ver:3;
	unsigned int free_2:4;
	unsigned int service_type:8;
	unsigned int service_sub_type:8;
	unsigned int packet_counter:8;
	unsigned int free_3:5;
	unsigned int APID_2:11;

	//PAYLOAD_LEN_TM
	unsigned char payload[PAYLOAD_LEN];
};

int fill_packet(char *src, unsigned long size, struct packt_t *packet)
{
	//initialize the packet header
	packet->ver = 0;
	packet->type = 1;
	packet->flag = 1;
	packet->service_type = 131;
	packet->service_sub_type = 1;

	memcpy(packet->payload, src, size);
	return EXIT_SUCCESS;
}

int create_client_socket(int port, char* ipaddr)
{
	int l;
	int sfd;

	sfd = socket(AF_INET6, SOCK_DGRAM, 0);
	if(sfd == -1)
	{
		perror("socket fail.");
		return EXIT_FAILURE;
	}

	l = sizeof(struct sockaddr_in6);
	bzero(&sock_serv, l);

	sock_serv.sin6_family = AF_INET6;
	sock_serv.sin6_port = htons(port);

	if(inet_pton(AF_INET6, ipaddr, &sock_serv.sin6_addr) == 0)
	{
		perror("invalid IP address.");
		return EXIT_FAILURE;
	}

	return sfd;
}

int main(int argc, char **argv)
{
	int sfd;
	int fd;
	socklen_t adr_sz;

	struct stat buffer;
	off_t = sz;

	char databuf[SPP_BUFF];
	char static_address[PAYLOAD_LEN_TM];
	char route_table_path[100];

	struct packet_t packet;
	int l = sizeof(struct sockaddr_in6);
	int m, count = 0;

	if(argc!=3)
	{
		printf("error usage: %s <ip_serv> <port_serv>.",argv[0]);
		return EXIT_FAILURE;
	}

	sfd = create_client_socket(atoi(argv[2]), argv[1]);

	while(1)
	{
		fputs("Input static route table file to send(q to quit): ",stdout);
		fgets(route_table_path, sizeof(route_table_path), stdin);
		if(!strcmp(route_table_path, "q\n") || !strcmp(route_table_path, "Q\n"))
			break;
		if((fd = open(route_table_path, O_RDONLY)) == -1)
		{
			perror("file open failed.");
			return EXIT_FAILURE;
		}

		if(stat(route_table_path, &buffer) == -1)
		{
			perror("file stat failed.");
			return EXIT_FAILURE;
		}
		sz = buffer.st_size;

		bzero(static_address, PAYLOAD_LEN_TM);
		n = read(fd, static_address, PAYLOAD_LEN_TM);
		fill_packet(static_address, n, &packet);
		while(n)
		{
			if(n == -1)
			{
				perror("read data fails");
				return EXIT_FAILURE;
			}

			m = sendto(sfd, (char *)(&packet), sizeof(packet), 0, (struct sockaddr*)&sock_serv, l);
			if(m == -1)
			{
				perror("data send failed.")
				return EXIT_FAILURE;
			}	
			count += m;
			bzero(static_address, PAYLOAD_LEN_TM);
			n = read(fd, static_address, PAYLOAD_LEN_TM);
			fill_packet(static_address, n, &packet);
		}
		printf("size of file(send): %lld\n", sz);
		printf("total size(send): %lld\n", count);

		close(fd);
	}
	close(sfd);
	return 0;
}
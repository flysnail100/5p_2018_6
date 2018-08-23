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
#include <inttypes.h>
#include <stdint.h>

#define SO_REUSEPORT 15

#define MAXEPOLLSIZE 100

#define SPP_BUFF 752
#define PAYLOAD_LEN 740
#define PAYLOAD_LEN_TM 734
//#define INET6_ADDRSTRLEN 46

//value to process
int packet_len = 0;
int service_type = 0;
int service_sub_type = 0;
char recv_payload[PAYLOAD_LEN];

int flag = 0;
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

	//header_assistant_t header_assistant
	unsigned int free_1:1;
	unsigned int TM_ver:3;
	unsigned int free_2:4;
	unsigned int service_type:8;
	unsigned int service_sub_type:8;
	unsigned int packet_counter:8;
	unsigned int free_3:5;
	unsigned int APID_2:11;
	unsigned int time_1:32;
	unsigned int time_2:16;

	//PAYLOAD_LEN_TM比PAYLOAD_LEN还要再少6bytes
	unsigned char payload[PAYLOAD_LEN_TM];
};

uint8_t HexToChar(uint8_t temp)
{
	uint8_t res;

	//需要大写转换的话，将a变为A即可
	if(temp<10)
	{
		res = res + '0';
	}
	else
	{
		res = res - 10 + 'a';
	}

	return res;
}

int fill_packet(char *src, unsigned long size, struct packet_t *packet)
{
	//initialize the packet header
	packet->ver = 0;
	packet->type = 1;
	packet->flag = 1;

	memcpy(packet->payload, src, size);
	return EXIT_SUCCESS;
}

int config_staticroutetable(char recv_payload[])
{
	//要加个文件删除转存的逻辑，先这么写吧！
	if(remove("static_route_table") == 0)
	{
		printf("old static_route_table has been removed.\n");
	}
	fp = fopen("static_route_table", "a");

	/*
	StaticRouteTable的长度应该是TableBuffer的两倍，
	但长度足够所以暂时设为该值。
	*/
	uint8_t TableBuffer[PAYLOAD_LEN];
	char StaticRouteTable[PAYLOAD_LEN];
	char RouteLine[32];
	char RouteLineBuffer[40]; 

	int fd;
	if((fd=open("static_route_table",O_CREAT|O_WRONLY|O_TRUNC,0600))==-1)
	{
		perror("create or open file fail.");
		return EXIT_FAILURE;
	}

	int len_1;
	len_1 = strlen(recv_payload);
	int table_len = len_1 - 6;
	printf("%d\n",len);

	memcpy(TableBuffer, recv_payload + 6, table_len);

	for(int i=0; i<table_len; i++)
	{
		StaticRouteTable[2*i]=HexToChar(TableBuffer[i]>>4);
		StaticRouteTable[2*i+1]=HexToChar(TableBuffer[i]&0xf);
	}

	int len_2;
	len_2 = strlen(StaticRouteTable);
	printf("StaticRouteTable length: %d\n", len_2);
	int count;
	count = len_2/68;
	printf("the number of route line: %d\n", count);

	int a = 0;
	int b = 0;
	for(int c=0; c<count; c++)
	{
		memcpy(RouteLine, StaticRouteTable+68*c, 32);
		for(int m=0; m<32; m++)
		{
			if(b == 4)
			{
				RouteLineBuffer[a]=':';
				a++;
				RouteLineBuffer[a]=RouteLine[m];
				b=0;
			}
			else
			{
				RouteLineBuffer[a]=RouteLine[m];
			}
			a++;
			b++;
		}
		strcat(RouteLineBuffer," ");
		write(fd, RouteLineBuffer, 40);

		memset(RouteLineBuffer, 39*sizeof(char), 0);
		a=0;
		b=0;

		memcpy(RouteLine, StaticRouteTable+68*c+32, 32);
		for(int n=0; n<32; n++)
		{
			if(b == 4)
			{
				RouteLineBuffer[a]=':';
				a++;
				RouteLineBuffer[a]=RouteLine[n];
				b=0;
			}
			else
			{
				RouteLineBuffer[a]=RouteLine[n];
			}
			a++;
			b++;
		}
		strcat(RouteLineBuffer,"\n");
		write(fd, RouteLineBuffer, 40);

		memset(RouteLineBuffer, 39*sizeof(char), 0);
		a=0;
		b=0;
	}

	close(fd);

	system("echo 1234 | sudo -S ./configure_static_routetable");
	printf("recv command: configure static route table successfully.\n");
	return EXIT_SUCCESS;
}

int transform_to_OSPF6()
{
	system("echo 1234 | sudo -S ./transform_OSPF6");
	printf("recv command: transform to OSPF6 successfully.\n");
	return EXIT_SUCCESS;
}

int set_terminal_ip(char recv_payload[])
{
	char nic[20];
	char ip_address[39];
	char command[100];
	int m,n;

	for(m = 0; m < 20; m++)
	{
		if(recv_payload[m] == '%')
			break;
		nic[m] = recv_payload[m];
	}
	
	for(n = 0; n < 39; n++)
	{
		m = m+1;
		if(recv_payload[m] == '%')
			break;
		ip_address[n] = recv_payload[m];
	}

	sprintf((char*)command, "echo 1234 | sudo -S ifconfig %s add %s/64", nic, ip_address);
	system(command);
	printf("recv command: set terminal ip and ip updated successfully.\n");
	return EXIT_SUCCESS;
}

int set_laser_link(char recv_payload[])
{
	char nic[20];
	char status[10];
	char command[100];
	int m,n;

	for(m = 0; m < 20; m++)
	{
		if(recv_payload[m] == '%')
			break;
		nic[m] = recv_payload[m];
	}

	for(n = 0; n < 10; n++)
	{
		m = m + 1;
		if(recv_payload[m] == '%')
			break;
		status[n] = recv_payload[m];
	}

	if(status == "down")
	{
		sprintf((char*)command, "echo 1234 | sudo -S ifconfig %s down", nic);
		system(command);
		printf("recv command: set laser link down successfully.\n");
	}
	else if(status == "up")
	{
		sprintf((char*)command, "echo 1234 | sudo -S ifconfig %s up", nic);
		system(command);
		printf("recv command: set laser link up successfully.\n");
	}
	else
	{
		printf("recv command error.\n");
		EXIT_FAILURE;
	}

	return EXIT_SUCCESS;	
}

//和read_data相对，作为一个测试用例，向对端写入数据
int write_data(int sd)
{
	int fd;
	char databuf[PAYLOAD_LEN];
	off_t m,sz,count;
	count = 0;
	unsigned long n;
	struct stat buffer;
	struct packet_t packet;

	if((fd = open("testfile", O_RDONLY)) == -1)
	{
		perror("open fail");
		return EXIT_FAILURE;
	}

	if(stat("testfile", &buffer) == -1)
	{
		perror("stat fail");
		return EXIT_FAILURE;
	}
	sz = buffer.st_size;

	bzero(databuf, PAYLOAD_LEN);
	n = read(fd, databuf, PAYLOAD_LEN);
	fill_packet(databuf, n, &packet);
	while(n)
	{
		if(n==-1)
		{
			perror("read data fails");
			return EXIT_FAILURE;
		}
		//已有sd，而没有初始化好的结构体，所以用send来处理
		//m = sendto(sd,(char *)packet, SPP_BUFF, 0, (struct sockaddr*))
		m = send(sd, (char *)&packet, SPP_BUFF, 0);
		if(m == -1)
		{
			perror("data send failed.");
			return EXIT_FAILURE;
		}
		count+=m;
		bzero(databuf, PAYLOAD_LEN);
		n = read(fd, databuf, PAYLOAD_LEN);
		fill_packet(databuf, n, &packet);
	}

	printf("size of file(send): %lld\n", sz);
	printf("total size(send): %lld\n", count);

	close(fd);

	return EXIT_SUCCESS;
}

//read_data用来打印接收到的数据
int get_values_and_select_process(int sd)
{
	char recv_buf[SPP_BUFF];
	int ret;
	struct sockaddr_in6 client_addr;
	socklen_t cli_len = sizeof(client_addr);

	bzero(recv_buf,SPP_BUFF);

	ret = recvfrom(sd, recv_buf, SPP_BUFF, 0, (struct sockaddr*)&client_addr, &cli_len);
	if(ret>0)
	{
		printf("read[%d]: %s from %d\n", ret, recv_buf, sd);
	}
	else
	{
		printf("read err: %s %d\n", strerror(errno), ret);
	}
	fflush(stdout);

	if(first_get_values_and_select_process(recv_buf, sd) == EXIT_SUCCESS)
	{
		printf("process rely on the content of packet successfully!\n");
	}
	else
		return EXIT_FAILURE;	
	
	return EXIT_SUCCESS;
}

int first_get_values_and_select_process(char recv_buffer[], int sd)
{
	int return_value;
	memcpy(&packet_len, recv_buffer + 4, 2);
	memcpy(&service_type, recv_buffer + 7, 1);
	memcpy(&service_sub_type, recv_buffer + 8, 1);
	bzero(recv_payload, PAYLOAD_LEN);
	memcpy(recv_payload, recv_buffer + 12, PAYLOAD_LEN);
	printf("packet_len:\t%d\nservice_type:\t%d\nservice_sub_type:\t%d\n",packet_len,service_type,service_sub_type);

	/*因为是模拟终端收到的指令，看了业务类型的表格，都是主控0x401过来的，
	  所以这里不再判断源APID，既然已经接受并交给spp_server处理，所以根据
	  下层IP已经确定了接收的终端，所以也不再对目的APID判断。
	  但最后在生成反馈的数据包时，要在这些位置上设置对应的信息，用一个结构体来实现。
	 */
	if(service_type == 131)
	{
		switch(service_sub_type)
		{
			case 1:
				//软件路由表上注
				return_value = config_staticroutetable(recv_payload);
				if(return_value != EXIT_SUCCESS)
					printf("configure static route table failed!");
				else
					printf("configure static route table successfully!");
				break;

			case 2:
				//FPGA路由表上注
				break;

			case 3:
				//软件路由表删除
				return_value = transform_to_OSPF6(recv_payload);
				if(return_value != EXIT_SUCCESS)
					printf("transform to OSPF6 failed!");
				else
					printf("transform to OSPF6 successfully!");
				break;
				
			case 4:
				//FPGA路由表删除
				break;
			
			case 9:
				//激光终端链路通断控制
				//想到的解决方法是收到需断开端的网卡名，然后直接ifconfig down/up
				return_value = set_laser_link(recv_payload);
				if(return_value != EXIT_SUCCESS)
					printf("set laser link failed!");
				else
					printf("set laser link successfully!");
				break;
			
			case 10:
				//终端IP地址设置
				return_value = set_terminal_ip(recv_payload);
				if(return_value != EXIT_SUCCESS)
					printf("set terminal ip failed!");
				else
					printf("set terminal ip successfully!");				
				break;
			
			case 20:
				//this case as a test example
				return_value = write_data(sd);
				if(return_value != EXIT_SUCCESS)
					printf("write data to peer_addr failed!");
				else
					printf("write data to peer_addr successfully!");
				break;
			
			default:
				printf("No such service_sub_type and can't response the sender!");
				break;
		}
	}

}

int udp_accept(int sd, struct sockaddr_in6 my_addr)
{
	int new_sd=-1;
	int ret=0;
	int reuse=1;
	//这里的buf长度要做修改
	char recv_buf[SPP_BUFF];
	struct sockaddr_in6 peer_addr;
	socklen_t cli_len = sizeof(peer_addr);

	/*recvfrom首先接收来自client的请求和指令，所以这里的if中我应该添加
	  对接收到数据的处理过程，之后由connect的child socket来与对端通信
	  （其实只有一个主控，我觉得这里的UDP并发并不必要，但先这么写吧）
	recvfrom函数会将对端的地址存储在提供的struct sockaddr中，这里是peer_addr
	*/
	ret = recvfrom(sd, recv_buf, SPP_BUFF, 0, (struct sockaddr*)&peer_addr, &cli_len);
	if(ret>0)
	{
		printf("first recv data successfully and will build the connect with peer_addr.\n");
	}

	if((new_sd = socket(PF_INET6, SOCK_DGRAM, 0))== -1)
	{
		perror("child socket");
		return EXIT_FAILURE;
	}
	else
	{
		printf("parent: %d child: %d\n", sd, new_sd);
	}

	ret = setsockopt(new_sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(ret)
	{
		perror("setsockopt SO_REUSEADDR");
		return EXIT_FAILURE;
	}

	ret = setsockopt(new_sd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
	if(ret)
	{
		perror("setsockopt SO_REUSEPORT");
		return EXIT_FAILURE;
	}

	ret = bind(new_sd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr_in6));
	if(ret)
	{
		perror("child bind");
		return EXIT_FAILURE;
	}
	else
	{

	}

	peer_addr.sin6_family = PF_INET6;
	//printf("address: %s\n", inet_ntoa(peer_addr.sin6_addr));
	char str[INET6_ADDRSTRLEN];
	printf("address: %s\n", inet_ntop(AF_INET6, &peer_addr.sin6_addr, str, sizeof(str)));
	if(connect(new_sd, (struct sockaddr*)&peer_addr, sizeof(struct sockaddr_in6))==-1)
	{
		perror("child connect");
		return EXIT_FAILURE;
	}
	else
	{

	}

	if(first_get_values_and_select_process(recv_buf, new_sd) == EXIT_SUCCESS)
	{
		printf("first connect and process rely on the content of packet successfully!\n");
	}
	else
		return EXIT_FAILURE;

out:
	return new_sd;	
}

int main(int argc, char **argv)
{
	int listener, kdpfd, nfds, n, curfds;
	socklen_t len;
	struct sockaddr_in6 my_addr, their_addr;
	unsigned int port;
	struct epoll_event ev;
	struct epoll_event events[MAXEPOLLSIZE];
	int opt=1;
	int ret=0;

	port = 1234;

	if((listener = socket(PF_INET6, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		return EXIT_FAILURE;
	} 
	else
	{
		printf("socket OK\n");
	}

	ret = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(ret)
	{
		perror("setsockopt SO_REUSEADDR");
		return EXIT_FAILURE;
	}

	ret = setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if(ret)
	{
		perror("setsockopt SO_REUSEPORT");
		return EXIT_FAILURE;
	}

	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin6_family = PF_INET6;
	my_addr.sin6_port = htons(port);
	//my_addr.sin6_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin6_addr = in6addr_any;
	if(bind(listener, (struct sockaddr*)&my_addr, sizeof(struct sockaddr_in6)) == -1)
	{
		perror("bind");
		return EXIT_FAILURE;
	}
	else
	{
		printf("IP bind OK\n");
	}

	kdpfd = epoll_create(MAXEPOLLSIZE);

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listener;

	if(epoll_ctl(kdpfd, EPOLL_CTL_ADD, listener, &ev) < 0)
	{
		fprintf(stderr, "epoll set insertion error: fd = %d\n", listener);
		return EXIT_FAILURE;
	}
	else
	{
		printf("ep add OK\n");
	}

	/*
		这里的while循环，是如果之前没有在epoll的数组中进行注册，则生成新的线程，并注册在events中；
		若之前已经注册，则用已经生成的注册的文件描述符完成数据接收。
	*/
	while(1)
	{
		nfds = epoll_wait(kdpfd, events, 100, -1);
		if(nfds == -1)
		{
			perror("epoll_wait");
			break;
		}

		/*
			按这里和查阅的代码都是这么写的，虽然现在还不太明白event的排序问题，但应该是
			将所有可用的事件按序排好了，即管理好了。
			listener状态通过udp_accept转换为连接状态！
		*/
		for(n = 0; n < nfds; ++n)
		{
			if(events[n].data.fd == listener)
			{
				printf("listener: %d\n", n);
				int new_sd;
				struct epoll_event child_ev;

				if((new_sd = udp_accept(listener, my_addr)) == EXIT_FAILURE)
				{
					printf("udp_accept failed.\n");
					return EXIT_FAILURE;
				}
				child_ev.events = EPOLLIN;
				child_ev.data.fd = new_sd;
				if(epoll_ctl(kdpfd, EPOLL_CTL_ADD, new_sd, &child_ev) < 0)
				{
					fprintf(stderr, "epoll set insertion error: fd = %d\n", new_sd);
					return EXIT_FAILURE;
				}
			}
			else
			{
				get_values_and_select_process(events[n].data.fd);
			}
		}
	}
	close(listener);
	return EXIT_SUCCESS;
}




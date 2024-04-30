/*======================================================================*/
/*									*/
/* File:        broadcast.c						*/
/* Description: This file contains a utility to send or receive		*/
/*		broadcast messages. It is meant as a little test to	*/
/*		check if broadcasting on the network will work.		*/
/*		One feature of this utility is the ability to specify	*/
/*		the Multi-cast TTL, which affects how many routers	*/
/*		the broadcast can go through. By default the protocol	*/
/*		is set to 1, which means it doesn't make it past any	*/
/*		routers. Increasing the value should allow the broadcast*/
/*		to go through routers, and in theory, if the router	*/
/*		allows, to travel to further subnets.			*/
/*									*/
/*		Note that there is a Unix version of this utility	*/
/*		that works with this version.				*/
/*									*/
/* Usage:	The format is:						*/
/*			[-r] [-AAAA] [-pPPPP] [-tTTTT]			*/
/*		-r indicates to run as receiver.			*/
/*		If not specified the task will run as broadcaster.	*/
/*		PPPP is the port to broadcast to.			*/
/*		TTTT is the multi-cast TTL to use.			*/
/*		AAAA is an override IP address to broadcast to.		*/
/*		     it should be XXX.YYY.ZZZ.255			*/
/* Author:      Aaron Candy						*/
/* Date:        14-Dec-2022						*/
/*									*/
/************************************************************************/
#include <iostream>

#include	<sys/types.h>
#include	<stdio.h>
#include	<winsock.h>
#include	<stdlib.h>

static void broadcast(int s, int nPort, char* sAddr);
static void receive(int s);
static void parseCommandLine(int argc, char* argv[], int* pnPort, int* pnTtl, int* pbRecv, char* sAddr, int nAddrSize);
static void displayUsage(void);
static int initWinSock(void);

int main(int argc, char* argv[])
{
	int			s;		/* socket */
	int			on = 1;		/* set broadcast on */
	struct sockaddr_in	sin;
	int			nPort = 40061;
	int			nTtl = 1;
	int			bRecv;
	char			sOvrAddr[256] = { 0 };

	parseCommandLine(argc, argv, &nPort, &nTtl, &bRecv, sOvrAddr, sizeof(sOvrAddr));

	if (initWinSock() != 0)
		exit(1);

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
		perror("Setting up socket");
		exit(1);
	}

	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on)) < 0)
	{
		perror("Setting broadcast");
		exit(1);
	}

	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&nTtl, sizeof(nTtl)) < 0)
	{
		perror("Setting TTL\n");
		exit(1);
	}

	/*
	* Create the socket that broadcasts will send/receive on
	*/
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(nPort);
	if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
	{
		perror("Binding socket");
		exit(1);
	}

	if (bRecv)
		receive(s);
	else
		broadcast(s, nPort, sOvrAddr);

	exit(0);
}

static void broadcast(int s, int nPort, char *sAddr)
{
	struct sockaddr_in	dst;
	int			nValue = 0;

	dst.sin_family = AF_INET;
	if (strlen(sAddr) > 0)
		dst.sin_addr.s_addr = inet_addr(sAddr);
	else
		dst.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	dst.sin_port = htons(nPort);

	printf("Broadcast to %s:%d\n", inet_ntoa(dst.sin_addr), ntohs(dst.sin_port));
	for (;;)
	{
		printf("Sending %d\n", nValue);
		if (sendto(s, (char *)&nValue, sizeof(nValue), 0, (struct sockaddr*)&dst, sizeof(dst)) < 0)
		{
			perror("Sending broadcast");
			exit(1);
		}

		nValue++;
		Sleep(5000);
	}
}

static void receive(int s)
{
	int			nValue = 0;
	struct sockaddr_in	dst;
	int			size;
	int			result;

	printf("Waiting for data\n");
	for (;;)
	{
		size = sizeof(dst);
		result = recvfrom(s, (char*)&nValue, sizeof(nValue), 0, (struct sockaddr*)&dst, &size);
		if(result < 0)
		{
			if(result == SOCKET_ERROR)
				printf("recvfrom failed with error %d\n", WSAGetLastError());
			exit(1);
		}
		else
		{
			printf("Received %d from '%s'\n", nValue, inet_ntoa(dst.sin_addr));
		}
	}
}

/************************************************************************/
/* Name:	parseCommandLine					*/
/* Description:	Parses the command line and sets the values found.	*/
/* Inputs								*/
/*	argc:	The number of command line arguments			*/
/*	argv:	The argument array.					*/
/*	nAddrSize: Address buffer size.					*/
/* Outputs								*/
/*	pnPort:	The port specified by the command line. If not specified*/
/*		the value passed in doesn't change.			*/
/*	pnTTL:	The TTL specified by the command line. If not specified	*/
/*		the value passed in doesn't change.			*/
/*	pbRecv:	Set to 1 if the task should be in receiver mode, 0 if	*/
/*		in broadcaster mode.					*/
/*	sAddr:	Override broadcast address.				*/
/*		Instead of using INADDR_BROADCAST this address will be	*/
/*		used to send the broadcast.				*/
/************************************************************************/
static void parseCommandLine(int argc, char* argv[], int* pnPort, int* pnTtl,
			     int* pbRecv, char *sAddr, int nAddrSize)
{
	int			i;

	*pbRecv = 0;
	for (i = 1; i < argc; i++)
	{
		switch (argv[i][1])
		{
		case 'a':
			strcpy_s(sAddr, nAddrSize, &argv[i][2]);
			break;
		case 'p':
			*pnPort = strtol(&argv[i][2], NULL, 10);
			break;
		case 'r':
			*pbRecv = 1;
			break;
		case 't':
			*pnTtl = strtol(&argv[i][2], NULL, 10);
			break;
		case '?':
			displayUsage();
			break;
		default:
			printf("Unknown command line argument '%c'\n",
				argv[i][1]);
		}
	}
}

static void displayUsage(void)
{
	printf("broadcast [-r] [-aAAAA] [-pPPPP] [tTTTT] [-?]\n");
	printf("\t  -r\tReceive broadcasts. Default is to send without this flag.\n");
	printf("\tAAAA\tIP address to send broadcast to.\n"
		"\t\tBy default the global broadcast address 255.255.255.255 is used."
		"\t\tIf you want to test a specific network you could use this to specify.\n"
		"\t\tEG 192.168.50.1\n");
	printf("\tPPPP\tThe port to broadcast on. Default is 40061\n");
	printf("\tTTTT\tThe multi-cast TTL to use. Default is 1\n");
}

static int initWinSock(void)
{
	WORD		wVersionRequested;
	WSADATA		wsaData;
	int		err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}

	return(0);
}

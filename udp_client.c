/*******************************
tcp_client.c: the source file of the client in tcp transmission 
********************************/

#include "headsock.h"

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len);                       //transmission function
void tv_sub(struct  timeval *out, struct timeval *in);	    //calcu the time interval between out and in

int main(int argc, char **argv)
{
	int sockfd, ret;
	int i;
	float ti, rt;
	float tiSum, rtSum;
	long len;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	if (argc != 2) {
		printf("parameters not match");
	}

	sh = gethostbyname(argv[1]);	                                       //get host's information
	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}

	printf("canonical name: %s\n", sh->h_name);					//print the remote host's information
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
        
	addrs = (struct in_addr **)sh->h_addr_list;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);                           //create the socket
	if (sockfd <0)
	{
		printf("error in socket");
		exit(1);
	}
	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYUDP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);
	// ret = connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));         //connect the socket with the host
	// if (ret != 0) {
	// 	printf ("connection failed\n"); 
	// 	close(sockfd); 
	// 	exit(1);
	// }
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}
	tiSum = 0;
	rtSum = 0;
	for (i=0; i< 9; i++) {
		ti = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);                       //perform the transmission and receiving
		rt = (len/(float)ti);                                         //caculate the average transmission rate
		tiSum += ti;
		rtSum += rt;
		printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);
	}
	tiSum = tiSum / 10.0;
	rtSum = rtSum / 10.0;
	printf("OVERALL: Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", tiSum, (int)len, rtSum);

	close(sockfd);
	fclose(fp);
//}
	exit(0);
}

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len)
{
	char *buf;
	long lsize, ci;
	char sends[DATALEN*2];
	struct ack_so ack;
	int n, slen, addressInLen, m, i = 1;
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	ci = 0;

    struct sockaddr_in addressIn;

	addressInLen = sizeof (struct sockaddr_in);

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp);
	rewind (fp);
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  // copy the file into the buffer.
	fread (buf,1,lsize,fp);

  /*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0';									//append the end byte
	gettimeofday(&sendt, NULL);							//get the current time
	while(ci<= lsize)
	{
		if ((lsize+1-ci) <= DATALEN * (2-i%2))
			slen = lsize+1-ci;
		else 
			slen = DATALEN * (2-i%2);
		memcpy(sends, (buf+ci), slen);
		n = sendto(sockfd, &sends, slen, 0, addr, addrlen);
		if(n == -1) {
			printf("send error!");								//send the data
			exit(1);
		}

        if ((m= recvfrom(sockfd, &ack, 2, 0, (struct sockaddr *)&addressIn, &addressInLen))==-1)                                   //receive the ack
        {
            printf("error when receiving\n");
            exit(1);
        }
        i += 1;
		ci += slen;
	}
	// if ((n= recvfrom(sockfd, &ack, 2, 0, &addr, &addrlen))==-1)                                   //receive the ack
	// {
	// 	printf("error when receiving\n");
	// 	exit(1);
	// }
	if (ack.num != 1|| ack.len != 0)
		printf("error in transmission\n");
	gettimeofday(&recvt, NULL);
	*len= ci;                                                         //get current time
	tv_sub(&recvt, &sendt);                                                                 // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

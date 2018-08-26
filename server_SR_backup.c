/*-------------------------------------------------------------------- 
This file recieve server takes a command line argument.

0 : If the server donot drop packets
1 : If the server drop packets

----------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/socket.h>//for socket(), connect(), send(), recv()functions
#include <arpa/inet.h>// different address structures are declared here
#include <stdlib.h>// atoi() which convert string to integer
#include <string.h>
#include <unistd.h> // close() function
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#define BUFSIZE 512
#define PORT 8882
typedef struct msg_pack {
	int seq_no;
	int size;
	char data[BUFSIZE];
}data_packet;

struct buff{
	int status;  // -1: The buffer is empty 1: recieved
	data_packet buffered_packet;

}recv_buff[20];

typedef struct ack_pack {
	int seq_no;
}ack_packet;

void error_msg(char *s)
{
	perror(s);
	exit(1);
}



void clear_packet(data_packet * p){
	int i = 0;
	p->seq_no = -1;
	p->size = 0;
	for(i = 0; i <= BUFSIZE; i++){
		p->data[i] = '\0';
	}
}

void clear_buffer(int w){
	int i;
	for(i =0; i<=w; i++){
		recv_buff[i].status = -1;
		clear_packet(&recv_buff[i].buffered_packet);
	}
}

int check_buff(int w){  // Check how many packets o are available to be written to the file
	int i;
	for(i = 0; i<w; i++){
		if(recv_buff[i].status == -1)
			break;
	}
	return i-1;
}


void shift(int w){
	int i;
	for(i=0;i<w;i++){
		recv_buff[i] = recv_buff[i+1];
	}
		recv_buff[w-1].status = -1;

}

void sort(int w){
	struct buff temp;
	int i,j;
	for(i = 0; i < w-1; i++){
		for(j = 0; j<w-1-i; j++){
			if(recv_buff[j].buffered_packet.seq_no > recv_buff[j+1].buffered_packet.seq_no)
			{
				temp = recv_buff[j];
				recv_buff[j] = recv_buff[j+1];
				recv_buff[j+1] = temp;
			}
		}
	}
}


int main(int argc, char *argv[]) {
	// Processing command line

	int choice;
	if (argc < 2){
		printf("\nPlease enter option\nWithout packet drop : 0\nWith packet drop : 1\n");
		scanf("%d", &choice);
	}
	else
		choice = atoi(argv[1]);


	struct sockaddr_in serv_address, recv_address;  // sockaddr_in is a structure whicj=h can be typecasted to sockaddr whenever needed
	int s, i, slen = sizeof(serv_address), rlen, W, base = 0, done = 0, length;
	data_packet recv_pkt;
	ack_packet ack_pkt;
	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){  // Creating UDP socket
		error_msg("Socket Error");
	}
	memset((char *)&serv_address, 0, sizeof(serv_address)); // Zero out the structure
	serv_address.sin_family = AF_INET;
	serv_address.sin_port = htons(PORT);
	serv_address.sin_addr.s_addr = htonl(INADDR_ANY);
	// Bind socket to port

	if(bind(s, (struct sockaddr *)&serv_address, sizeof(serv_address)) == -1){
		error_msg("Bind Error");
	}


	printf("Waiting for window size from client\n");
	if((rlen = recvfrom(s, &W, sizeof(W), 0, (struct sockaddr *)&recv_address, &slen)) == -1){
		error_msg("Recieve Error");
	}
	printf("Current window size = %d \n", W);

	char filename[BUFSIZE];

	printf("Waiting for filename from client\n");
	if((rlen = recvfrom(s, filename, sizeof(filename), 0, (struct sockaddr *)&recv_address, &slen)) == -1){
		error_msg("Recieve Error");
	}
	printf("Recieved filename : %s \n ", filename);

	printf("Waiting for no of packets from client\n");
	if((rlen = recvfrom(s, &length, sizeof(length), 0, (struct sockaddr *)&recv_address, &slen)) == -1){
		error_msg("Recieve Error");
	}
	printf("\nNo of packets = %d", length);
	int p_count = 0;
	FILE *fp;
    fp = fopen(filename, "a+");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    } 
 	int j;
    clear_buffer(W);
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    }
    for(i = 0; i < W; i++){
	recv_buff[i].status = -1;
	clear_packet(&recv_buff[i].buffered_packet);
	}
    if(choice == 1){
    	printf("\n-------PACKET DROP MODE---------\n");
    	while(1){
    		clear_buffer(W);
    		int count = 0;
    		while(count<W && p_count<= length){
    			clear_packet(&recv_pkt);
    			if((rlen = recvfrom(s, &recv_pkt, sizeof(data_packet), 0, (struct sockaddr *)&recv_address, &slen)) == -1){
					error_msg("Recieve Error");
				}
				if(rand()%2){
					printf("\nDROPPING %d", recv_pkt.seq_no);
					continue;
				}
				count++;
				printf("\nRECIEVED PACKET %d : BASE %d", recv_pkt.seq_no, base);
				
				//printf("\nRecieved packet data : %s", recv_pkt.data);
				
				recv_buff[i].status = 1;
				recv_buff[i].buffered_packet  = recv_pkt;
				ack_pkt.seq_no = recv_pkt.seq_no;
				if(sendto(s, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&recv_address, slen) == -1){
					error_msg("Send to error");
				}
				printf("\nSENT ACK %d", recv_pkt.seq_no);
				p_count++;
				printf("\nPacket count = %d", p_count);
    		}
    		sort(count);
    		for(j = 0; j < W; j++){
				//printf("\nWriting content : %s", recv_buff[j].buffered_packet.data);
				//printf("\nThe sequence no written = %d",recv_buff[j].buffered_packet.seq_no );
				fwrite(recv_buff[j].buffered_packet.data, 1,recv_buff[j].buffered_packet.size,fp);
				//if(recv_buff[i].buffered_packet.size < BUFSIZE){
					//done =1;
				//}
			}
			base = recv_pkt.seq_no + 1;
			if(p_count >= length)
				break;

    	}

    }
    else{
    	printf("\n-------WITHOUT PACKET DROP MODE---------\n");
    	while(1)
    	{
    		for(i = 0;i< W && p_count <= length;i++)
    		{
    			clear_packet(&recv_pkt);
	    		if((rlen = recvfrom(s, &recv_pkt, sizeof(data_packet), 0, (struct sockaddr *)&recv_address, &slen)) == -1){
					error_msg("Recieve Error");
				}

				printf("\nRECIEVED PACKET %d : BASE %d", recv_pkt.seq_no, base);
				//printf("\nRecieved packet data : %s", recv_pkt.data);
				recv_buff[i].status = 1;
				recv_buff[i].buffered_packet  = recv_pkt;

				ack_pkt.seq_no = recv_pkt.seq_no;
				if(sendto(s, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&recv_address, slen) == -1){
					error_msg("Send to error");
				}
				printf("\nSENT ACK %d", recv_pkt.seq_no);
				p_count++;
				//printf("\nPacket count = %d", p_count);
				if(p_count>length)
					break;

			}
			
			for(j = 0; j < i; j++){
				//printf("\nWriting content : %s", recv_buff[j].buffered_packet.data);
				int status = fwrite(recv_buff[j].buffered_packet.data, 1,recv_buff[j].buffered_packet.size,fp);
				//printf("\nWrite status : %d", status);
				//if(recv_buff[i].buffered_packet.size < BUFSIZE){
					//done =1;
				//}
			}
			clear_buffer(W);
			base = recv_pkt.seq_no + 1;
			if(p_count >= length)
				break;

    	}
    	printf("\nSENT ACK %d", recv_pkt.seq_no);
    	fclose(fp);
    }
	

} 	
/*********************************************************************************************
Name : Nisha Merin Jacob
ID : 2018H1030129P


SERVER PROGRAM

Inputs:

Takes input in 2 different ways
1. If command line arguments are given:
	0 for no packet drop 
 	1 for packet drop

2.If command line argument not passed, menu will be displayed
	0 for no packet drop 
 	1 for packet drop

3.Packet Drop Rate if mode 1 is chosen

Description: Recieves packets, shifting the window whenever the acknowledgement for the base packet is sent

*********************************************************************************************/
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
#define BUFSIZE 128
#define PORT 8882

// PACKET STRUCTURE
typedef struct msg_pack {
	int seq_no;
	int size;
	char data[BUFSIZE];
}data_packet;


// KEEPS TRACK OF THE BUFFER
struct buff{
	int status;  // -1: The buffer is empty 1: recieved
	data_packet buffered_packet;

}recv_buff[20];


//ACKNOWLEDGEMENT PACKET STRUCTURE
typedef struct ack_pack {
	int seq_no;
}ack_packet;

void error_msg(char *s)
{
	perror(s);
	exit(1);
}


//CLEARS A PACKET STRUCTURE
void clear_packet(data_packet * p){
	int i = 0;
	p->seq_no = 9999;
	p->size = 0;
	for(i = 0; i <= BUFSIZE; i++){
		p->data[i] = '\0';
	}
}


//CLEARS THE ENTIRE BUFFER
void clear_buffer(int w){
	int i;
	for(i =0; i<=w; i++){
		recv_buff[i].status = -1;
		clear_packet(&recv_buff[i].buffered_packet);
	}
}

// SHIFTS THE BUFFER 1 POSITION
void shift(int w){
	int i = 0;
	for(i = 0; i < w-1; i++){
		recv_buff[i].status = recv_buff[i+1].status;
		recv_buff[i].buffered_packet = recv_buff[i+1].buffered_packet;
	} 


	recv_buff[w-1].status = -1;
	clear_packet(&(recv_buff[w-1].buffered_packet));
	

}

//SORTING ALGORITHM WHICH RETURNS THE FIRST OUT OF ORDER PACKET TO THE CALLING FUNCTION AFTER CALLING
//AND SORTS THE BUFFER

int sort(int w){
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
	for(i=0; i<w; i++){
		if(recv_buff[i].buffered_packet.seq_no + 1 != recv_buff[i+1].buffered_packet.seq_no)
			break;
	}
	return i+1;
}

/*********************************************************************************************

MAIN FUNCTION

*********************************************************************************************/

int main(int argc, char *argv[]) {
	// Processing command line

	int choice;
	float pdr;
	if (argc < 2){
		printf("\nPlease enter option\nWithout packet drop : 0\nWith packet drop : 1\n");
		scanf("%d", &choice);
	}
	else
		choice = atoi(argv[1]);


    if(choice == 1){
    	printf("\n-------PACKET DROP MODE---------\n");
    	printf("\nPlease enter Packet Drop Rate(0.0-1.0):  ");
    	scanf("%f", &pdr);
    }
    else{
    	printf("\n-------WITHOUT PACKET DROP MODE---------\n");
    	
    }


/*-----------------------------------------------------
INITIALIZATION
-----------------------------------------------------*/

	struct sockaddr_in serv_address, recv_address;  // sockaddr_in is a structure which can be typecasted to sockaddr whenever needed
	int s, i, slen = sizeof(serv_address), rlen, W, base = 0, done = 0, length, p_count = 0, j, count = 0, shift_count = 0;
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



	// FILE OPENING
	
	FILE *fp;
    fp = fopen(filename, "a");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    } 
 	


	clear_buffer(W);


/*---------------------------------------------------------------------------------------
RECIEVING PACKETS
----------------------------------------------------------------------------------------*/

    	while(1){
    		
    		count = count - shift_count;
    		
    		while(count<W && p_count<= length){

    			clear_packet(&recv_pkt);
    			if((rlen = recvfrom(s, &recv_pkt, sizeof(data_packet), 0, (struct sockaddr *)&recv_address, &slen)) == -1){
					error_msg("Recieve Error");
				}
				if(recv_pkt.seq_no > length)
					continue;
				//if(rand()%2 && choice == 1){
				if((drand48()>pdr?0:1) && choice == 1){        //EMULATING PACKET DROP
					printf("\nDROPPING %d", recv_pkt.seq_no);
					continue;
				}
				printf("\nRECIEVED PACKET %d : BASE %d", recv_pkt.seq_no, base);
				
				recv_buff[count].status = 1;
				recv_buff[count].buffered_packet  = recv_pkt;
				count++;
				ack_pkt.seq_no = recv_pkt.seq_no;
				if(sendto(s, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&recv_address, slen) == -1){
					error_msg("Send to error");
				}
				p_count++;
				
				//printf("\nPacket count = %d", p_count);
				printf("\nSENT ACK %d", recv_pkt.seq_no);
				if(recv_pkt.seq_no == base )
					goto l1;
				
    		}
    		l1:
    		
    		shift_count = 0;
    		shift_count = sort(count);
    		
    		
    		for(j = 0; j < shift_count; j++){
				
				printf("\nWriting packet with seq no : %d to file  ",recv_buff[j].buffered_packet.seq_no);
				fwrite(recv_buff[j].buffered_packet.data, 1,recv_buff[j].buffered_packet.size,fp);
				base++;
				
			}
			for(i = 0; i<shift_count;i++){
				shift(W);
			}
			if(p_count > length)
				break;

    	}


} 	

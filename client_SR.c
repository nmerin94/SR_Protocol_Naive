/*********************************************************************************************
Name : Nisha Merin Jacob
ID : 2018H1030129P


CLIENT PROGRAM

Inputs:
1. Window Size
2. Output File Name

Description: Sends packets, shifting the window whenever the acknowledgement for the base packet is recieved

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
#define TIMEOUT 2

// PACKET STRUCTURE
typedef struct msg_pack {   
	int seq_no;
	int size;
	char data[BUFSIZE];
}data_packet;


// KEEPS TRACK OF THE BUFFER
struct buff{
	int status;  // -1: The buffer is empty, 0: Ack not recieved 1: Ack recieved
	data_packet buffered_packet;

}send_buff[20];


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


// SHIFTS THE BUFFER 1 POSITION
void shift(int w){
	int i = 0;
	for(i = 0; i < w-1; i++){
		send_buff[i] = send_buff[i+1];
	} 


	send_buff[w-1].status = -1;


}

/*********************************************************************************************

MAIN FUNCTION

*********************************************************************************************/

int main(){

/*-----------------------------------------------------
INITIALIZATION
-----------------------------------------------------*/
	struct sockaddr_in serv_address;
	data_packet snd_pkt;
	ack_packet rcv_ack;

	int s, state, slen = sizeof(serv_address), W, i,shift_count;
	char buffer[BUFSIZE], message[BUFSIZE];

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 1){
		error_msg("Socket Error");
	}


	//server address setting
	memset((char *) &serv_address, 0, sizeof(serv_address)); 
	serv_address.sin_family = AF_INET;
	serv_address.sin_port = htons(PORT);						
	serv_address.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	
	
	// For select system call
	fd_set readfds; 
	fcntl(s, F_SETFL, O_NONBLOCK); 
	struct timeval tv;



	printf("\nEnter the window size W :");
	scanf("%d", &W);
	shift_count = W;
	if(sendto(s, &W, sizeof(W), 0, (struct sockaddr *)&serv_address, slen) == -1){
		error_msg("sendto()");
	}

	char filename[BUFSIZE];

	printf("\nEnter the filename to be recieved ");
	scanf("%s", filename);

	if(sendto(s, filename, strlen(filename), 0, (struct sockaddr *)&serv_address, slen) == -1){
		error_msg("sendto()");
	}

	FILE *fp = fopen("in.txt","rb");


	if(fp == NULL){
		printf("\n File %s doesnt exist\n", filename);
		exit(1);
	}

	size_t pos = ftell(fp);    // Current position
 	fseek(fp, 0, SEEK_END);    // Go to end
 	int length = ftell(fp); // read the position which is the size
 	fseek(fp, pos, SEEK_SET);  // restore original position

 	if(length%BUFSIZE == 0)
	 	length = (length/BUFSIZE);
	else
		length = (length/BUFSIZE)+1;

 	printf("\nlength = %d",length);
 	if(sendto(s, &length, sizeof(length), 0, (struct sockaddr *)&serv_address, slen) == -1){
		error_msg("sendto()");
	}

	printf("\n...Sending file...\n");

	// Clearing the send buffer
	for(i = 0; i <= W; i++){
		send_buff[i].status = -1;
		clear_packet(&send_buff[i].buffered_packet);
	}

	int seq = 0;
	int base = seq;
	data_packet p;
	int p_count = 0, finish = 0;
	p.seq_no = 0;
	

/*********************************************
SENDING PACKETS
**********************************************/

	while(1){ 


		// sending buffer size of packets
		for(i = 0; i<shift_count && p_count <= length && p.seq_no<=length; i++){
				clear_packet(&p);

				p.seq_no = seq;

				p.size = fread(p.data, 1, BUFSIZE, fp);
				

				send_buff[seq - base].status = 0;  //ack not recieved
				send_buff[seq - base].buffered_packet = p;

				if(sendto(s, &p, sizeof(data_packet), 0, (struct sockaddr *)&serv_address, slen) == -1) {
					error_msg("sendto()");
				}
				else{
					seq ++;
					if(seq > length)
						break;
					printf("\nSENT PACKET %d : BASE %d",p.seq_no, base);
					//printf("\nContent is%s\n", p.data);
					if(p.size <BUFSIZE)
						break;
				}
		}

		// Starting timer after the last packet in the buffer

		FD_ZERO(&readfds);
        FD_SET(s, &readfds);

        tv.tv_sec = TIMEOUT;  //Timeout
        tv.tv_usec = 0;
        int rv = select(s + 1, &readfds, NULL, NULL, &tv);

        // waits until a response or a timer expire
        shift_count = 0;
		if(rv > 0){   // Check if one or more acks are recieved
				recvfrom(s, &rcv_ack, sizeof(ack_packet), 0, (struct sockaddr *)&serv_address, &slen);
				p_count ++;
				for(i = 0; i<W ;i++){
					if(rcv_ack.seq_no == send_buff[i].buffered_packet.seq_no) {
						send_buff[i].status = 1;
						printf("\nRECIEVED ACK %d : BASE : %d", rcv_ack.seq_no, base);
						
						
					}
				}
				
				if(rcv_ack.seq_no == base){
				
					for(i = 0; i<W ;i++){
						
						if(send_buff[i].status == 1)
							shift_count++;
						else
							break;

					}
					
					for(i = 0; i<shift_count ;i++){
						shift(W);
						base++;
					}

				}

		}

		// case when no acknowledgement for the base packet

		else if(rv == 0){

			//resend the first packet in the window and start timer again 
			
			printf("\nTIMEOUT : %d", send_buff[0].buffered_packet.seq_no);
			if(sendto(s, &send_buff[0].buffered_packet, sizeof(data_packet), 0, (struct sockaddr *)&serv_address, slen) == -1) {
				error_msg("sendto()");
			}
			else{

				printf("\nRESENT PACKET %d : BASE %d",send_buff[0].buffered_packet.seq_no, base);
				continue;
			}
		}
		if(base>length || p_count > length){  // EDITED
			break;
		}
	}
	fclose(fp);
}
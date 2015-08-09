#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include"my402list.h"
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#include<signal.h>


int tkn_count;		// Current No. of Tokens
int B=10;		// Bucket Size
int P=3;		// No. of Tokens required per packet in deterministic mode
double r=1.5;		// Token arrival rate
double lambda=1;	// 1/lambda is packet inter arrival time
double mu=0.35;		// 1/mu is service time per packet
int num=20; 		// Total No. of Packets
char filename[50];	// trace filename
int run_mode=0;		// 0: Deterministic mode; 1: Tracefile mode
int token_global;	// Total number of tokens generated in the system
int packet_global;	// Total number of packets generated in the system
int shutdown=FALSE;	// Global flag set after receiving signal

int packet_exited=FALSE;
int token_exited=FALSE;
int server1_exited=FALSE;
int server2_exited=FALSE;
//Statistics Variables
int dropped_tokens;
int dropped_packets;
int q1_packets;
int q2_packets;
int s1_packets;
int s2_packets;
//New stats variables
double total_packet_IAT;
double total_emulation_time;
double total_packet_ST1;
double total_packet_ST2;
double total_packet_Q1;
double total_packet_Q2;
double packet_time_in_sys;
double packet_time_in_sys_squared;
//double *packet_sd;
int ind=0;
struct timespec tstart={0,0}, tend={0,0};

My402List *q1,*q2;
void print_stats();

sigset_t set;

typedef struct PacketData{
int pkt_num;
int notr;//No.of tokens reqd.
double pkt_arr_time;
double pkt_IAT;	//Packet inter arrival time
double pkt_ST;	//Packet service time
double pkt_q1_time;//time packet enters Q1
double pkt_q2_time;//time packet enters Q2
}PacketData;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void print_current_timestamp(){
clock_gettime(CLOCK_MONOTONIC, &tend);

printf("%012.3fms: ",
           (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1000);

}

double return_current_timestamp(){
clock_gettime(CLOCK_MONOTONIC, &tend);
return (( ( (double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
           ( (double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1000);
}

void* control_c_handler(void *arg) {
int sig;
//printf("waiting for sigint\n");
sigwait(&set,&sig);
shutdown=TRUE;
pthread_mutex_lock(&m);
//printf("broadcast cv\n");
pthread_cond_broadcast(&cv);
pthread_mutex_unlock(&m);
//printf("sending sigint %d\n",sig);
//print_current_timestamp();
//printf(" emulation ends\n");
printf("\n");
pthread_mutex_lock(&m);
while(!My402ListEmpty(q1)){
	My402ListElem *elem=My402ListFirst(q1);
	if(elem!=NULL){
		print_current_timestamp();
		printf("p%d removed from Q1\n",((PacketData*)elem->obj)->pkt_num);
		My402ListUnlink(q1,elem);
	}
}
while(!My402ListEmpty(q2)){
	My402ListElem *elem=My402ListFirst(q2);
	if(elem!=NULL){
		print_current_timestamp();
		printf("p%d removed from Q2\n",((PacketData*)elem->obj)->pkt_num);
		My402ListUnlink(q2,elem);
	}
}
pthread_mutex_unlock(&m);

//print_stats();
return 0;
}

void* create_server1(void *arg){
int pkt_num;
double q1_ST;
double service_time_start=0.0,service_time_end=0.0,pkt_arr_time=0.0;
while(1){
	pkt_num=0;
	My402ListElem *elem=NULL;
	pthread_mutex_lock(&m);
	while(My402ListEmpty(q2) && !shutdown && !token_exited){//Guard
 		pthread_cond_wait(&cv,&m);	
	}
	if(shutdown){
		pthread_mutex_unlock(&m);
		//printf("Exiting server1\n");
		return 0;
	}
	//printf("1.token_exited=%d,packet_exited=%d,list length=%d",token_exited,packet_exited,My402ListLength(q2));
	if(token_exited && packet_exited && My402ListEmpty(q2)){
	server1_exited=TRUE;
	//printf("server1 exited\n");
	pthread_mutex_unlock(&m);
	return 0;
	}

	service_time_start=return_current_timestamp();
	elem = My402ListFirst(q2);
	q1_ST=((PacketData*)elem->obj)->pkt_ST;
	pkt_num = ((PacketData*)elem->obj)->pkt_num;
	pkt_arr_time=((PacketData*)elem->obj)->pkt_arr_time;
	print_current_timestamp();
	double y=return_current_timestamp();
	printf("p%d leaves Q2, time in Q2 = %.3fms\n",((PacketData*)elem->obj)->pkt_num,y-((PacketData*)elem->obj)->pkt_q2_time);
	total_packet_Q2+=y-((PacketData*)elem->obj)->pkt_q2_time;
	My402ListUnlink(q2,elem);
	pthread_mutex_unlock(&m);
	if(pkt_num){
		pthread_mutex_lock(&m);
		print_current_timestamp();
		printf("p%d begins service at S1, requesting %gms of service\n",pkt_num,q1_ST);
		s1_packets++;
		pthread_mutex_unlock(&m);
		usleep( q1_ST * 1000);
		service_time_end=return_current_timestamp();
		double x=return_current_timestamp();
		pthread_mutex_lock(&m);
		print_current_timestamp();
		printf("p%d departs from S1, service time = %.3fms, time in system = %.3fms\n",pkt_num,service_time_end-service_time_start,x-pkt_arr_time);
		pthread_mutex_unlock(&m);
		total_packet_ST1+=service_time_end-service_time_start;
		packet_time_in_sys+=x-pkt_arr_time;
		packet_time_in_sys_squared+=(x-pkt_arr_time)*(x-pkt_arr_time);
		//packet_sd[ind++]=x-pkt_arr_time;
		//printf("Added to array %g, current packet_time in sys=%g\n",packet_sd[ind-1],packet_time_in_sys);
	}
	if(shutdown){
		//printf("Exiting server1\n");
		return 0;
	
	}
	pthread_mutex_lock(&m);
	if(token_exited && packet_exited && My402ListEmpty(q2)){
		
		server1_exited=TRUE;
		//printf("server1 exited\n");
		pthread_mutex_unlock(&m);
		return 0;
	}
	pthread_mutex_unlock(&m);
}//end while
}

void* create_server2(void *arg){
int pkt_num;
double q2_ST;
double service_time_start=0.0,service_time_end=0.0,pkt_arr_time=0.0;
while(1){
	pkt_num=0;
	My402ListElem *elem=NULL;
	pthread_mutex_lock(&m);
	while(My402ListEmpty(q2) && !shutdown && !token_exited){//Guard
 		pthread_cond_wait(&cv,&m);	
	}
	if(shutdown){
		pthread_mutex_unlock(&m);
		//printf("Exiting server2\n");
		return 0;
	}
	//printf("2.token_exited=%d,packet_exited=%d,list length=%d",token_exited,packet_exited,My402ListLength(q2));
	if(token_exited && packet_exited && My402ListEmpty(q2)){
	server2_exited=TRUE;
	//printf("server2 exited\n");
	pthread_mutex_unlock(&m);
	return 0;
	}

	service_time_start=return_current_timestamp();
	elem = My402ListFirst(q2);
	q2_ST=((PacketData*)elem->obj)->pkt_ST;
	pkt_num = ((PacketData*)elem->obj)->pkt_num;
	pkt_arr_time=((PacketData*)elem->obj)->pkt_arr_time;
	print_current_timestamp();
	double y=return_current_timestamp();
	printf("p%d leaves Q2, time in Q2 = %.3fms\n",((PacketData*)elem->obj)->pkt_num,y-((PacketData*)elem->obj)->pkt_q2_time);
	total_packet_Q2+=y-((PacketData*)elem->obj)->pkt_q2_time;
	My402ListUnlink(q2,elem);
	pthread_mutex_unlock(&m);
	if(pkt_num){
		pthread_mutex_lock(&m);
		print_current_timestamp();
		printf("p%d begins service at S2, requesting %gms of service\n",pkt_num,q2_ST);
		s2_packets++;
		pthread_mutex_unlock(&m);
		usleep( q2_ST * 1000);
		service_time_end=return_current_timestamp();
		double x=return_current_timestamp();
		pthread_mutex_lock(&m);
		print_current_timestamp();
		printf("p%d departs from S2, service time = %.3fms, time in system = %.3fms\n",pkt_num,service_time_end-service_time_start,x-pkt_arr_time);
		pthread_mutex_unlock(&m);		
		total_packet_ST2+=service_time_end-service_time_start;
		packet_time_in_sys+=x-pkt_arr_time;
		packet_time_in_sys_squared+=(x-pkt_arr_time)*(x-pkt_arr_time);
		//packet_sd[ind++]=x-pkt_arr_time;
		//printf("Added to array %g, current packet_time in sys=%g\n",packet_sd[ind-1],packet_time_in_sys);
	}
	if(shutdown){
		//printf("Exiting server2\n");
		return 0;
	}
	pthread_mutex_lock(&m);
	if(token_exited && packet_exited && My402ListEmpty(q2)){
	server2_exited=TRUE;
	//printf("server2 exited\n");
	pthread_mutex_unlock(&m);
	return 0;
	}
	pthread_mutex_unlock(&m);

}//end while
}


void* create_tokens(void *arg){
while(1)
{
	if(shutdown){
		printf("Exiting token1\n");
		return 0;
	}
	if((1/r)>10.0){
		usleep(10*1000000);//sleep for 10 seconds
	}else{
		usleep((1/r)*1000000);
	}
	if(shutdown){
		//printf("Exiting token2\n");
		return 0;
	}
	pthread_mutex_lock(&m);
	++token_global;
	if(tkn_count<B){
		tkn_count++;
		print_current_timestamp();
		
		printf("token t%d arrives, token bucket now has %d ",token_global,tkn_count);
		tkn_count==1?printf("token\n"):printf("tokens\n");

	}else{
		dropped_tokens++;
		print_current_timestamp();
	
		printf("token t%d arrives, dropped\n",token_global);
	
	}

	if(!My402ListEmpty(q1)){
		My402ListElem *elem = My402ListFirst(q1);
		if(tkn_count >= ((PacketData*)elem->obj)->notr){
			tkn_count-=((PacketData*)elem->obj)->notr;
			double x=return_current_timestamp();
			print_current_timestamp();
			printf("p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n",\
			((PacketData*)elem->obj)->pkt_num,x-((PacketData*)elem->obj)->pkt_q1_time,tkn_count);
			total_packet_Q1+=x-((PacketData*)elem->obj)->pkt_q1_time;		
			((PacketData*)elem->obj)->pkt_q2_time=return_current_timestamp();
			My402ListAppend(q2,(void*)elem->obj);
			print_current_timestamp();
			printf("p%d enters Q2\n",((PacketData*)elem->obj)->pkt_num);
			q2_packets++;
			//printf("Send signal cv from token\n");		
			pthread_cond_broadcast(&cv);		
			My402ListUnlink(q1,elem);
		
		}
	}
	if(My402ListEmpty(q1) && packet_exited==TRUE)
	{
	//printf("Exit token\n");
	token_exited=TRUE;
	pthread_cond_broadcast(&cv);
	pthread_mutex_unlock(&m);
	return 0;
	}
	pthread_mutex_unlock(&m);
}//end while
}




void* create_packets(void *arg){
FILE *fp=NULL;
fp=fopen(filename,"r");
//printf("Inside create packet \n");
char *tabptr,*startptr;
char buf[50];
double prev_packet_arr_time=0.0;
int first_line_flag=1;
int packet_cntr=num;

//TODO: If deterministic mode set packet_cntr=1 always

//while(packet_cntr){
while(packet_cntr){

	if(fp!=NULL){
		fgets(buf,sizeof(buf),fp);
	}

//printf("%s\n",buf);
	if(first_line_flag && run_mode){
		packet_cntr=atoi(buf);
		//printf("Total packets=%d\n",num);
		first_line_flag=0;
		//pthread_mutex_unlock(&m);
		continue;
	}
	//printf("Counter=%d ",packet_cntr);
	PacketData *pkt_data=malloc(sizeof(PacketData));



	pkt_data->pkt_num=++packet_global;
	if(run_mode){//Tracefile mode

		tabptr=startptr=buf;
		while(*tabptr == ' ' || *tabptr == '\t')
		{
			tabptr++;
		}
		startptr=tabptr;
		while(*tabptr >= '0' && *tabptr <= '9')
		{
			tabptr++;
		}
		*tabptr='\0';
		//printf("string1=%s\n",startptr);
		pkt_data->pkt_IAT=atoi(startptr);
		tabptr++;
		//startptr=tabptr;
		while(*tabptr == ' ' || *tabptr == '\t')
		{
			tabptr++;
		}
		startptr=tabptr;
		while(*tabptr >= '0' && *tabptr <= '9')
		{
			tabptr++;
		}
		*tabptr='\0';
		//printf("string2=%s\n",startptr);
		pkt_data->notr=atoi(startptr);
		tabptr++;
		//startptr=tabptr;
		while(*tabptr == ' ' || *tabptr == '\t')
		{
			tabptr++;
		}
		startptr=tabptr;
		while(*tabptr >= '0' && *tabptr <= '9')
		{
			tabptr++;
		}
		//*tabptr='\0';
		//printf("string3=%s\n",startptr);
		pkt_data->pkt_ST=atoi(startptr);


	}else{//Deterministic mode
		if((1/lambda)>10.0)
			lambda=0.1;
		pkt_data->pkt_IAT=(1/lambda)*1000;
		pkt_data->notr=P;
		if((1/mu)>10.0)
			mu=0.1;
		pkt_data->pkt_ST=(1/mu)*1000;
	}
	if(shutdown){
	
	//printf("Exiting packet1\n");
	return 0;
	}
	usleep(pkt_data->pkt_IAT*1000);
	if(shutdown){
	
	//printf("Exiting packet2\n");
	return 0;
	}
    
    	pthread_mutex_lock(&m);
	print_current_timestamp();
	pkt_data->pkt_arr_time = return_current_timestamp();
	printf("p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n",pkt_data->pkt_num,pkt_data->notr,pkt_data->pkt_arr_time-prev_packet_arr_time);
	total_packet_IAT+=pkt_data->pkt_arr_time-prev_packet_arr_time;
	pthread_mutex_unlock(&m);
//if packet needs more than bucket size drop it
	if(pkt_data->notr > B){
		dropped_packets++;
		pthread_mutex_lock(&m);
		print_current_timestamp();
		printf("p%d arrives, needs %d tokens, inter-arrival time = %.3fms, dropped\n",pkt_data->pkt_num,pkt_data->notr,pkt_data->pkt_arr_time-prev_packet_arr_time);
		pthread_mutex_unlock(&m);	
		prev_packet_arr_time=pkt_data->pkt_arr_time;
		packet_cntr--;
		continue;
	}
	prev_packet_arr_time=pkt_data->pkt_arr_time;
	pthread_mutex_lock(&m);
	if(!My402ListEmpty(q1)){
		pkt_data->pkt_q1_time=return_current_timestamp();
		My402ListAppend(q1,(void*)pkt_data);
		q1_packets++;
		print_current_timestamp();
		printf("p%d enters Q1\n",pkt_data->pkt_num);
	}else{
		if(tkn_count>=pkt_data->notr){
			pkt_data->pkt_q1_time=return_current_timestamp();
			My402ListAppend(q1,(void*)pkt_data);
			q1_packets++;
			print_current_timestamp();
			printf("p%d enters Q1\n",pkt_data->pkt_num);
			tkn_count-=pkt_data->notr;
			My402ListElem *elem=My402ListFirst(q1);
			My402ListUnlink(q1,elem);
			double x=return_current_timestamp();
			print_current_timestamp();
			printf("p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n",pkt_data->pkt_num,x-pkt_data->pkt_q1_time,tkn_count);
			total_packet_Q1+=x-pkt_data->pkt_q1_time;
			pkt_data->pkt_q2_time=return_current_timestamp();
			My402ListAppend(q2,(void*)pkt_data);
			q2_packets++;
			print_current_timestamp();
			printf("p%d enters Q2\n",pkt_data->pkt_num);
			//printf("Send signal cv from packet\n");		
			pthread_cond_broadcast(&cv);
		}else{
			pkt_data->pkt_q1_time=return_current_timestamp();
			My402ListAppend(q1,(void*)pkt_data);
			print_current_timestamp();
			printf("p%d enters Q1\n",pkt_data->pkt_num);
			q1_packets++;
		}
	
	}
	pthread_mutex_unlock(&m);



	packet_cntr--;

}//end while packet_cntr
pthread_mutex_lock(&m);
packet_exited=TRUE;
//printf("packet exited\n");
pthread_mutex_unlock(&m);
return 0;
}

void print_usage(){
	fprintf(stderr,"Error: malformed command!\n");
	fprintf(stderr,"Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
	exit(1);
}

void parseCLI(int argc,char **argv){
int i=1;
int option=0;
//printf("Inparse\n");
//printf("argc=%d\n",argc);
if(((argc-1)%2) !=0){
	
	print_usage();
	
	}
while(i<argc){
	option=0;
	//printf("\ni=%d \n",i);
		if(strcmp(argv[i],"-lambda")==0){
		option=1;
		double dval=(double)0;
		if (sscanf(argv[i+1], "%lf", &dval) != 1) {
        		print_usage();
    		} else {
        	lambda=dval;
    		}
		//lambda=atof(argv[i+1]);
		if(lambda<0)
			print_usage();
		//printf("lambda=%f\n",lambda);
		}
	if(strcmp(argv[i],"-mu")==0){
		option=1;
		double dval=(double)0;
		if (sscanf(argv[i+1], "%lf", &dval) != 1) {
        		print_usage();
    		} else {
        	mu=dval;
    		}
		//mu=atof(argv[i+1]);
		if(mu<0)
			print_usage();
		//printf("mu=%f\n",mu);
		}
	if(strcmp(argv[i],"-r")==0){
		option=1;
		double dval=(double)0;
		if (sscanf(argv[i+1], "%lf", &dval) != 1) {
        		print_usage();
    		} else {
        	r=dval;
    		}
		//r=atof(argv[i+1]);
		if(r<0)
			print_usage();
		//printf("r=%f\n",r);
		}
	if(strcmp(argv[i],"-B")==0){
		option=1;
		B=atoi(argv[i+1]);
		if(B<0)
			print_usage();
		//printf("B=%d\n",B);
		}
	if(strcmp(argv[i],"-P")==0){
		option=1;
		P=atoi(argv[i+1]);
		if(P<0)
			print_usage();
		//printf("P=%d\n",P);
		}
	if(strcmp(argv[i],"-n")==0){
		option=1;
		num=atoi(argv[i+1]);
		if(num>2147483647){
			fprintf(stderr,"Integer Limit exceeded for -n\n");
			print_usage();
		}
		if(num<0)
			print_usage();
		//printf("n=%d\n",num);
		}
	if(strcmp(argv[i],"-t")==0){
		option=1;
		strcpy(filename,argv[i+1]);
		FILE *test=fopen(filename,"r");
		if(test==NULL){
			fprintf(stderr,"Error in file open!\n");
			exit(1);	
		}
		run_mode=1;
		//printf("filename=%s\n",filename);
	}
	i+=2;
	if(argc>1 && option==0){
		print_usage();
	}
}//end while

}


void print_stats()
{
//int i=0;
double avg=0,avg2=0;
//,numerator=0;
	printf("\nStatistics:\n\n");
	printf("    average packet inter-arrival time = %.6g\n",(total_packet_IAT/total_emulation_time));
	printf("    average packet service time = %.6g\n\n",((total_packet_ST1+total_packet_ST2)/total_emulation_time));
	printf("    average number of packets in Q1 = %.6g\n",(total_packet_Q1/total_emulation_time));
	printf("    average number of packets in Q2 = %.6g\n",(total_packet_Q2/total_emulation_time));
	printf("    average number of packets at S1 = %.6g\n",(total_packet_ST1/total_emulation_time));
	printf("    average number of packets at S2 = %.6g\n\n",(total_packet_ST2/total_emulation_time));
	printf("    average time a packet spent in system = %.6g\n",(packet_time_in_sys/packet_global)/1000);
	
	avg=(packet_time_in_sys*packet_time_in_sys)/(packet_global*packet_global);
	avg2=packet_time_in_sys_squared/packet_global;
	//printf("Index=%d average=%f\n",ind,avg);
	/*	
	for(i=0;i<ind;i++)
	{
		numerator+=(packet_sd[i]-avg)*(packet_sd[i]-avg);
	}*/
	//printf("numerator=%g, num/ind=%g",numerator,numerator/ind);
	//if(ind==0)
	//printf("    standard deviation for time spent in system = 0\n");
	//else
	printf("    standard deviation for time spent in system = %.6g\n\n",sqrt(avg2-avg)/1000);
	if(token_global==0)
	printf("    token drop probability = N/A : no token arrived at this facility\n");
	else
	printf("    token drop probability = %.6g\n",(double)dropped_tokens/token_global);
	if(packet_global==0)	
	printf("    packet drop probability = N/A : no packet arrived at this facility\n");
	else
	printf("    packet drop probability = %.6g\n",(double)dropped_packets/packet_global);
}


int main(int argc, char **argv)
{
pthread_t pkt_id,tkn_id,s1_id,s2_id,ctrlc_id;

parseCLI(argc,argv);


if(run_mode){
FILE *fp=fopen(filename,"r");
	if(fp!=NULL){
		char buf[50];
		fgets(buf,sizeof(buf),fp);
		num=atoi(buf);
		if(num==0){
			fprintf(stderr,"Error: input file is not in the right format!\n");
			fclose(fp);
			exit(1);
		}fclose(fp);
	}
}
printf("Emulation Parameters:\n");
printf("    number to arrive = %d\n",num);
if(!run_mode){
	if(lambda<0.1)
	lambda=0.1;
	printf("    lambda = %.2f\n",lambda);
}
if(!run_mode){
	if(mu<0.1)
	mu=0.1;
	printf("    mu = %.2f\n",mu);
}
if(r<0.1){
r=0.1;
}
printf("    r = %.2f\n",r);
printf("    B = %d\n",B);
if(!run_mode)
	printf("    P = %d\n",P);
if(run_mode)
	printf("    tsfile = %s\n",filename);
printf("\n");
//packet_sd=calloc(num,sizeof(double));

q1 = (My402List *)malloc(sizeof(My402List));
if( q1 == NULL){
	fprintf(stderr,"Memory Error!\n");
	exit(1);
}
q2 = (My402List *)malloc(sizeof(My402List));
if(q2 == NULL)
{
	fprintf(stderr,"Memory Error!\n");
	exit(1);
}
My402ListInit(q1);
My402ListInit(q2);

clock_gettime(CLOCK_MONOTONIC, &tstart);
printf("00000000.000ms: emulation begins\n");
//sigset(SIGINT, ctrl_c_handler);

sigemptyset(&set);
sigaddset(&set, SIGINT);
sigprocmask(SIG_BLOCK, &set, 0);
//sigset(SIGINT, control_c_handler);
if(pthread_create(&pkt_id,0,create_packets,0)){
	fprintf(stderr,"Error - pthread_create() for packets\n");
	exit(EXIT_FAILURE);
}
if(pthread_create(&tkn_id,0,create_tokens,0)){
	fprintf(stderr,"Error - pthread_create() for tokens\n");
	exit(EXIT_FAILURE);
}

if(pthread_create(&s1_id,0,create_server1,0)){
	fprintf(stderr,"Error - pthread_create() for server1\n");
	exit(EXIT_FAILURE);
}
if(pthread_create(&s2_id,0,create_server2,0)){
	fprintf(stderr,"Error - pthread_create() for server2\n");
	exit(EXIT_FAILURE);
}

if(pthread_create(&ctrlc_id,0,control_c_handler,0)){
	fprintf(stderr,"Error - pthread_create() for control_c\n");
	exit(EXIT_FAILURE);
}



pthread_join(pkt_id,0);
pthread_join(tkn_id,0);
pthread_join(s1_id,0);
pthread_join(s2_id,0);
if(server1_exited && server2_exited){
	//printf("All threads exited\n");
	pthread_cancel(ctrlc_id);
}
else
pthread_join(ctrlc_id,0);
total_emulation_time=return_current_timestamp();
print_current_timestamp();
printf("emulation ends\n");

print_stats();
return 0;
}

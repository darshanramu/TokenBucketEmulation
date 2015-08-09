/******************************************************************************/
/* This file is used to sort the list and display the transaction table.      */
/* Takes input from either stdin or file                                      */
/******************************************************************************/

/*
 * Author: Darshan V Ramu (darshanv@usc.edu)
 *:
 * @(#)$Id: my402sort.c, v1.0 2015/01/27 $
 */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include"my402list.h"

//Struct which contains the element data
typedef struct My402ListData {
char *type,*desc;
int amount,timestamp;
} My402ListData;

//Assume that we will be reading from file
int stdin_flag=FALSE;

//The following two functions are taken from Professor's listtest 
static
void BubbleForward(My402List *pList, My402ListElem **pp_elem1, My402ListElem **pp_elem2)
{
    My402ListElem *elem1=(*pp_elem1), *elem2=(*pp_elem2);
    void *obj1=elem1->obj, *obj2=elem2->obj;
    My402ListElem *elem1prev=My402ListPrev(pList, elem1);
    My402ListElem *elem2next=My402ListNext(pList, elem2);

    My402ListUnlink(pList, elem1);
    My402ListUnlink(pList, elem2);
    if (elem1prev == NULL) {
        (void)My402ListPrepend(pList, obj2);
        *pp_elem1 = My402ListFirst(pList);
    } else {
        (void)My402ListInsertAfter(pList, obj2, elem1prev);
        *pp_elem1 = My402ListNext(pList, elem1prev);
    }
    if (elem2next == NULL) {
        (void)My402ListAppend(pList, obj1);
        *pp_elem2 = My402ListLast(pList);
    } else {
        (void)My402ListInsertBefore(pList, obj1, elem2next);
        *pp_elem2 = My402ListPrev(pList, elem2next);
    }
}

void BubbleSortForwardList(My402List *pList, int num_items)
{
    My402ListElem *elem=NULL;
    int i=0;

    if (My402ListLength(pList) != num_items) {
        fprintf(stderr, "List length is not %1d in BubbleSortForwardList().\n", num_items);
        exit(1);
    }
    for (i=0; i < num_items; i++) {
        int j=0, something_swapped=FALSE;
        My402ListElem *next_elem=NULL;
	//Just changed professor's version to sort based on timestamp
        for (elem=My402ListFirst(pList), j=0; j < num_items-i-1; elem=next_elem, j++) {
            int cur_val=(int)((My402ListData *)elem->obj)->timestamp, next_val=0;

            next_elem=My402ListNext(pList, elem);
            next_val = (int)((My402ListData *)next_elem->obj)->timestamp;
	    if(cur_val == next_val){
		fprintf(stderr,"Error! Same transcation Timestamp given.\n");
		exit(1);
	    }
            if (cur_val > next_val) {
                BubbleForward(pList, &elem, &next_elem);
                something_swapped = TRUE;
            }
        }
        if (!something_swapped) break;
    }
}

void display_amtbal(char* amt,int amount, int pos){
//i: string index, j: no.of digits
int i = 12, j = 1, rem;
snprintf(amt, 15,"              ");
if(amount < 0){
	pos = 0;
	amount *= -1;
}
while(j<=10 && (amount > 0 || j < 4)){
	rem = amount % 10;
	amount /= 10;
	if(j == 3){
		amt[i--] = '.';
	}
	else if((j - 3) % 3 == 0){
	amt[i--] = ',';
	}
	amt[i--] = '0' + rem;
	j++;
}
if(j > 10){
	snprintf(amt,15," ?,???,???.?? ");
}
else{
	while(i > 0){
	amt[i--] = ' ';
	}
}
if(pos==FALSE){
	amt[0] = '(';
	amt[13] = ')';
}
amt[14]=' ';
amt[15]='\0';
}

void DisplayTable(My402List *list){
int balance=0,count=0,pos=TRUE;
int amt;
char *d,*eol,*dstr;
time_t times;
char *timestamp1=(char*)malloc(10);
char *timestamp2=(char*)malloc(4);
char *amt_str=(char*)malloc(15);
char *bal_str=(char*)malloc(15);
if(list==NULL)
	return;
count = list->num_members;
My402ListElem *elem;
elem = list->anchor.next;
printf("+-----------------+--------------------------+----------------+----------------+\n");
printf("|       Date      | Description              |         Amount |        Balance |\n");
printf("+-----------------+--------------------------+----------------+----------------+\n");

while(count){
if(strcmp(((My402ListData *)elem->obj)->type,"+") == 0){
	balance += ((My402ListData *)elem->obj)->amount;
	pos=TRUE;
}else if(strcmp(((My402ListData *)elem->obj)->type,"-") == 0){
	balance -= ((My402ListData *)elem->obj)->amount;
	pos=FALSE;
}

d=dstr=((My402ListData *)elem->obj)->desc;
eol=strchr(d,'\n');
if(eol!=NULL)
	*eol='\0';
if(strlen(dstr)>24)
	*(dstr+24)='\0';
times = ((My402ListData *)elem->obj)->timestamp;

   
amt = ((int)((My402ListData *)elem->obj)->amount);
strncpy(timestamp1,ctime(&times),10);
strncpy(timestamp2,ctime(&times)+20,4);

*(timestamp1+10)='\0';
*(timestamp2+4)='\0';

display_amtbal(amt_str,amt,pos);
if(balance<0)
	pos=FALSE;
else
	pos=TRUE;
display_amtbal(bal_str,balance,pos);
printf("| %-11s%4s | %-24s |%16s|%16s|\n",timestamp1,timestamp2,\
		((My402ListData *)elem->obj)->desc,amt_str,bal_str);
count--;
elem=elem->next;
}//end while count
printf("+-----------------+--------------------------+----------------+----------------+");
printf("\n");
if(amt_str!=NULL && bal_str!=NULL && timestamp1!=NULL && timestamp2!=NULL){
free(amt_str);free(bal_str);free(timestamp1);free(timestamp2);
}
}

void Process(char *argv[]){
//Create a DCLL
My402List *list = NULL;
list = (My402List *) malloc( sizeof(My402List) );
My402ListInit(list);
int wf=FALSE;// while flag: Did while loop enter and did we process anything : avoids junk files and consequent Display of table		
FILE *fp;
char buf[1027];
if (stdin_flag){
	fp = stdin;
	if(fp == NULL){
		fprintf(stderr,"Error opening stdin!\n");
		exit(1);
	}
}
else{
	fp = fopen(*(argv+2),"r");
	if(fp == NULL){
		fprintf(stderr,"Error opening file!\n");
		exit(1);
	}
}

while(!feof(fp)){
	if(fgets(buf,sizeof(buf),fp)!=NULL)
	{
		wf=TRUE;
		if(strlen(buf)>1024)
		{
			fprintf(stderr,"Error! Exceeded max line length.\n");
			exit(1);
		}	
		else {
			int amount_dot = 0, digit = 0,i=0;
			My402ListData *data = (My402ListData *) malloc(sizeof(My402ListData));
			int amount,times,tabcount=1;
			char *type,*desc,*str,*dotstr,*rptr,*catptr;
			char *start_ptr = buf,*tabptr;
			char *tab_ptr = strchr(start_ptr,'\t');
			time_t cur_time;
			if(tab_ptr != NULL) {
				*tab_ptr ='\0';
				tab_ptr++;
			}else if(tab_ptr==NULL){
				fprintf(stderr,"Error! Input file not in right format.\n");
				exit(1);			
			}
			type=start_ptr;
			start_ptr=tab_ptr;
			tabptr=tab_ptr;
			while((tabptr=strchr(tabptr,'\t'))!=NULL){
				tabcount++;
				tabptr++;
			}
			if(tabcount>3){
				fprintf(stderr,"Error! Invalid number of tabs.\n");
				exit(1);
			}
			
			data->type = malloc (sizeof(type));
			strcpy(data->type,type);
			if(strcmp(type,"+") != 0 && strcmp(type,"-") != 0)
			{
				fprintf(stderr,"Error! Invalid Transaction Type.\n");
				exit(1);
			}
			tab_ptr = strchr(start_ptr,'\t');
			if(tab_ptr != NULL) {
				*tab_ptr ='\0';
				tab_ptr++;
			}else if(tab_ptr==NULL){
				fprintf(stderr,"Error! Input not tabbed correctly.\n");
				exit(1);			
			}
			if(strlen(start_ptr)>10){
				fprintf(stderr,"Error! Invalid Timestamp.\n");
				exit(1);
			}
			str=start_ptr;
			//Check for Invalid Timestamp
			while(*str != '\t' && *str != '\0')
			{
				
				if(*str >= '0' && *str <= '9'){
					str++;
				}
				else {
				fprintf(stderr,"Error! Invalid Timestamp.\n");
				exit(1);
				}
			}//end while
			times = atoi(start_ptr);
			cur_time = time(NULL);
			if(times>cur_time || times<0 ||times == 0){
				fprintf(stderr,"Error! Invalid Timestamp.\n");
			exit(1);
			}
			
			data->timestamp=times;
			start_ptr=tab_ptr;
			tab_ptr = strchr(start_ptr,'\t');
			
			if(tab_ptr != NULL) {
				*tab_ptr ='\0';
				tab_ptr++;
			}else if(tab_ptr==NULL){
				fprintf(stderr,"Error! Input not tabbed correctly.\n");
				exit(1);			
			}
			if(*start_ptr=='-'){
				fprintf(stderr,"Error! Negative amount given.\n");
				exit(1);			
			}
			if(strlen(start_ptr)>10){
				fprintf(stderr,"Error! Amount exceeds the max value.\n");
				exit(1);
			}
			str=start_ptr;
			while(*str != '\t' && *str != '\0')
			{
				
				if(*str >= '0' && *str <= '9'){
					str++;i++;
				}
				else if(*str == '.')
				{
					
					if(++amount_dot > 1){
					fprintf(stderr,"Error! Invalid format of amount.\n");
					exit(1);
					}
					else // skip the '.' symbol
					{
					str++;i++;
					digit = i;
					}
				}
				else {
				fprintf(stderr,"Error! Invalid format of amount.\n");
				exit(1);
				}
			}//end while
			digit = i - digit;
			if(digit != 2){
			fprintf(stderr,"Error! Invalid format of amount.\n");
			exit(1);
			}
			str=start_ptr;	
			while(*str != '.')
				str++;
			*str='\0';
			rptr=str;	
			str++;
			dotstr=str;
			while(*str != '\t')
				str++;
			*str='\0';
		        catptr=malloc(strlen(start_ptr)+strlen(dotstr));	
		        strncpy(catptr,start_ptr,strlen(start_ptr));			
		        strncpy(catptr+strlen(start_ptr),dotstr,strlen(dotstr));
		 	*(catptr+strlen(start_ptr)+strlen(dotstr))='\0';	
			amount = atoi(catptr);
			if(catptr!=NULL)			
				free(catptr);
			if(amount == 0){
				fprintf(stderr,"Error! Invalid amount.\n");
				exit(1);
			}
		        *str='\t';*rptr='.';	
			
			data->amount=amount;
			
			while(*tab_ptr== ' ' || *tab_ptr=='\t')
				tab_ptr++;
			if(*tab_ptr == '\n'){
				fprintf(stderr,"Error! Empty Description.\n");
				exit(1);
			}
			
			desc=tab_ptr;
			data->desc= malloc(strlen(desc) * sizeof(desc));
			strcpy(data->desc,desc);
			My402ListAppend(list,(void*)data);
                        
			
		}
	}else if(!feof(fp)){
		fprintf(stderr,"Error! Invalid file!\n");
				exit(1);
	}
	
}//end while
if(wf){
//Display contents of the list as table
BubbleSortForwardList(list, list->num_members);
DisplayTable(list);
My402ListUnlinkAll(list);
}else if(wf==FALSE){ //came to while after opening but didnt read anything
		fprintf(stderr,"Error! Invalid file!\n");
				exit(1);
}
}

static
void Usage()
{
    fprintf(stderr,
            "Invalid Arguments!\nUsage: %s %s %s\n",
            "warmup1", "sort","[tfile]");
    exit(1);
}

static
void ProcessArgs(int argc, char *argv[])
{
	if(argc > 3 || argc == 1)
		Usage();
	if (strcmp(*(argv+1), "sort") != 0) {
		Usage();
	}
        else{
            //first arg is sort check for second arg
	    if(argc == 2)
		stdin_flag = TRUE;
	    else if(argc == 3)
		stdin_flag = FALSE;
        }
}

int main(int argc, char* argv[])
{
	ProcessArgs(argc, argv);
	Process(argv);
	return 0;
}

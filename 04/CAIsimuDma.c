#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include "WD-dask64.h"

#define TIMEBASE      WD_IntTimeBase
#define ADTRIGSRC     WD_AI_TRGSRC_SOFT
#define ADTRIGMODE    WD_AI_TRGMOD_POST
#define ADTRIGPOL     WD_AI_TrgNegative
#define BUFAUTORESET  1
#define SCAN_INTERVAL 1

FILE *svfile;

I16 *ai_buf;

LARGE_INTEGER freq, start_count, current_count;
U32 count;
BOOLEAN fStop=0;
static HANDLE s_Event = NULL;
U16 CardType[] = {PCIe_9852, PXIe_9852};

void AI_CallBack()
{
  QueryPerformanceCounter(&current_count);
  fStop = 1;
  printf("end: %d\n",  count);
  SetEvent( s_Event );
}

void main()
{
   I16 card, err, card_num=0, Id, ai_mode=0;
   U16 trgcnt=1, chcnt=8, card_type;
   U32 startPos=0, totalcnt=0, sts=0;
   U32 i;
   int k=0;
   double take_msec;

   printf("---------------------\n");
   printf("0. PCIe-9852\n1. PXIe-9852\n");
   printf("---------------------\n");
   printf("Please select a card type: ");
   scanf(" %hd", &card_type);
   printf("Please input a card number: ");
   scanf(" %hd", &card_num);
   card_type = CardType[card_type];

   if ((card=WD_Register_Card (card_type, card_num)) <0 ) {
        printf("Register_Card error=%d", card);
        exit(1);
   }

   printf("\nPlease input scan count?");
   scanf(" %d", &totalcnt);

   printf("\nHow many channels do you wish to perform (1 ~ 8)? ");
   scanf(" %hd", &chcnt);

   if(!chcnt) chcnt=1;
   if(chcnt>2) chcnt = 2;
   
   totalcnt *= chcnt;

   ai_buf = WD_Buffer_Alloc (card, totalcnt*sizeof(I16));
   if(!ai_buf) 
   {
	   printf("buffer allocation failed\n");
	   return;
   }
   memset(ai_buf, 0, totalcnt*sizeof(I16));

   err = WD_AI_Config (card, TIMEBASE, 1, WD_AI_ADCONVSRC_TimePacer, 0, BUFAUTORESET);
   if (err!=0) {
       printf("WD_AI_Config error=%d", err);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Release_Card(card);
       exit(1);
   }
   err = WD_AI_Trig_Config (card, ADTRIGMODE, ADTRIGSRC, ADTRIGPOL, 0, 0.0, 0, 0, 0, 1);
   if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Release_Card(card);
       exit(1);
   }
   err=WD_AI_ContBufferSetup (card, ai_buf, totalcnt, &Id);
   if (err!=0) {
       printf("WD_AI_ContBufferSetup error=%d", err);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Release_Card(card);
       exit(1);
   }
   if( s_Event == NULL )
   {
		s_Event = CreateEvent( NULL, TRUE, FALSE, NULL );
   }
   ResetEvent( s_Event );   
   err = WD_AI_EventCallBack_x64 (card, 1, DAQEnd, (ULONG_PTR) AI_CallBack);
   if (err!=0) {
       printf("WD_AI_EventCallBack error=%d", err);
       WD_AI_ContBufferReset (card);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Release_Card(card);
       exit(1);
   }

   QueryPerformanceFrequency(&freq);
   QueryPerformanceCounter(&start_count); 

   err = WD_AI_ContScanChannels (card, chcnt-1, Id, totalcnt/chcnt, SCAN_INTERVAL, SCAN_INTERVAL, ASYNCH_OP);
   if (err!=0) {
       printf("AI_ContScanChannels error=%d", err);
       WD_AI_ContBufferReset (card);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Release_Card(card);
       exit(1);
   }
   if(WaitForSingleObject( s_Event, 5000 ) == WAIT_TIMEOUT )
   {
		printf("wait failed\n");
   }

   WD_AI_EventCallBack_x64 (card, 0, DAQEnd, (ULONG_PTR) NULL);
   WD_AI_AsyncClear(card, &startPos, &count);
   if(!fStop)
   {
	printf("manual clear: %d\n", count);
   }
   printf("end: %d %d\n", count, totalcnt);
   take_msec =((double)(current_count.QuadPart-start_count.QuadPart))/freq.QuadPart*1000;
   printf("The time taken for Data acquisition is: %4.3f msec(%f)\n",take_msec, ((double) (totalcnt/chcnt))*1000/take_msec);
   svfile = fopen("9852d.txt", "w");

   for(i=0;i<totalcnt;i+=k)
   {
	   F64 voltage=0.0;
	   for(k=0;k<chcnt;k++)
	   {
	    WD_AI_VoltScale (card, AD_B_2_V, (I16) (ai_buf[i+k]&0xffff), &voltage);
	    fprintf(svfile, "0x%4x(%4.4f) ",  (I16) (ai_buf[i+k]&0xffff), voltage);
	   }
	   fprintf(svfile, "\n");
   }

   fclose(svfile);
   WD_AI_ContBufferReset (card);
   WD_Buffer_Free (card, ai_buf);
   WD_Release_Card(card);

   printf("\nPress ENTER to exit the program. "); getch();
}
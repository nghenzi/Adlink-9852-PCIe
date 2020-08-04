#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "wd-dask64.h"

#define SCANCOUNT     600000
#define RETRGCNT	  0

#define TIMEBASE      WD_IntTimeBase
#define ADTRIGSRC     WD_AI_TRGSRC_SOFT
#define ADTRIGMODE    WD_AI_TRGMOD_POST
#define ADTRIGPOL     WD_AI_TrgNegative
#define BUFAUTORESET  1
#define SCAN_INTERVAL 1

I16 *ai_buf;
I16 *ai_buf2;

U16 CardType[] = {PCIe_9852, PXIe_9852};

void main()
{
   I16 card, err, card_num=0, Id;
   BOOLEAN fStop=0, fStop2=0, bReady=0;
   U32 count=0, startPos=0;
   U32 i, k, RdyTrigCnt=0;
   U16 card_type=0;

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

   ai_buf = WD_Buffer_Alloc (card, SCANCOUNT*sizeof(I16));
   if(!ai_buf) 
   {
	   printf("buffer1 allocation failed\n");
	   return;
   }
   ai_buf2 = WD_Buffer_Alloc (card, SCANCOUNT*sizeof(I16));
   if(!ai_buf2) 
   {
	   printf("buffer2 allocation failed\n");
	   WD_Buffer_Free (card, ai_buf);
	   return;
   }
   memset(ai_buf, 0, SCANCOUNT*2);
   memset(ai_buf2, 0, SCANCOUNT*2);

   err = WD_AI_Config (card, TIMEBASE, 1, WD_AI_ADCONVSRC_TimePacer, 0, BUFAUTORESET);
   if (err!=0) {
       printf("WD_AI_Config error=%d", err);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Buffer_Free (card, ai_buf2);
	   WD_Release_Card(card);
       exit(1);
   }
   err = WD_AI_Trig_Config (card, ADTRIGMODE, ADTRIGSRC, ADTRIGPOL, 0, 0.0, 0, 0, 0, RETRGCNT);
   if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Buffer_Free (card, ai_buf2);
	   WD_Release_Card(card);
       exit(1);
   }

   err=WD_AI_ContBufferSetup (card, ai_buf, SCANCOUNT, &Id);
   if (err!=0) {
       printf("P9842_AI_ContBufferSetup 1 error=%d", err);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Buffer_Free (card, ai_buf2);
	   WD_Release_Card(card);
       exit(1);
   }
   err=WD_AI_ContBufferSetup (card, ai_buf2, SCANCOUNT, &Id);
   if (err!=0) {
       printf("P9842_AI_ContBufferSetup 2 error=%d", err);
	   WD_AI_ContBufferReset (card);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Buffer_Free (card, ai_buf2);
	   WD_Release_Card(card);
       exit(1);
   }
   err = WD_AI_ContReadChannel (card, 0, 0, SCANCOUNT, SCAN_INTERVAL, SCAN_INTERVAL, ASYNCH_OP);
   if (err!=0) {
       printf("WD_AI_ContReadChannel error=%d", err);
	   WD_AI_ContBufferReset (card);
	   WD_Buffer_Free (card, ai_buf);
	   WD_Buffer_Free (card, ai_buf2);
	   err = WD_Release_Card(card);
       exit(1);
   }
   i=0;
   k=0;
   do {
    i = 0;
    do {
		WD_AI_AsyncReTrigNextReady (card, &bReady, &fStop, &RdyTrigCnt);
		if(i>=1000000)
		{
			printf("timeout\n");
			break;
		}
		i++;		
    } while(!bReady);
	if(i>=1000000)
	{
			printf("timeout\n");
			break;
	} else {
		k++;
		printf("(%d) ready cnt: %d\n", k, RdyTrigCnt);
		Sleep(100);
		WD_SoftTriggerGen(card, SOFTTRIG_AI);
	}
   }while(!kbhit());
    WD_AI_AsyncClear(card, &startPos, &count);
    WD_AI_ContBufferReset (card);
    WD_Buffer_Free (card, ai_buf);
	WD_Buffer_Free (card, ai_buf2);
   err = WD_Release_Card(card);
   printf("\nPress ENTER to exit the program. "); getch();
}
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "WD-dask.h"
#include "wddaskex.h"

#define CHANNELNUMBER All_Channels
#define MAXCHANNELCOUNT 8
#define SCANCOUNT     102400
#define TIMEBASE      WD_IntTimeBase
#define ADTRIGSRC     WD_AI_TRGSRC_SOFT
#define ADTRIGMODE    WD_AI_TRGMOD_POST
#define ADTRIGPOL     WD_AI_TrgNegative
#define BUFAUTORESET  1
#define SCAN_INTERVAL 1
#define CNT_IN_EACH_BUF SCANCOUNT*MAXCHANNELCOUNT/2
I16 ai_buf[CNT_IN_EACH_BUF];
void show_channel_data(U16 *buf);

char *file_name="aidma.txt";
FILE *svfile;
U32 numRead = 0, scanBacklog = 0;
DAS_IOT_DEV_PROP cardProp;
U16 chcnt=2, ai_range=0;
I16 card = 0;

void main()
{
    I16 err, card_num, Id, i;
    BOOLEAN fStop=0;
	U32 count=0, startPos=0, sdramsize=0;
	U8 complete = 0;
	U16 card_type = 1, parts=2;

    printf("This program inputs %d scans from all channels.\n", SCANCOUNT);
	card_type = WD_ChooseDeviceType(0);
	printf("Please input a card number: ");
    scanf(" %hd", &card_num);
    if ((card=WD_Register_Card (card_type, card_num)) <0 ) {
        printf("Register_Card error=%d", card);
        exit(1);
    }
	WD_GetDeviceProperties (card, 0, &cardProp);
	chcnt = cardProp.num_of_channel;
	ai_range = cardProp.default_range;

	err = WD_AI_CH_Config (card, All_Channels, ai_range);
    if (err!=NoError) {
       printf("WD_AI_CH_Config error=%d", err);
       exit(1);
    }
	err = WD_AI_Config (card, TIMEBASE, 1, WD_AI_ADCONVSRC_TimePacer, 0, BUFAUTORESET);
    if (err!=0) {
       printf("WD_AI_Config error=%d", err);
       exit(1);
    }
    err = WD_AI_Trig_Config (card, ADTRIGMODE, ADTRIGSRC, ADTRIGPOL, 0, 0.0, 0, 0, 0, 1);
    if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
       exit(1);
    }
	printf("How many parts of buffer (>=2): ");
    scanf(" %hd", &parts);
	if(parts<2) parts = 2;

	err=WD_AI_ContBufferSetup (card, ai_buf, SCANCOUNT*chcnt/parts, &Id);
	// Or err=WD_AI_ContBufferLock (card, ai_buf, SCANCOUNT*chcnt/2, &Id);
    if (err!=0) {
       printf("WD_AI_ContBufferSetup 0 error=%d", err);
       exit(1);
    }
	err = WD_AI_Set_Mode (card, DAQSTEPPED|DMASTEPPED, 1);
    
	svfile = fopen(file_name, "w");
	err = WD_AI_ContScanChannels (card, (chcnt-1), Id, SCANCOUNT, SCAN_INTERVAL, SCAN_INTERVAL, ASYNCH_OP);
    if (err!=0) {
       printf("AI_ContScanChannels error=%d", err);
       exit(1);
    }
    do {
		WD_AI_ConvertCheck(card, &fStop);
	} while (!fStop);
	//DAQ conversion stopped
    for(i=0;i<chcnt;i++)
		fprintf(svfile,"    Ch%d        ", i);
    fprintf(svfile,"\n");
	
	do {
		WD_AI_DMA_TransferBySize (card, 10, Id, SCANCOUNT*chcnt/parts, &numRead, &scanBacklog, &complete);
        printf("%d %d %d\n", numRead, scanBacklog, complete);
		show_channel_data(ai_buf);
	} while(!complete);
    WD_AI_AsyncClear(card, &startPos, &count);    
     fclose(svfile);
    err = WD_Release_Card(card);
    printf("\nPress ENTER to exit the program. "); getch();
}

void show_channel_data( U16 *buf)
{
  int k, j;
  U32 i, totalcnt = numRead;
  F64 vol = 0.0;

  for (k=0; k<totalcnt/80; k++) {
   for( i=0+k*80; i<80+k*80 ; i=i+chcnt ){
	   for(j=0;j<chcnt;j++)
	   {   
		   WD_AI_VoltScale (card, ai_range, ai_buf[i+j], &vol);
		   //fprintf(svfile, "0x%8x(%4.4f) ",  (I32) ai_buf[i+j], vol);
		   fprintf(svfile," %04x(%4.4fV) ", (U16) (ai_buf[i+j]&cardProp.mask), vol);
	   }
	   fprintf(svfile,"\n");
   }	   
  }
}
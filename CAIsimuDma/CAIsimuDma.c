#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "WD-dask.h"
#include "wddaskex.h"

#define CHANNELNUMBER All_Channels
#define SCANCOUNT     3200
#define MAXCHANNELCOUNT 4
#define TIMEBASE      WD_IntTimeBase
#define ADTRIGSRC     WD_AI_TRGSRC_SOFT
#define ADTRIGMODE    WD_AI_TRGMOD_POST
#define ADTRIGPOL     WD_AI_TrgNegative
#define BUFAUTORESET  1
#define SCAN_INTERVAL 1

DAS_IOT_DEV_PROP cardProp;
U16 chcnt, ai_range=0;
I16 card = 0;
U16 ai_buf[SCANCOUNT*MAXCHANNELCOUNT];
void show_channel_data(U16 *buf);

void main()
{
    I16 err, card_num, Id;
	U16 card_type = 1;

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
    //default setting : the following two functions are removed
	err = WD_AI_CH_Config (card, CHANNELNUMBER, ai_range);
    if (err!=NoError) {
       printf("WD_AI_CH_Config error=%d", err);
       exit(1);
    }
	/*err = WD_AI_Config (card, TIMEBASE, 1, WD_AI_ADCONVSRC_TimePacer, 0, BUFAUTORESET);
    if (err!=0) {
       printf("WD_AI_Config error=%d", err);
       exit(1);
    }
    err = WD_AI_Trig_Config (card, ADTRIGMODE, ADTRIGSRC, ADTRIGPOL, 0, 0.0, 0, 0, 0, 1);
    if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
       exit(1);
    }*/
	err=WD_AI_ContBufferSetup (card, ai_buf, SCANCOUNT*chcnt, &Id);
    if (err!=0) {
       printf("WD_AI_ContBufferSetup error=%d", err);
       exit(1);
    }
	WD_AI_SetTimeout (card, 5000);
	err = WD_AI_ContScanChannels (card, chcnt-1, Id, SCANCOUNT, SCAN_INTERVAL, SCAN_INTERVAL, SYNCH_OP);
    if (err!=0) {
       printf("AI_ContScanChannels error=%d", err);
	   WD_AI_ContBufferReset (card);
       WD_Release_Card(card);
       exit(1);
    }
    show_channel_data(ai_buf);
	WD_AI_ContBufferReset (card);
    WD_Release_Card(card);
    printf("\nPress ENTER to exit the program. "); getch();
}

void show_channel_data( U16 *buf)
{
  int k;
  U32 i, j;
  F64 vol = 0.0;

  for(i=0;i<chcnt;i++)
	printf("    Ch%d        ", i);
  printf("\n");
  for (k=0; k<12; k++) {
  for( i=0+k*80; i<80+k*80 ; i=i+chcnt ){
	   for(j=0;j<chcnt;j++)
	   {   
		   WD_AI_VoltScale (card, ai_range, ai_buf[i+j], &vol);
		   printf(" %04x(%4.4fV) ", (U16) (ai_buf[i+j]&cardProp.mask), vol);
	   }
	   printf("\n");
  }
  printf("Press any key for the continued data...\n");
  getch();
  }
}

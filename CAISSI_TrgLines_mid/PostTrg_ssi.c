#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "wd-dask.h"
#include "wddaskex.h"

#define CHANNELNUMBER 0
#define SCANCOUNT     400000
#define BUFAUTORESET   1
#define ADTRIGMODE    WD_AI_TRGMOD_MIDL
#define ADTRIGPOL     WD_AI_TrgNegative
#define SCAN_INTERVAL  1
#define ADTRIGSRC     WD_AI_TRGSRC_ANA
#define ANATRIGSRC    CH0ATRIG
#define ANATrigLevel   0
#define PRE_TRIG_COUNT       160000
#define POST_TRIG_COUNT      240000

I16 master_d=-1, slave_d=-1;
U16 ai_range_m=0, ai_range_s=0;
DAS_IOT_DEV_PROP cardProp_m, cardProp_s;

U16 ai_buf_m[SCANCOUNT];
U16 ai_buf_s[SCANCOUNT];
    U32 count_m=0, startPos_m;
    U32 count_s=0, startPos_s;

void show_channel_data();

void main()
{
    I16 err, card_num ,Id_m, Id_s = 0, is_9852_m=0, is_9852_s=0;
    BOOLEAN fStop=0, fok=0;
    U16 pol = WD_AI_TrgPositive;
	U16 card_type = 1;

	printf("Please input a card type for master devive: \n");
	card_type = WD_ChooseDeviceType(0);
	printf("Please input a card number for master devive: ");
    scanf(" %hd", &card_num);
    if ((master_d=WD_Register_Card (card_type, card_num)) <0 ) {
        printf("Register_Card error=%d", master_d);
        exit(1);
    }
	if(card_type == PCIe_9852) is_9852_m=TRUE;
	printf("\nPlease input a card type for slave devive: \n");
	card_type = WD_ChooseDeviceType(0);
	printf("Please input a card number for slave devive: ");
    scanf(" %hd", &card_num);
    if ((slave_d=WD_Register_Card (card_type, card_num)) <0 ) {
        printf("Register_Card error=%d", slave_d);
        exit(1);
    }
	if(card_type == PCIe_9852) is_9852_s=TRUE;
	WD_GetDeviceProperties (master_d, 0, &cardProp_m);
	ai_range_m = cardProp_m.default_range;
	WD_GetDeviceProperties (slave_d, 0, &cardProp_s);
	ai_range_s = cardProp_s.default_range;
	//here to config master device
	err = WD_AI_CH_Config (master_d, CHANNELNUMBER, ai_range_m);
    if (err!=NoError) {
       printf("WD_AI_CH_Config error=%d", err);
       exit(1);
    }

	err = WD_AI_Config (master_d, WD_IntTimeBase, 1, WD_AI_ADCONVSRC_TimePacer, 0, BUFAUTORESET);
    if (err!=0) {
       printf("WD_AI_Config error=%d", err);
       exit(1);
    }

	err = WD_AI_Trig_Config (master_d, ADTRIGMODE, ADTRIGSRC, ADTRIGPOL, ANATRIGSRC, ANATrigLevel, POST_TRIG_COUNT, PRE_TRIG_COUNT, 0, 1);
   // err = WD_AI_Trig_Config (master_d, ADTRIGMODE, WD_AI_TRGSRC_ANA, ADTRIGPOL, 0, 0.0, 0, 0, 0, 1);
    if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
       exit(1);
    }

	WD_SSI_SourceConn (master_d, SSI_TIME);
	WD_SSI_SourceConn (master_d, SSI_TRIG_SRC1);

	if((is_9852_s == TRUE) && (is_9852_m == TRUE))
	{
//master wish to receive the pre-data-ready signal from the slave through bus 0
	 err = WD_Route_Signal (master_d, 0x10, 0, 0);
     if (err!=0) {
       printf("WD_Route_Signal error=%d", err);
       exit(1);
     }
//slave output the pre-data-ready signal through bus 0
	 err = WD_Route_Signal (slave_d, 0x10, 0, 1);
     if (err!=0) {
       printf("WD_Route_Signal error=%d", err);     
	   exit(1);
     }
	}

	err=WD_AI_ContBufferSetup (master_d, ai_buf_m, SCANCOUNT, &Id_m);
    if (err!=0) {
       printf("WD_AI_ContBufferSetup error=%d", err);
       exit(1);
    }
	//here to config slave device
	err = WD_AI_CH_Config (slave_d, CHANNELNUMBER, ai_range_s);
    if (err!=NoError) {
       printf("WD_AI_CH_Config error=%d", err);
       exit(1);
    }
	err = WD_AI_Config (slave_d, WD_SSITimeBase/*WD_IntTimeBase*/, 1, WD_AI_ADCONVSRC_TimePacer, 0, BUFAUTORESET);
    if (err!=0) {
       printf("WD_AI_Config error=%d", err);
       exit(1);
    }
    err = WD_AI_Trig_Config (slave_d, ADTRIGMODE, WD_AI_TRSRC_SSI_1, pol, 0, 0.0, POST_TRIG_COUNT, PRE_TRIG_COUNT, 0, 1);
    if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
       exit(1);
    }
	err=WD_AI_ContBufferSetup (slave_d, ai_buf_s, SCANCOUNT, &Id_s);
    if (err!=0) {
       printf("WD_AI_ContBufferSetup error=%d", err);
       exit(1);
    }
    err = WD_AI_ContReadChannel (slave_d, CHANNELNUMBER, Id_s, SCANCOUNT, SCAN_INTERVAL, SCAN_INTERVAL, ASYNCH_OP);
    if (err!=0) {
       printf("WD_AI_ContReadChannel error=%d", err);
       exit(1);
    }
    err = WD_AI_ContReadChannel (master_d, CHANNELNUMBER, Id_m, SCANCOUNT, SCAN_INTERVAL, SCAN_INTERVAL, ASYNCH_OP);
    if (err!=0) {
       printf("WD_AI_ContReadChannel error=%d", err);
       exit(1);
    }
	fStop = 0;
    do {
        WD_AI_AsyncCheck(slave_d, &fStop, &count_s);
    } while(!fStop);
    WD_AI_AsyncClear(slave_d, &startPos_s, &count_s);
	printf("slave done\n");
	fStop = 0;
    do {
        WD_AI_AsyncCheck(master_d, &fStop, &count_m);
    } while(!fStop);

    WD_AI_AsyncClear(master_d, &startPos_m, &count_m);	

	WD_SSI_SourceClear (master_d);

	if((is_9852_s == TRUE) && (is_9852_m == TRUE))
		WD_SSI_SourceClear (slave_d);

    //Here to handle the data stored in ready buffer
	printf("Press any key to show the data...\n");
	getch();
    show_channel_data();
    WD_Release_Card(slave_d);
    WD_Release_Card(master_d);
    printf("\nPress ENTER to exit the program. "); getch();
}

void show_channel_data()
{
  int k;
  U32 i;
  char c;
  F64 vol_m = 0.0, vol_s = 0.0;

  printf(" >>>>>>>>>>>>>>> the valid scans  <<<<<<<<<<<<<<< \n");
  //printf("CH0 :\n");
  printf("master              slave\n");
  for (k=0; k<SCANCOUNT/20; k++) {
   for( i=0+k*20; i<20+k*20 ; i++ ){
	   	WD_AI_VoltScale (master_d, ai_range_m, ai_buf_m[(i+startPos_m)%SCANCOUNT], &vol_m);
		WD_AI_VoltScale (slave_d, ai_range_s, ai_buf_s[(i+startPos_s)%SCANCOUNT], &vol_s);
		printf("%d: 0x%-04x(%+4.4fV)      0x%-04x(%+4.4fV)\n", i,\
					(U16) (ai_buf_m[(i+startPos_m)%SCANCOUNT]&cardProp_m.mask), vol_m,
					(U16) (ai_buf_s[(i+startPos_s)%SCANCOUNT]&cardProp_s.mask), vol_s
				  );
   }
   printf("Press 'q' to EXIT or any other key for the continued data...\n");
   c=getch();
   if(c=='q')
	   return;
  }
}

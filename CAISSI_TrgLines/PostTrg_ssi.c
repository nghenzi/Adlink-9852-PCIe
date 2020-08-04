#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "wd-dask.h"
#include "wddaskex.h"

#define CHANNELNUMBER 0
#define SCANCOUNT     1024000
#define BUFAUTORESET   1
#define ADTRIGMODE    WD_AI_TRGMOD_POST
#define ADTRIGPOL     WD_AI_TrgPositive
#define SCAN_INTERVAL  1

I16 master_d=-1, slave_d=-1;
U16 ai_range_m=0, ai_range_s=0, timebase=WD_IntTimeBase;
DAS_IOT_DEV_PROP cardProp_m, cardProp_s;

U16 ai_buf_m[SCANCOUNT];
U16 ai_buf_s[SCANCOUNT];

void show_channel_data();

void main()
{
    I16 err, card_num ,Id_m, Id_s = 0;
    BOOLEAN fStop=0, fok=0;
    U32 count_m=0, startPos_m, i=0;
    U32 count_s=0, startPos_s;
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
	printf("\nPlease input a card type for slave devive: \n");
	card_type = WD_ChooseDeviceType(0);
	printf("Please input a card number for slave devive: ");
    scanf(" %hd", &card_num);
    if ((slave_d=WD_Register_Card (card_type, card_num)) <0 ) {
        printf("Register_Card error=%d", slave_d);
        exit(1);
    }
	WD_GetDeviceProperties (master_d, 0, &cardProp_m);
	ai_range_m = cardProp_m.default_range;
	WD_GetDeviceProperties (slave_d, 0, &cardProp_s);
	ai_range_s = cardProp_s.default_range;
	//here to config master device
	//for(i=0;i<10;i++){
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
    err = WD_AI_Trig_Config (master_d, ADTRIGMODE, WD_AI_TRGSRC_SOFT, ADTRIGPOL, 0, 0.3, 0, 0, 0, 1);
    if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
       exit(1);
    }

	WD_SSI_SourceConn (master_d, SSI_TIME);
    WD_SSI_SourceConn (master_d, SSI_TRIG_SRC1);

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
	err = WD_AI_Config (slave_d, WD_SSITimeBase, 1, WD_AI_ADCONVSRC_TimePacer, 0, BUFAUTORESET);
    if (err!=0) {
       printf("WD_AI_Config error=%d", err);
       exit(1);
    }
    err = WD_AI_Trig_Config (slave_d, ADTRIGMODE, WD_AI_TRSRC_SSI_1, pol, 0, 0.5, 0, 0, 0, 1);
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
	printf("slave\n");
	fStop = 0;
    do {
        WD_AI_AsyncCheck(master_d, &fStop, &count_m);
    } while(!fStop);
    WD_AI_AsyncClear(master_d, &startPos_m, &count_m);
	
	WD_SSI_SourceClear (master_d);

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
	   	WD_AI_VoltScale (master_d, ai_range_m, ai_buf_m[i%SCANCOUNT], &vol_m);
		WD_AI_VoltScale (slave_d, ai_range_s, ai_buf_s[i%SCANCOUNT], &vol_s);
		printf("%d: 0x%-04x(%+4.4fV)      0x%-04x(%+4.4fV)\n", i,\
					(U16) (ai_buf_m[i%SCANCOUNT]&cardProp_m.mask), vol_m,
					(U16) (ai_buf_s[i%SCANCOUNT]&cardProp_s.mask), vol_s
				  );
   }
   printf("Press 'q' to EXIT or any other key for the continued data...\n");
   c=getch();
   if(c=='q')
	   return;
  }
}

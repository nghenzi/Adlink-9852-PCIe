#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "wd-dask.h"
#include "wddaskex.h"

#define MaxCount 8000000
#define MaxCH 8

U16 channel;  
U16 range;
char *file_name="dbfai";
U32 read_count=1000000;
U32 samp_intrv = 1;
I16 *ai_buf, *ai_buf2;
DAS_IOT_DEV_PROP cardProp;
U16 chcnt;
I16 card = 0;
LARGE_INTEGER freq, start_count, current_count;
double take_msec;

void main()
{
    I16 err, card_num,Id, tofile=0;
    BOOLEAN halfReady, fStop;
    U32 count=0, count1, startPos, sts;
	U16 card_type = 1, g=0;

    printf("This program inputs data from all channels by\ndouble-buffer mode, and store data to file '%s.dat'. The size of circular\nbuffer is %d.\n",
           file_name, read_count);
	card_type = WD_ChooseDeviceType(0);
	printf("Please input a card number: ");
    scanf(" %hd", &card_num);
    //getch();
    if ((card=WD_Register_Card (card_type, card_num)) <0 ) {
        printf("Register_Card error=%d\n", card);
        exit(1);
    }
	WD_GetDeviceProperties (card, 0, &cardProp);
	range = cardProp.default_range;
	printf("channel count: ");
    scanf(" %hd", &chcnt);
	//chcnt = cardProp.num_of_channel;
	if(chcnt<1) chcnt = 1;
	channel = chcnt-1;
	WD_AI_CH_Config (card, -1, range);
	err = WD_AI_Config (card, WD_IntTimeBase, 1, WD_AI_ADCONVSRC_TimePacer, 0,1);
    if (err!=0) {
       printf("WD_AI_Config error=%d", err);
       exit(1);
    }
    err = WD_AI_Trig_Config (card, WD_AI_TRGMOD_POST, WD_AI_TRGSRC_SOFT, WD_AI_TrgNegative, 0, 0.0, 0, 0, 0, 1);
    if (err!=0) {
       printf("WD_AI_Trig_Config error=%d", err);
       exit(1);
    }
	printf("scan count?");
    scanf(" %ld", &read_count);
    read_count *= chcnt;
	/*if(read_count>MaxCount)
    {
		printf("The Max. scan count is %ld\n", MaxCount/chcnt);
		exit(1);
	}*/
   ai_buf = WD_Buffer_Alloc (card, read_count*sizeof(I16));
   if(!ai_buf) 
   {
	   printf("1st buffer allocation failed\n");
	   return;
   }
   memset(ai_buf, 0, read_count*sizeof(I16));
   ai_buf2 = WD_Buffer_Alloc (card, read_count*sizeof(I16));
   if(!ai_buf2) 
   {
	   printf("2nd buffer allocation failed\n");
	   WD_Buffer_Free (card, ai_buf);
	   return;
   }
   memset(ai_buf2, 0, read_count*sizeof(I16));

    err = WD_AI_AsyncDblBufferMode (card, 1);
    if (err!=0) {
       printf("DAQ2K_AI_DblBufferMode error=%d", err);
       exit(1);
    }
    err = WD_AI_ContBufferSetup (card, ai_buf, read_count, &Id);
    if (err!=0) {
       printf("WD_AI_ContBufferSetup error=%d for the 1st buffer", err);
       exit(1);
    }
    WD_AI_ContBufferSetup (card, ai_buf2, read_count, &Id);
    if (err!=0) {
       printf("WD_AI_ContBufferSetup error=%d for the 2nd buffer", err);
       exit(1);
    }

	printf("To File (0:No 1:Yes) ? ");
    scanf(" %hd", &tofile);

	printf("sample interval?");
    scanf(" %ld", &samp_intrv);

	printf("It will not stop until you press a key.\n\nPress any key to start the operation.\n");
   QueryPerformanceFrequency(&freq);
   QueryPerformanceCounter(&start_count);
   if(tofile==1)
		err = WD_AI_ContScanChannelsToFile (card, channel, Id, file_name, read_count/chcnt, samp_intrv, samp_intrv, ASYNCH_OP);
	else
		err = WD_AI_ContScanChannels (card, channel, Id, read_count/chcnt, samp_intrv, samp_intrv, ASYNCH_OP);
    if (err!=0) {
       printf("WD_AI_ContScanChannels error=%d", err);
       exit(1);
    }
    printf("\n\nPress any key to stop input operation.");
    printf("\n\nData count : \n");
    do {
        do 
		{
          WD_AI_AsyncDblBufferHalfReady(card, &halfReady, &fStop);
 		  WD_AI_ContStatus (card, &sts);
		  if(sts & 0x60000000)
		  {
			if(sts & 0x20000000)
					printf("db buffer overrun...\n");
			if(sts & 0x40000000)
			{
				printf("DDR overflow (%x)...\n", sts);
				g=1;
			}
			QueryPerformanceCounter(&current_count);
			take_msec =((double)(current_count.QuadPart-start_count.QuadPart))/freq.QuadPart*1000;
			printf("The time taken is: %4.3f msec\n",take_msec);
			printf("sts: %x\n", sts);
		  }
        } while (!halfReady && !kbhit());
		if(tofile==1)
			WD_AI_AsyncDblBufferToFile(card);
		WD_AI_AsyncDblBufferHandled(card);
        count += (read_count);
        printf("%d\r", count);
		if(g==1)
			break;
    } while(!kbhit());
	
	WD_AI_AsyncClear(card, &startPos, &count1);
	if(!g)
	{
	QueryPerformanceCounter(&current_count);
	take_msec =((double)(current_count.QuadPart-start_count.QuadPart))/freq.QuadPart*1000;
	printf("The time taken is: %4.3f msec\n",take_msec);
    }
    count += (count1);
    WD_Buffer_Free (card, ai_buf);
    WD_Buffer_Free (card, ai_buf2);
    WD_Release_Card(card);
	if(tofile==1)
		printf("\n\n%ld input data are stored in file '%s.dat'.\n", count, file_name);
	else
		printf("\n\n%ld input data\n", count);
    printf("\nPress ENTER to exit the program. "); getch();
}

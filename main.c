#pragma config FPLLIDIV = DIV_2
#pragma config FPLLMUL = MUL_16
#pragma config FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL
#pragma config POSCMOD = XT
#pragma config FPBDIV = DIV_1
#pragma config DEBUG = ON
#pragma config WDTPS = PS1024
#pragma config FWDTEN = OFF

#include <proc/p32mx360f512l.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/appio.h>
#include <stdint.h>
#include "transmit.h"
#include "GPS.h"
#include "MS5607.h"
#include "MAG3310.h"
#include "Timing.h"
#include "Resets.h"
#include "Stubs.h"
#include "Packet.h"
#include "RockBlock.h"
#include "Yikes.h"

#define T_TICK_MS 100
#define T_SECOND (1000/T_TICK_MS)
#define T_MINUTE (T_SECOND*60)
#define T_NORM_LEN (T_MINUTE*9)
#define T_CON_LEN (T_MINUTE)
#define T_CON_CHG_BEGIN  (T_SECOND*2)
#define T_CON_CHGN_END  (T_CON_CHG_BEGIN+T_SECOND*2)
#define T_CON_CHG1_END  (T_CON_CHGN_END+T_SECOND*2)
#define T_CON_MEAS1_END (T_CON_CHG1_END+T_SECOND*27)
#define T_CON_CHG2_END  (T_CON_MEAS1_END+T_SECOND*2)

#define T_FASTSAM_INTERVAL (T_SECOND*5)
#define T_SLOWSAM_INTERVAL (T_FASTSAM_INTERVAL*12)

#define FSYS 80000000L      // Frequency of system clock, 80MHz


unsigned char SBDnormal[512] = {0};

char nTime[20];
char nLati[20];
char nLong[20];
char nAlti[20];
char nLatD;
char nLonD;

double dTime;
double dLati;
double dLong;
double dAlti;

int magX=0;
int magY=0;
int magZ=0;

int pressure;
int temperature;

int VpH=0x111111;
int VpL=0x222222;
int VmH=0x333333;
int VmL=0x444444;
int HpH=0x555555;
int HpL=0x666666;
int HmH=0x777777;
int HmL=0x888888;

uint32_t gTime;
int32_t gLat;
int32_t gLon;
uint32_t gAlt;

uint16_t cVertH[320];
uint16_t cVertL[320];

int picTemp;

typedef enum state {INIT,NORMAL,CONDUCTIVITY} state_t;

state_t state=INIT;
int statetimer;

char tempbuf[20]; //TODO: DELETE

packet_u packet;


void clearPacket(packet_u *pack) {
    int i;
    for (i=0;i<sizeof(*pack);i++)
        (*pack).bytes[i]=0;
}

void printPacket(packet_u pack) {
    int i;
    SendChar_UART1('\n');
    for (i=0;i<sizeof(pack);i++) {
        if ((pack.bytes[i]&0xF)>9)
            SendChar_UART1((pack.bytes[i]&0xF)+'A'-10);
        else if ((pack.bytes[i]&0xF)>0)
            SendChar_UART1((pack.bytes[i]&0xF)+'0');
        else
            SendChar_UART1('.');

        if (((pack.bytes[i]>>4)&0xF)>9)
            SendChar_UART1(((pack.bytes[i]>>4)&0xF)+'A'-10);
        else if (((pack.bytes[i]>>4)&0xF)>0)
            SendChar_UART1(((pack.bytes[i]>>4)&0xF)+'0');
        else
            SendChar_UART1('.');
    }
    SendChar_UART1('\n');
}

int i,j;

int main(void) {
    //=============================//
    //       INITIALIZATION        //
    //=============================//
    yikes.byte=0;
    yikes.reset=1;

    InitGPIO();

    InitUART();

    InitGPS();
    InitRB();
    InitInterrupt();
    InitI2C();
    InitMagneto(MAG_ADDR);
    InitAltimeter(ALT_ADDR);

    InitPICADC();

    InitADC_S();

    InitLoopDelay();

    //SendString_UART1("!Init'd\r");

    //for (i=0;i<10000000;i++);
    
    //SendString_UART1("!OK\r");

    //SendString_UART1("!");
    //PrintResetReason();
    //SendString_UART1("\r");

    //InitWatchdog();

    state=CONDUCTIVITY;

    //PORTECLR=0xFF;

    GPSready=1;

    PORTEbits.RE8=1;


    //=============================//
    //          MAIN LOOP          //
    //=============================//
    while (1) {
        //SendString_UART1(gpsbuf);
        //sprintf(tempbuf,"%2d:%02d.%03d %1d %1d",statetimer/T_MINUTE,(statetimer/T_SECOND)%60,(statetimer%T_SECOND)*T_TICK_MS,GPSready,GPSnew);
        //SendString_UART1(tempbuf);
        //if (state==NORMAL)
        //    SendString_UART1("NORMAL ");
        //if (state==CONDUCTIVITY)
        //    SendString_UART1("CONDUCTIVITY ");
        TickRB();
        
        PORTEbits.RE7=statetimer%T_SECOND==0;
        switch (state) {
            case NORMAL:
                PORTEbits.RE6=0;
                packet.norm.type=0x55;
                if (statetimer%T_SLOWSAM_INTERVAL==0) {
                    clearPacket(&packet);
                    if ((statetimer/T_MINUTE)<8) {
                        memcpy(packet.norm.cVertH,cVertH+(statetimer/T_MINUTE)*40,40*2);
                        memcpy(packet.norm.cVertL,cVertL+(statetimer/T_MINUTE)*40,40*2);
                    }
                }
                if (statetimer%T_FASTSAM_INTERVAL==0) {
                    //SendChar_UART1('\n');
                    TriggerMagneto_S();
                }
                if (statetimer%T_FASTSAM_INTERVAL==1) {
                    uint16_t mx;
                    uint16_t my;
                    uint16_t mz;
                    ReadMagneto_S(&mx,&my,&mz);
                    packet.norm.compassX[(statetimer/T_FASTSAM_INTERVAL)%12]=mx;
                    packet.norm.compassY[(statetimer/T_FASTSAM_INTERVAL)%12]=my;
                    packet.norm.horizL[(statetimer/T_FASTSAM_INTERVAL)%12]=ReadADC_S(2);
                    packet.norm.horizR[(statetimer/T_FASTSAM_INTERVAL)%12]=ReadADC_S(5);
                }
                if (statetimer%T_FASTSAM_INTERVAL==2) {
                    packet.norm.horizD[(statetimer/T_FASTSAM_INTERVAL)%12]=ReadADC_S(3);
                }
                if (statetimer%T_SLOWSAM_INTERVAL==3) {
                    packet.norm.vertH=ReadADC_S(0);
                    packet.norm.vertL=ReadADC_S(4);
                }
                if (statetimer%T_SLOWSAM_INTERVAL==4) {
                    packet.norm.vertD=ReadADC_S(1);
                }
                if (statetimer%T_SLOWSAM_INTERVAL==5) {
                    ReadGPS_S(&gTime, &gLat, &gLon, &gAlt);
                    packet.norm.time=gTime;
                    packet.norm.lat=gLat;
                    packet.norm.lon=gLon;
                    packet.norm.alt=gAlt;
                }
                if (statetimer%T_SLOWSAM_INTERVAL==T_SECOND*59) {
                    packet.norm.yikes=yikes.byte;
                    yikes.byte=0;
                    RockSend_S(packet.bytes);
                    //printPacket(packet);
                }
                break;
            case CONDUCTIVITY:
                PORTEbits.RE6=1;
                packet.rare.type=0xAA;
                switch (statetimer) {
                    case T_CON_CHG_BEGIN:
                        //SendChar_UART1('\n');
                        ChargeProbe_S(GND);
                        break;
                    case T_CON_CHGN_END:
                        //SendChar_UART1('\n');
                        ChargeProbe_S(UP);
                        break;
                    case T_CON_CHG1_END:
                        //SendChar_UART1('\n');
                        ChargeProbe_S(NONE);
                        break;
                    case T_CON_MEAS1_END:
                        //SendChar_UART1('\n');
                        ChargeProbe_S(DOWN);
                        break;
                    case T_CON_CHG2_END:
                        //SendChar_UART1('\n');
                        ChargeProbe_S(NONE);
                        break;
                }
                if (statetimer==0) {
                    packet.rare.vertH=ReadADC_S(0);
                    packet.rare.vertL=ReadADC_S(4);
                }
                if (statetimer==1) {
                    packet.rare.vertD=ReadADC_S(1);
                }
                if (statetimer>T_CON_CHGN_END && statetimer<T_CON_CHGN_END+160) {
                    cVertH[statetimer-T_CON_CHGN_END]=ReadADC_S(0);
                    cVertL[statetimer-T_CON_CHGN_END]=ReadADC_S(4);
                }
                if (statetimer>T_CON_MEAS1_END && statetimer<T_CON_MEAS1_END+160) {
                    cVertH[statetimer-T_CON_MEAS1_END+160]=ReadADC_S(0);
                    cVertL[statetimer-T_CON_MEAS1_END+160]=ReadADC_S(4);
                }
                if (statetimer==0)
                    TriggerAltimeter_Temperature_S();
                if (statetimer==1)
                    packet.rare.temperature=ReadAltimeter_S();
                if (statetimer==2)
                    TriggerAltimeter_Pressure_S();
                if (statetimer==3)
                    packet.rare.pressure=ReadAltimeter_S();
                if (statetimer==4)
                    packet.rare.thermval=ReadPICADC_S(0);
                if (statetimer==5)
                    packet.rare.batvolt=ReadPICADC_S(1);
                if (statetimer==6)
                    packet.rare.batcurr=ReadPICADC_S(2);
                break;
        }
        statetimer++;
        switch (state) {
            case NORMAL:
                if (statetimer>T_NORM_LEN) {
                    state=CONDUCTIVITY;
                    statetimer=0;
                }
                break;
            case CONDUCTIVITY:
                if (statetimer>T_CON_LEN) {
                    packet.rare.yikes=yikes.byte;
                    yikes.byte=0;
                    RockSend_S(packet.bytes);
                    //printPacket(packet);
                    clearPacket(&packet);
                    state=NORMAL;
                    statetimer=0;
                }
                break;

        }
        ResetWatchdog();
        i++;
        DelayLoopMS(T_TICK_MS);
    }

    return 0;
}

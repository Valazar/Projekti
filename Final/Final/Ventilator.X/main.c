#include<p30fxxxx.h>
#include <stdlib.h>
#include "driverGLCD.h"
#include "adc.h"

_FOSC(CSW_ON_FSCM_OFF & HS3_PLL4);
_FWDT(WDT_OFF);
_FGS(CODE_PROT_OFF);

unsigned int X, Y,x_vrednost, y_vrednost;
unsigned int flag1=0;
unsigned int temperatura;
unsigned int napon; 
unsigned int ton = 0, duz, j;
const unsigned int AD_Xmin =220;
const unsigned int AD_Xmax =3642;
const unsigned int AD_Ymin =520;
const unsigned int AD_Ymax =3450;

float pauza;  
unsigned int sirovi0,sirovi1,sirovi2,sirovi3;
unsigned int broj,broj1,broj2,temp0,temp1; 
unsigned char tempRX;
unsigned int brojac_ms,stoperica,ms,sekund;

#define DRIVE_A PORTCbits.RC13
#define DRIVE_B PORTCbits.RC14

void ConfigureTSPins(void)
{
	
	TRISCbits.TRISC13=0;
    TRISCbits.TRISC14=0;
	
}

void initUART1(void)
{
    U1BRG=0x0015;//ovim odredjujemo baudrate
    U1MODEbits.ALTIO=0;//biramo koje pinove koristimo za komunikaciju osnovne ili alternativne
    IEC0bits.U1RXIE=1;//omogucavamo rx1 interupt
    U1STA&=0xfffc;
    U1MODEbits.UARTEN=1;//ukljucujemo ovaj modul
    U1STAbits.UTXEN=1;//ukljucujemo predaju
}

void __attribute__((__interrupt__)) _U1RXInterrupt(void) 
{
    IFS0bits.U1RXIF = 0;
    tempRX = 0;
    tempRX=U1RXREG;
} 

void WriteUART1(unsigned int data)
{
	  while(!U1STAbits.TRMT);
    if(U1MODEbits.PDSEL == 3)
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}


void WriteUART1dec2string(unsigned int data)
{
	unsigned char temp;

	temp=data/1000;
	WriteUART1(temp+'0');
    RS232_putst(".");
	data=data-temp*1000;
	temp=data/100;
	WriteUART1(temp+'0');
	data=data-temp*100;
	temp=data/10;
	WriteUART1(temp+'0');
	data=data-temp*10;
	WriteUART1(data+'0');
}


void Delay(unsigned int N)
{
	unsigned int i;
	for(i=0;i<N;i++);
}

void Touch_Panel (void)
{
// vode horizontalni tranzistori
	DRIVE_A = 1;  
	DRIVE_B = 0;
    
     LATCbits.LATC13=1;
     LATCbits.LATC14=0;

	Delay(500); //cekamo jedno vreme da se odradi AD konverzija
				
	// ocitavamo x	
	x_vrednost = temp0;//temp0 je vrednost koji nam daje AD konvertor na BOTTOM pinu		

	// vode vertikalni tranzistori
     LATCbits.LATC13=0;
     LATCbits.LATC14=1;
	DRIVE_A = 0;  
	DRIVE_B = 1;

	Delay(500); //cekamo jedno vreme da se odradi AD konverzija
	
	// ocitavamo y	
	y_vrednost = temp1;// temp1 je vrednost koji nam daje AD konvertor na LEFT pinu	
	
//Ako želimo da nam X i Y koordinate budu kao rezolucija ekrana 128x64 treba skalirati vrednosti x_vrednost i y_vrednost tako da budu u opsegu od 0-128 odnosno 0-64
//skaliranje x-koordinate

    X=(x_vrednost-161)*0.03629;



//X= ((x_vrednost-AD_Xmin)/(AD_Xmax-AD_Xmin))*128;	
//vrednosti AD_Xmin i AD_Xmax su minimalne i maksimalne vrednosti koje daje AD konvertor za touch panel.


//Skaliranje Y-koordinate
	Y= ((y_vrednost-500)*0.020725);

//	Y= ((y_vrednost-AD_Ymin)/(AD_Ymax-AD_Ymin))*64;
}


void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
							
	
	sirovi0=ADCBUF0;//0
	sirovi1=ADCBUF1;//1
    sirovi2=ADCBUF2;//2
    sirovi3=ADCBUF3;
    
	temp0=sirovi0;
	temp1=sirovi1;

    IFS0bits.ADIF = 0;
} 

void Write_GLCD(unsigned int data)
{
unsigned char temp;

temp=data/1000;
Glcd_PutChar(temp+'0');
data=data-temp*1000;
temp=data/100;
Glcd_PutChar(temp+'0');
data=data-temp*100;
temp=data/10;
Glcd_PutChar(temp+'0');
data=data-temp*10;
Glcd_PutChar(data+'0');
}

void Delay_ms (int vreme)//funkcija za kasnjenje u milisekundama
	{
		stoperica = 0;
		while(stoperica < vreme);
	}


void __attribute__ ((__interrupt__)) _T2Interrupt(void) // svakih 1ms
{

	 TMR2 =0;
     ms=1;//fleg za milisekundu ili prekid;potrebno ga je samo resetovati u funkciji

	brojac_ms++;//brojac milisekundi
    stoperica++;//brojac za funkciju Delay_ms

  if (brojac_ms==1000)//sek
        {
          brojac_ms=0;
          sekund=1;//fleg za sekundu
		 } 
	IFS0bits.T2IF = 0; 
       
}
void ispis_temperature(unsigned int data)
{
    unsigned char temp;
    
    temp=data/10;
	WriteUART1(temp+'0');
	data=data-temp*10;
	WriteUART1(data+'0');
}
void RS232_putst(register const char *str)
{
	while((*str) != 0)
	{
		WriteUART1(*str);
		if(*str == 13) WriteUART1(10);
		if(*str == 10) WriteUART1(13);
		str++;
	}
}

//funkcija za aktivaciju pir senzora
void pir_senzor()
{
    if(PORTDbits.RD8==1)
    {
        LATDbits.LATD9=1; 
    }
    else
    {
        LATDbits.LATD9=0;
    }
}

//funkcija za detektovanje intenziteta sunca
void fotootpornik()
{
    if(sirovi2>=0 && sirovi2<3400)
    {
        LATFbits.LATF6=0; //Dioda A11 se gasi na slabo osvetljenje
        flag1=0;
    }  

    else if(sirovi2>3400)
    {
        LATFbits.LATF6=1;//Dioda A11 se aktivira na jako osvetljenje
        flag1=1;
     
    } 
}

//funkcija za temperaturni senzor
void temperaturni_senzor(){
    if(sirovi3>2360 && sirovi3<2420)
    {
      temperatura=20;
    }
    else if(sirovi3>2420 && sirovi3<2450)
    {
      temperatura=25;  
    }
    else if(sirovi3>2450 && sirovi3<2490)
    {
      temperatura=30;  
    }
    
}
void nula_stepeni()
{
    LATBbits.LATB12=1;
    Delay_ms (6); 
    LATBbits.LATB12=0;
    Delay_ms (94);
    //LATBbits.LATB12=1;
}

void p90_stepeni()
{
    LATBbits.LATB12=1;
    Delay_ms (10); 
    LATBbits.LATB12=0;
    Delay_ms (90);
    //LATBbits.LATB12=1;
}

void m90_stepeni()
{
    LATBbits.LATB12=1;
    Delay_ms (5); 
    LATBbits.LATB12=0;
    Delay_ms (95);
    //LATBbits.LATB12=1;
}



//funkcija za aktivaciju bazera
void buzzer()
{
    for(j = 0; j < 30; j++)
    {
        for(ton = 0; ton < 70; ton++)
        {
            LATAbits.LATA11 =~ LATAbits.LATA11;
            for(duz = 0;duz < 100; duz++);                    
        }
            for(broj1=0;broj1<100;broj1++);
    }
}

void main(void)
{
    
	ConfigureLCDPins();
	ConfigureTSPins();

	GLCD_LcdInit();
	GLCD_ClrScr();
    initUART1();
	
	ADCinit();
	ConfigureADCPins();
	ADCON1bits.ADON=1;

    Init_T2();
    
    
	while(1)
	{
        
        
        
	Touch_Panel ();
            
        temperaturni_senzor(); // funkcija za ucitavanje temperature
        napon=(5*sirovi3)/4.095;
        for(broj1=0;broj1<100;broj1++)
		   for(broj2=0;broj2<300;broj2++);
           
        WriteUART1(' ');
        RS232_putst("Temperatura je ");
        ispis_temperature(temperatura);
        RS232_putst(" stepena celzijusa, a napon na senzoru je ");
        WriteUART1dec2string(napon);
        RS232_putst(" V");
        for(broj2=0;broj2<1000;broj2++);
        WriteUART1(13);//enter
                   
        fotootpornik(); //fukncija za fotootpornik
        pir_senzor(); //pir funkcija
        
        
        if(PORTDbits.RD9==1)
        {
            if(flag1==1)
            {

            GoToXY(0,0);
            GLCD_Printf ("UKLJUCITI VENTILATOR?");

            GoToXY(15,6);
            GLCD_Printf ("DA");

            GoToXY(100,6);		
            GLCD_Printf ("NE");

                if(PORTBbits.RB7==1)
                {
                    GLCD_ClrScr();

                }
                else
                {
                if ((1<X)&&(X<30)&& (1<Y)&&(Y<15)) //Ako je pritisnuto DA
                {           

                        if(temperatura==20)
                        {
                            for(broj1=0;broj1<5;broj1++);
                            nula_stepeni(); //nulti polozaj

                        }
                        else if(temperatura==25)
                        {
                            for(broj1=0;broj1<5;broj1++);
                            p90_stepeni(); //90 stepeni
                        }
                        else if(temperatura==30)
                        {
                            for(broj1=0;broj1<5;broj1++);
                            m90_stepeni(); //-90 stepeni 

                        }

                }
                else if ((90<X)&&(X<120)&& (1<Y)&&(Y<15)) //Ako je pritisnuto NE
                {  

                         GLCD_ClrScr();
                         buzzer();
                         GoToXY(0,0);
                         GLCD_Printf ("PRETOPLO JE,UKLJUCITE VENTILATOR IPAK");
                         for(broj1=0;broj1<500;broj1++)
                            for(broj2=0;broj2<1000;broj2++); 
                            GLCD_ClrScr();

                 }
            }

            }
            else
            {
                GLCD_ClrScr();
            }
        }
        else
        {
          GLCD_ClrScr();
          GoToXY(0,0);
          GLCD_Printf ("NEMA NIKOG U BLIZINI!");
          for(broj1=0;broj1<500;broj1++)
            for(broj2=0;broj2<1000;broj2++); 
            GLCD_ClrScr();  
        }
        
	}//WHILE

}//MAIN

			
		
												

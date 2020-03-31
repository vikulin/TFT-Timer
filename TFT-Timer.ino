// © Vikulin 2020
// Реле времени для часов/минут 
// погрешность ~ 2 секунды в час
#include "pitches.h"
#include <TFT.h> // Hardware-specific library
#include <SPI.h>

#include <TimerOne.h>
#include <Encod_er.h>

#define CS   10
#define DC   9
#define RESET  8

#define TFT_SCLK 13   // set these to be whatever pins you like! (slow connection)
#define TFT_MOSI 11   // set these to be whatever pins you like!

#define pin_DT 6      // пин 6 подключаем к DT энкодера
#define pin_CLK 5     // пин 5 подключаем к CLK энкодера
#define pin_SW 7      // пин 7 подключаем к SW энкодера
#define pin_Relay 3   // пин 4 подключаем к реле
#define pin_Speaker 4 // пин 3 подключаем к + пищалки ( - пищалки на землю)

#define NotPush 0
#define ShortPush 1
#define LongPush 2
#define DurationOfLongPush 2000 // длительность длинного нажатия
#define interval 250

Encod_er encoder(pin_CLK, pin_DT, pin_SW);

TFT tft = TFT(CS, DC, RESET);

String data[] = {"","","",""};
byte mask=15, Hour=0, Minut=0, flsvet, zn, Signal=0;
unsigned long TimeOfPush, tempu, TimerSec=0, mycounter=0, temp1, temp2;
bool dvoet, myfl=false, ModeArbeiten=false, UkazHM=true, flEND=false;
int8_t VAL;
struct myMerz
 {word freq;
  unsigned long temp; 
  bool flsvet;
 };

myMerz trig[]={{500,0,false},  // интервал мерцания двоеточия при работе
               {200,0,false},  // интервал мерцания минут и часов при задании времени
               {1000,0,false}};// интервал прерывания сигнала при окончании работы

char charBufVarHH[3];
char charBufVarMM[3];

void setup() 
{ Serial.begin(9600);
  Timer1.initialize(interval); // инициализация таймера 1, период interval мкс
  Timer1.attachInterrupt(timerInterrupt, interval); // задаем обработчик прерываний
  tft.begin();  
  tft.background(255, 255, 255);
  tft.setTextSize(3);
  pinMode (pin_DT,INPUT);
  pinMode (pin_CLK,INPUT);
  pinMode (pin_SW,INPUT);  
  pinMode (pin_Relay,OUTPUT);  
  digitalWrite(pin_Relay, HIGH);
  digitalWrite(pin_SW, HIGH); // подтягиваем к 5V
  TimeOfPush=0;
  delay(100);
  encoder.timeLeft= 0;
  encoder.timeRight= 0;

  data[0]=String(Hour/10);
  data[1]=String(Hour%10);
  data[2]=String(Minut/10);
  data[3]=String(Minut%10);
  data[0].concat(data[1]);
  data[2].concat(data[3]);

  for (byte i=0;i<4;i++) if (!bitRead(mask,i)) data[3-i]="0";
  // print the sensor value
  data[0].toCharArray(charBufVarHH, 3);
  data[2].toCharArray(charBufVarMM, 3);
}
//------------------------------------------------------------
// обработчик прерывания 
void timerInterrupt() {
  encoder.scanState(); 
  if (ModeArbeiten) {
    tempu++;
    if (tempu==4000) {
        tempu=0; mycounter++;
    }
  }
}
String str;

//------------------------------------------------------------
void View()
{ 
  if(data[3]!=String(Minut%10) || data[1]!=String(Hour%10)){
    // erase the text you just wrote
    tft.stroke(255, 255, 255);
    tft.text(charBufVarHH, 20, 40);
    tft.text(charBufVarMM, 80, 40);

    data[0]=String(Hour/10);
    data[1]=String(Hour%10);
    data[2]=String(Minut/10);
    data[3]=String(Minut%10);
    data[0].concat(data[1]);
    data[2].concat(data[3]);
  
    Serial.print(data[0]);
    for (byte i=0;i<4;i++) if (!bitRead(mask,i)) data[3-i]="0";
             
    // set the font color
    tft.stroke(0, 0, 0);
    // print the sensor value
    data[0].toCharArray(charBufVarHH, 3);
    data[2].toCharArray(charBufVarMM, 3);
    tft.text(charBufVarHH, 20, 40);
    tft.text(charBufVarMM, 80, 40);
    // wait for a moment
    if(!ModeArbeiten) {
      tft.stroke(0, 0, 0);
      tft.text(":", 60, 40);
    }
  }
  if (dvoet && ModeArbeiten) {
    tft.stroke(255, 255, 255);
    tft.text(":", 60, 40);
    delay(100);
    tft.stroke(0, 0, 0);
    tft.text(":", 60, 40);
  }
}
//------------------------------------------------------------
bool procmerz(byte num)
{ unsigned long tempmillis=millis();        // триггер переключается с периодом из trig
   if (tempmillis-trig[num].temp>trig[num].freq) 
      { trig[num].temp=tempmillis;
        trig[num].flsvet=!trig[num].flsvet;
      }
  return(trig[num].flsvet);
}
//------------------------------------------------------------
byte mypush() // возвращает длинное-2, короткое-1 или осутствие нажатия-0 
{ unsigned long tpr=millis();
  byte res=NotPush;
  if (!digitalRead(pin_SW)) 
     { if (TimeOfPush==0) TimeOfPush=tpr; else
                                          if (tpr-TimeOfPush>DurationOfLongPush && !myfl)
                                             { TimeOfPush=0;
                                               myfl=true;
                                               return(LongPush);
                                             }
     } else
       { if (TimeOfPush>0 && !myfl) res=ShortPush;
         TimeOfPush=0;                                     
         myfl=false;
       }
  return(res); 
}
//------------------------------------------------------------
int8_t norm(int8_t aa,  byte dob, byte mf) // делаем аа в пределах mf, начиная от dob
{ 
   if (aa>mf) return(dob); 
   if (aa<dob) return(mf); 
   return(aa);    
}
//------------------------------------------------------------
 void Sec2HourMin()
 {
  Hour=(TimerSec-mycounter)/3600;
  Minut=(TimerSec-mycounter)%3600/60;
 }
//------------------------------------------------------------

int melody[] = {
  NOTE_A7, NOTE_G7, NOTE_G3, NOTE_C7, NOTE_D7, NOTE_B7, NOTE_F7, NOTE_C7
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  8, 8, 8, 8, 8, 8, 8, 8
};

void loop() 
{
  dvoet=true;
  zn=procmerz(1);
  mask=15;
  unsigned long tempx=millis();
  if (procmerz(2) && flEND && tempx-temp2<20000L) {
      // iterate over the notes of the melody:
    for (int thisNote = 0; thisNote < 8; thisNote++) {
  
      // to calculate the note duration, take one second divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 1000 / noteDurations[thisNote];
      tone(pin_Speaker, melody[thisNote], noteDuration);
  
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(pin_Speaker);
    }
  }
     

  byte rrr=mypush();
  if (rrr!=NotPush)  // произошло нажатие
  { flEND=false;
    if (rrr==ShortPush) UkazHM=!UkazHM; // короткое
    if (rrr==LongPush) {ModeArbeiten=!ModeArbeiten; // длинное 
                        if (ModeArbeiten) { mycounter=0; // запуск таймера
                                            TimerSec=Hour*3600L+Minut*60L;
                                            tempu=0;
                                            if (TimerSec==0) ModeArbeiten=false; 
                                               else digitalWrite(pin_Relay,LOW);
                                          } else 
                                            { digitalWrite(pin_Relay,HIGH);//отключаем реле принудительно
                                              Hour=0; 
                                              Minut=0;
                                            }
                       }
  }
 if (ModeArbeiten) // режим работы
 { dvoet=procmerz(0);
   if (TimerSec==mycounter) { ModeArbeiten=false; // отключаем реле по времени
                              digitalWrite(pin_Relay,HIGH);
                              temp2=tempx;
                              flEND=true;
                            }
   Sec2HourMin();   
 } else            // режим установки времени
   {if (UkazHM) { if (zn) mask=B0011;
                  Hour=norm(Hour+VAL,0,99);
                }
                  else {if (zn) mask=B1100;
                        Minut=norm(Minut+VAL,0,59);   
                       } 
    if (tempx-temp1<500) mask=15; // не моргать при вращении энкодера
   }

VAL=0;
if (encoder.timeRight!=0) // обрабатываем повороты энкодера
  { VAL=1;
    encoder.timeRight=0;
    goto m1;
  } else
if (encoder.timeLeft!=0)
  { VAL=-1;
    encoder.timeLeft=0;
   m1: temp1=tempx;
       flEND=false; // при любом повороте отключаем сигнал
       if (!digitalRead(pin_SW)) VAL=0; // если кнопка нажата, поворот игнорить
  }

  View();
}

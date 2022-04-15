#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Wire.h>
#include "LCD.h"
#include "LiquidCrystal_I2C.h"
#include <SD.h>
#define I2C_ADDR 0x27
#define Rs_pin 0
#define Rw_pin 1
#define En_pin 2
#define BACKLIGHT_PIN 3
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

#define ARDUINO_RX 5//should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 6//connect to RX of the module

File file;
File myFile;
int sequence[8];
int j=0;

SoftwareSerial mySerial(ARDUINO_RX, ARDUINO_TX);//init the serial protocol, tell to myserial wich pins are TX and RX

////////////////////////////////////////////////////////////////////////////////////
//all the commands needed in the datasheet(http://geekmatic.in.ua/pdf/Catalex_MP3_board.pdf)
static int8_t Send_buf[8] = {0} ;//The MP3 player undestands orders in a 8 int string
                                 //0X7E FF 06 command 00 00 00 EF;(if command =01 next song order) 
#define NEXT_SONG 0X01 
#define PREV_SONG 0X02 

#define CMD_PLAY_W_INDEX 0X03 //DATA IS REQUIRED (number of song)

#define VOLUME_UP_ONE 0X04
#define VOLUME_DOWN_ONE 0X05
#define CMD_SET_VOLUME 0X06//DATA IS REQUIRED (number of volume from 0 up to 30(0x1E))
#define SET_DAC 0X17
#define CMD_PLAY_WITHVOLUME 0X22 //data is needed  0x7E 06 22 00 xx yy EF;(xx volume)(yy number of song)

#define CMD_SEL_DEV 0X09 //SELECT STORAGE DEVICE, DATA IS REQUIRED
                #define DEV_TF 0X02 //HELLO,IM THE DATA REQUIRED
                
#define SLEEP_MODE_START 0X0A
#define SLEEP_MODE_WAKEUP 0X0B

#define CMD_RESET 0X0C//CHIP RESET
#define CMD_PLAY 0X0D //RESUME PLAYBACK
#define CMD_PAUSE 0X0E //PLAYBACK IS PAUSED

#define CMD_PLAY_WITHFOLDER 0X0F//DATA IS NEEDED, 0x7E 06 0F 00 01 02 EF;(play the song with the directory \01\002xxxxxx.mp3

#define STOP_PLAY 0X16

#define PLAY_FOLDER 0X17// data is needed 0x7E 06 17 00 01 XX EF;(play the 01 folder)(value xx we dont care)

#define SET_CYCLEPLAY 0X19//data is needed 00 start; 01 close

#define SET_DAC 0X17//data is needed 00 start DAC OUTPUT;01 DAC no output
////////////////////////////////////////////////////////////////////////////////////
int count=0;
String menit;
String detik;
String jam;
String myjam;
String gabungan;
String gabunganall;
String getsetmode;
int konversi;
int pin_4=4;
long x, y;

LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

void(* resetFunc) (void) = 0;

bool readLine(File &f, char* line, size_t maxLen) {
  for (size_t n = 0; n < maxLen; n++) {
    int c = f.read();
    if ( c < 0 && n == 0) return false;  // EOF
    if (c < 0 || c == '\n') {
      line[n] = 0;
      return true;
    }
    line[n] = c;
  }
  return false; // line too long
}

bool readVals(long* v1, long* v2) {
  char line[40], *ptr, *str;
  if (!readLine(file, line, sizeof(line))) {
    return false;  // EOF or too long
  }
  *v1 = strtol(line, &ptr, 10);
  if (ptr == line) return false;  // bad number if equal
  while (*ptr) {
    if (*ptr++ == ',') break;
  }
  *v2 = strtol(ptr, &str, 10);
  return str != ptr;  // true if number found
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}
void setup()
{
  Wire.begin();
  Serial.begin(9600);
  pinMode(pin_4,OUTPUT);
  digitalWrite(pin_4,HIGH);
  // set the initial time here:
  // DS3231 seconds, minutes, hours, day, date, month, year
  //setDS3231time(30,11,11,4,1,5,19);
  lcd.begin (20, 4);

  // LCD Backlight ON
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);

  lcd.home (); // go home on LCD
  lcd.print("Smart Bell v1.0");
  delay(3000);
  lcd.clear();
  mySerial.begin(9600);//Start our Serial coms for THE MP3
  delay(500);//Wait chip initialization is complete
  sendCommand(CMD_SEL_DEV, DEV_TF);//select the TF card
  delay(200);//wait for 200ms  
  
  if (!SD.begin(10)) {
    Serial.println("begin error");
    return;
  }

  
  file = SD.open("modeatur.csv", FILE_READ);
  if (!file) {
  //Serial.println("open error");
  return;
  }

  while (readVals(&x, &y)) {
    sequence[j] = x;
    j++;
  }
  if(sequence[0]==1)
  {
    // DS3231 seconds, minutes, hours, day, date, month, year
    setDS3231time(sequence[3],sequence[2],sequence[1],sequence[4],sequence[5],sequence[6],sequence[7]);  
  }

  
}
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}
void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);


  if (hour<10)
  {
    jam="0"+String(hour, DEC);
    myjam=hour;
    //Serial.print("0");
  }
  else
  {
    jam=hour;  
    myjam=hour;
  }
  
  if (minute<10)
  {
    menit="0"+String(minute, DEC);
    //Serial.print("0");
  }
  else
  {
    menit=minute;  
  }

  //Serial.print(menit);
  //Serial.print(minute, DEC);
  //Serial.print(":");
  if (second<10)
  {
    detik="0"+String(second, DEC);
    //Serial.print("0");
  }
  else
  {
    detik=second;
  }

  gabungan = jam + ":" + menit + ":" + detik;
  gabunganall = myjam + "" + menit;
  konversi=gabunganall.toInt();

  //Serial.print(konversi);
  //Serial.print(second, DEC);
  //Serial.print(" ");
  //Serial.print(dayOfMonth, DEC);
  //Serial.print("/");
  //Serial.print(month, DEC);
  //Serial.print("/");
  //Serial.print(year, DEC);
  lcd.setCursor (8, 0); // go to start of 2nd line
  lcd.print(String(dayOfMonth,DEC)+"/"+String(month,DEC)+"/"+String(year,DEC));
  switch(dayOfWeek){
  case 1:
    file = SD.open("minggu.csv", FILE_READ);
    if (!file) {
    //Serial.println("open error");
    return;
    }
    lcd.setCursor (0, 0); // go to start of 2nd line
    lcd.print("Minggu          ");
    while (readVals(&x, &y)) {
      
      if(konversi==x)
      {
        digitalWrite(pin_4,LOW);
        delay(3000);
        if(y==1)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0101);  
          delay(61000);

        }
        else if(y==2)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0102);
          delay(61000);  
  
        }
        else if(y==3)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0103);
          delay(61000);  

        }
        else if(y==4)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0104);
          delay(61000); 
  
        }
        else if(y==5)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0105);
          delay(61000);  

        }
        else if(y==6)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0106);
          delay(61000); 

        }
        else if(y==7)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0107);
          delay(61000);  

        }
        else if(y==8)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0108);
          delay(61000);  

        }
        else if(y==9)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0109);
          delay(61000); 

        }
        else if(y==10)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010A);
          delay(61000);  
 
        }
        else if(y==11)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010B);
          delay(61000);  
   
        }
        else if(y==12)
        {
    
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010C);
          delay(61000); 
     
        }
        else if(y==13)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010D);
          delay(61000);  
   
        }
        else if(y==14)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010E);
          delay(61000);  

        }
        else if(y==15)
        {
     
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010F);
          delay(61000);  

        }
        else if(y==16)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0110);
          delay(61000); 

        }
        else if(y==17)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0111);
          delay(61000);  

        }
        else if(y==18)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0112);
          delay(61000);  

        }
        else if(y==19)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0113);
          delay(61000);  

        }
        else if(y==20)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0114);
          delay(61000);  

        }
        else if(y==21)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0115);
          delay(61000); 

        }
        else if(y==22)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0116);
          delay(61000);  
 
        }
        else if(y==23)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0117);
          delay(61000); 
   
        }
        else if(y==24)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0118);
          delay(61000);  

        }
        else if(y==25)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0119);
          delay(61000);  
      
        }
        else if(y==26)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011A);
          delay(61000);  

        }
        else if(y==27)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011B);
          delay(61000);  
   
        }
        else if(y==28)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011C);
          delay(61000);  
   
        }
        else if(y==29)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011D);
          delay(61000); 
  
        }
        else if(y==30)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011E);
          delay(61000); 

        }
        else if(y==31)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011F);
          delay(61000);  

        }
        else if(y==32)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0120);
          delay(61000);  

        }
        else if(y==33)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0121);
          delay(61000);  
   
        }
        else if(y==34)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0122);
          delay(61000);  
          
        }
        else if(y==35)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0123);
          delay(61000);
  
        }
        else if(y==36)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0124);
          delay(61000);
        
        }
        else if(y==37)
        {
      
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0125);
          delay(61000);  
      
        }

        digitalWrite(pin_4,HIGH);
        
      }
    }
    break;
  case 2:
    file = SD.open("senin.csv", FILE_READ);
    if (!file) {
    //Serial.println("open error");
    return;
    }
    lcd.setCursor (0, 0); // go to start of 2nd line
    lcd.print("Senin          ");
    while (readVals(&x, &y)) {
      
      if(konversi==x)
      {
        digitalWrite(pin_4,LOW);
        delay(3000);
        
        if(y==1)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0101);  
          delay(61000);

        }
        else if(y==2)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0102);
          delay(61000);  
  
        }
        else if(y==3)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0103);
          delay(61000);  

        }
        else if(y==4)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0104);
          delay(61000); 
  
        }
        else if(y==5)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0105);
          delay(61000);  

        }
        else if(y==6)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0106);
          delay(61000); 

        }
        else if(y==7)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0107);
          delay(61000);  

        }
        else if(y==8)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0108);
          delay(61000);  

        }
        else if(y==9)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0109);
          delay(61000); 

        }
        else if(y==10)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010A);
          delay(61000);  
 
        }
        else if(y==11)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010B);
          delay(61000);  
   
        }
        else if(y==12)
        {
    
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010C);
          delay(61000); 
     
        }
        else if(y==13)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010D);
          delay(61000);  
   
        }
        else if(y==14)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010E);
          delay(61000);  

        }
        else if(y==15)
        {
     
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010F);
          delay(61000);  

        }
        else if(y==16)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0110);
          delay(61000); 

        }
        else if(y==17)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0111);
          delay(61000);  

        }
        else if(y==18)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0112);
          delay(61000);  

        }
        else if(y==19)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0113);
          delay(61000);  

        }
        else if(y==20)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0114);
          delay(61000);  

        }
        else if(y==21)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0115);
          delay(61000); 

        }
        else if(y==22)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0116);
          delay(61000);  
 
        }
        else if(y==23)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0117);
          delay(61000); 
   
        }
        else if(y==24)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0118);
          delay(61000);  

        }
        else if(y==25)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0119);
          delay(61000);  
      
        }
        else if(y==26)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011A);
          delay(61000);  

        }
        else if(y==27)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011B);
          delay(61000);  
   
        }
        else if(y==28)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011C);
          delay(61000);  
   
        }
        else if(y==29)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011D);
          delay(61000); 
  
        }
        else if(y==30)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011E);
          delay(61000); 

        }
        else if(y==31)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011F);
          delay(61000);  

        }
        else if(y==32)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0120);
          delay(61000);  

        }
        else if(y==33)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0121);
          delay(61000);  
   
        }
        else if(y==34)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0122);
          delay(61000);  
          
        }
        else if(y==35)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0123);
          delay(61000);
  
        }
        else if(y==36)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0124);
          delay(61000);
        
        }
        else if(y==37)
        {
      
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0125);
          delay(61000);  
      
        }

        digitalWrite(pin_4,HIGH);
        
      }
    }
    break;
  case 3:
    file = SD.open("selasa.csv", FILE_READ);
    if (!file) {
    //Serial.println("open error");
    return;
    }
    lcd.setCursor (0, 0); // go to start of 2nd line
    lcd.print("Selasa          ");
    while (readVals(&x, &y)) {
      
      if(konversi==x)
      {
        digitalWrite(pin_4,LOW);
        delay(3000);
        if(y==1)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0101);  
          delay(61000);

        }
        else if(y==2)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0102);
          delay(61000);  
  
        }
        else if(y==3)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0103);
          delay(61000);  

        }
        else if(y==4)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0104);
          delay(61000); 
  
        }
        else if(y==5)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0105);
          delay(61000);  

        }
        else if(y==6)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0106);
          delay(61000); 

        }
        else if(y==7)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0107);
          delay(61000);  

        }
        else if(y==8)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0108);
          delay(61000);  

        }
        else if(y==9)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0109);
          delay(61000); 

        }
        else if(y==10)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010A);
          delay(61000);  
 
        }
        else if(y==11)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010B);
          delay(61000);  
   
        }
        else if(y==12)
        {
    
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010C);
          delay(61000); 
     
        }
        else if(y==13)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010D);
          delay(61000);  
   
        }
        else if(y==14)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010E);
          delay(61000);  

        }
        else if(y==15)
        {
     
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010F);
          delay(61000);  

        }
        else if(y==16)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0110);
          delay(61000); 

        }
        else if(y==17)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0111);
          delay(61000);  

        }
        else if(y==18)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0112);
          delay(61000);  

        }
        else if(y==19)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0113);
          delay(61000);  

        }
        else if(y==20)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0114);
          delay(61000);  

        }
        else if(y==21)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0115);
          delay(61000); 

        }
        else if(y==22)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0116);
          delay(61000);  
 
        }
        else if(y==23)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0117);
          delay(61000); 
   
        }
        else if(y==24)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0118);
          delay(61000);  

        }
        else if(y==25)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0119);
          delay(61000);  
      
        }
        else if(y==26)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011A);
          delay(61000);  

        }
        else if(y==27)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011B);
          delay(61000);  
   
        }
        else if(y==28)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011C);
          delay(61000);  
   
        }
        else if(y==29)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011D);
          delay(61000); 
  
        }
        else if(y==30)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011E);
          delay(61000); 

        }
        else if(y==31)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011F);
          delay(61000);  

        }
        else if(y==32)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0120);
          delay(61000);  

        }
        else if(y==33)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0121);
          delay(61000);  
   
        }
        else if(y==34)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0122);
          delay(61000);  
          
        }
        else if(y==35)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0123);
          delay(61000);
  
        }
        else if(y==36)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0124);
          delay(61000);
        
        }
        else if(y==37)
        {
      
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0125);
          delay(61000);  
      
        }
        digitalWrite(pin_4,HIGH);
        
      }
    }
    break;
  case 4:
   file = SD.open("rabu.csv", FILE_READ);
    if (!file) {
    //Serial.println("open error");
    return;
    }
    lcd.setCursor (0, 0); // go to start of 2nd line
    lcd.print("Rabu          ");
    while (readVals(&x, &y)) {
      
      if(konversi==x)
      {
        digitalWrite(pin_4,LOW);
        delay(3000);
        if(y==1)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0101);  
          delay(61000);

        }
        else if(y==2)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0102);
          delay(61000);  
  
        }
        else if(y==3)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0103);
          delay(61000);  

        }
        else if(y==4)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0104);
          delay(61000); 
  
        }
        else if(y==5)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0105);
          delay(61000);  

        }
        else if(y==6)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0106);
          delay(61000); 

        }
        else if(y==7)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0107);
          delay(61000);  

        }
        else if(y==8)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0108);
          delay(61000);  

        }
        else if(y==9)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0109);
          delay(61000); 

        }
        else if(y==10)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010A);
          delay(61000);  
 
        }
        else if(y==11)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010B);
          delay(61000);  
   
        }
        else if(y==12)
        {
    
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010C);
          delay(61000); 
     
        }
        else if(y==13)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010D);
          delay(61000);  
   
        }
        else if(y==14)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010E);
          delay(61000);  

        }
        else if(y==15)
        {
     
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010F);
          delay(61000);  

        }
        else if(y==16)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0110);
          delay(61000); 

        }
        else if(y==17)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0111);
          delay(61000);  

        }
        else if(y==18)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0112);
          delay(61000);  

        }
        else if(y==19)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0113);
          delay(61000);  

        }
        else if(y==20)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0114);
          delay(61000);  

        }
        else if(y==21)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0115);
          delay(61000); 

        }
        else if(y==22)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0116);
          delay(61000);  
 
        }
        else if(y==23)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0117);
          delay(61000); 
   
        }
        else if(y==24)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0118);
          delay(61000);  

        }
        else if(y==25)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0119);
          delay(61000);  
      
        }
        else if(y==26)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011A);
          delay(61000);  

        }
        else if(y==27)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011B);
          delay(61000);  
   
        }
        else if(y==28)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011C);
          delay(61000);  
   
        }
        else if(y==29)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011D);
          delay(61000); 
  
        }
        else if(y==30)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011E);
          delay(61000); 

        }
        else if(y==31)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011F);
          delay(61000);  

        }
        else if(y==32)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0120);
          delay(61000);  

        }
        else if(y==33)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0121);
          delay(61000);  
   
        }
        else if(y==34)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0122);
          delay(61000);  
          
        }
        else if(y==35)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0123);
          delay(61000);
  
        }
        else if(y==36)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0124);
          delay(61000);
        
        }
        else if(y==37)
        {
      
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0125);
          delay(61000);  
      
        }
        digitalWrite(pin_4,HIGH);
        
      }
    }
    break;
  case 5:
   file = SD.open("kamis.csv", FILE_READ);
    if (!file) {
    //Serial.println("open error");
    return;
    }
    lcd.setCursor (0, 0); // go to start of 2nd line
    lcd.print("Kamis          ");
    while (readVals(&x, &y)) {
      
      if(konversi==x)
      {
        digitalWrite(pin_4,LOW);
        delay(3000);
        
        if(y==1)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0101);  
          delay(61000);

        }
        else if(y==2)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0102);
          delay(61000);  
  
        }
        else if(y==3)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0103);
          delay(61000);  

        }
        else if(y==4)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0104);
          delay(61000); 
  
        }
        else if(y==5)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0105);
          delay(61000);  

        }
        else if(y==6)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0106);
          delay(61000); 

        }
        else if(y==7)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0107);
          delay(61000);  

        }
        else if(y==8)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0108);
          delay(61000);  

        }
        else if(y==9)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0109);
          delay(61000); 

        }
        else if(y==10)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010A);
          delay(61000);  
 
        }
        else if(y==11)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010B);
          delay(61000);  
   
        }
        else if(y==12)
        {
    
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010C);
          delay(61000); 
     
        }
        else if(y==13)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010D);
          delay(61000);  
   
        }
        else if(y==14)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010E);
          delay(61000);  

        }
        else if(y==15)
        {
     
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010F);
          delay(61000);  

        }
        else if(y==16)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0110);
          delay(61000); 

        }
        else if(y==17)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0111);
          delay(61000);  

        }
        else if(y==18)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0112);
          delay(61000);  

        }
        else if(y==19)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0113);
          delay(61000);  

        }
        else if(y==20)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0114);
          delay(61000);  

        }
        else if(y==21)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0115);
          delay(61000); 

        }
        else if(y==22)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0116);
          delay(61000);  
 
        }
        else if(y==23)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0117);
          delay(61000); 
   
        }
        else if(y==24)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0118);
          delay(61000);  

        }
        else if(y==25)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0119);
          delay(61000);  
      
        }
        else if(y==26)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011A);
          delay(61000);  

        }
        else if(y==27)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011B);
          delay(61000);  
   
        }
        else if(y==28)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011C);
          delay(61000);  
   
        }
        else if(y==29)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011D);
          delay(61000); 
  
        }
        else if(y==30)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011E);
          delay(61000); 

        }
        else if(y==31)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011F);
          delay(61000);  

        }
        else if(y==32)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0120);
          delay(61000);  

        }
        else if(y==33)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0121);
          delay(61000);  
   
        }
        else if(y==34)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0122);
          delay(61000);  
          
        }
        else if(y==35)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0123);
          delay(61000);
  
        }
        else if(y==36)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0124);
          delay(61000);
        
        }
        else if(y==37)
        {
      
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0125);
          delay(61000);  
      
        }

        digitalWrite(pin_4,HIGH);
        
      }
    }
    break;
  case 6:
    file = SD.open("jumat.csv", FILE_READ);
    if (!file) {
    //Serial.println("open error");
    return;
    }
    lcd.setCursor (0, 0); // go to start of 2nd line
    lcd.print("Jumat          ");
    while (readVals(&x, &y)) {
      
      if(konversi==x)
      {
        digitalWrite(pin_4,LOW);
        delay(3000);
        
        if(y==1)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0101);  
          delay(61000);

        }
        else if(y==2)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0102);
          delay(61000);  
  
        }
        else if(y==3)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0103);
          delay(61000);  

        }
        else if(y==4)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0104);
          delay(61000); 
  
        }
        else if(y==5)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0105);
          delay(61000);  

        }
        else if(y==6)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0106);
          delay(61000); 

        }
        else if(y==7)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0107);
          delay(61000);  

        }
        else if(y==8)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0108);
          delay(61000);  

        }
        else if(y==9)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0109);
          delay(61000); 

        }
        else if(y==10)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010A);
          delay(61000);  
 
        }
        else if(y==11)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010B);
          delay(61000);  
   
        }
        else if(y==12)
        {
    
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010C);
          delay(61000); 
     
        }
        else if(y==13)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010D);
          delay(61000);  
   
        }
        else if(y==14)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010E);
          delay(61000);  

        }
        else if(y==15)
        {
     
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010F);
          delay(61000);  

        }
        else if(y==16)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0110);
          delay(61000); 

        }
        else if(y==17)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0111);
          delay(61000);  

        }
        else if(y==18)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0112);
          delay(61000);  

        }
        else if(y==19)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0113);
          delay(61000);  

        }
        else if(y==20)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0114);
          delay(61000);  

        }
        else if(y==21)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0115);
          delay(61000); 

        }
        else if(y==22)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0116);
          delay(61000);  
 
        }
        else if(y==23)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0117);
          delay(61000); 
   
        }
        else if(y==24)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0118);
          delay(61000);  

        }
        else if(y==25)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0119);
          delay(61000);  
      
        }
        else if(y==26)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011A);
          delay(61000);  

        }
        else if(y==27)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011B);
          delay(61000);  
   
        }
        else if(y==28)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011C);
          delay(61000);  
   
        }
        else if(y==29)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011D);
          delay(61000); 
  
        }
        else if(y==30)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011E);
          delay(61000); 

        }
        else if(y==31)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011F);
          delay(61000);  

        }
        else if(y==32)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0120);
          delay(61000);  

        }
        else if(y==33)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0121);
          delay(61000);  
   
        }
        else if(y==34)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0122);
          delay(61000);  
          
        }
        else if(y==35)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0123);
          delay(61000);
  
        }
        else if(y==36)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0124);
          delay(61000);
        
        }
        else if(y==37)
        {
      
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0125);
          delay(61000);  
      
        }
        digitalWrite(pin_4,HIGH);
        
      }
    }
    break;
  case 7:
    file = SD.open("sabtu.csv", FILE_READ);
    if (!file) {
    //Serial.println("open error");
    return;
    }
    lcd.setCursor (0, 0); // go to start of 2nd line
    lcd.print("Sabtu          ");
    while (readVals(&x, &y)) {
      
      if(konversi==x)
      {
        digitalWrite(pin_4,LOW);
        delay(3000);
        if(y==1)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0101);  
          delay(61000);

        }
        else if(y==2)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0102);
          delay(61000);  
  
        }
        else if(y==3)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0103);
          delay(61000);  

        }
        else if(y==4)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0104);
          delay(61000); 
  
        }
        else if(y==5)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0105);
          delay(61000);  

        }
        else if(y==6)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0106);
          delay(61000); 

        }
        else if(y==7)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0107);
          delay(61000);  

        }
        else if(y==8)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0108);
          delay(61000);  

        }
        else if(y==9)
        {
          
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0109);
          delay(61000); 

        }
        else if(y==10)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010A);
          delay(61000);  
 
        }
        else if(y==11)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010B);
          delay(61000);  
   
        }
        else if(y==12)
        {
    
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010C);
          delay(61000); 
     
        }
        else if(y==13)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010D);
          delay(61000);  
   
        }
        else if(y==14)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010E);
          delay(61000);  

        }
        else if(y==15)
        {
     
          sendCommand(CMD_PLAY_WITHFOLDER, 0X010F);
          delay(61000);  

        }
        else if(y==16)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0110);
          delay(61000); 

        }
        else if(y==17)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0111);
          delay(61000);  

        }
        else if(y==18)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0112);
          delay(61000);  

        }
        else if(y==19)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0113);
          delay(61000);  

        }
        else if(y==20)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0114);
          delay(61000);  

        }
        else if(y==21)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0115);
          delay(61000); 

        }
        else if(y==22)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0116);
          delay(61000);  
 
        }
        else if(y==23)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0117);
          delay(61000); 
   
        }
        else if(y==24)
        {
       
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0118);
          delay(61000);  

        }
        else if(y==25)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0119);
          delay(61000);  
      
        }
        else if(y==26)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011A);
          delay(61000);  

        }
        else if(y==27)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011B);
          delay(61000);  
   
        }
        else if(y==28)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011C);
          delay(61000);  
   
        }
        else if(y==29)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011D);
          delay(61000); 
  
        }
        else if(y==30)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011E);
          delay(61000); 

        }
        else if(y==31)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X011F);
          delay(61000);  

        }
        else if(y==32)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0120);
          delay(61000);  

        }
        else if(y==33)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0121);
          delay(61000);  
   
        }
        else if(y==34)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0122);
          delay(61000);  
          
        }
        else if(y==35)
        {
         
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0123);
          delay(61000);
  
        }
        else if(y==36)
        {
        
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0124);
          delay(61000);
        
        }
        else if(y==37)
        {
      
          sendCommand(CMD_PLAY_WITHFOLDER, 0X0125);
          delay(61000);  
      
        }
        digitalWrite(pin_4,HIGH);
        
      }
    }
    break;
  }

  lcd.setCursor (0, 1); // go to start of 2nd line
  lcd.print(gabungan);

  //delay(15000);

  

}
void loop()
{
  count++;
  displayTime(); // display the real-time clock data on the Serial Monitor,
  delay(1000); // every second

  if(count>20)
  {
    resetFunc();
  }
  
}

void sendCommand(int8_t command, int16_t dat)
{
 delay(20);
 Send_buf[0] = 0x7e; //starting byte
 Send_buf[1] = 0xff; //version
 Send_buf[2] = 0x06; //the number of bytes of the command without starting byte and ending byte
 Send_buf[3] = command; //
 Send_buf[4] = 0x00;//0x00 = no feedback, 0x01 = feedback
 Send_buf[5] = (int8_t)(dat >> 8);//datah
 Send_buf[6] = (int8_t)(dat); //datal
 Send_buf[7] = 0xef; //ending byte
 for(uint8_t i=0; i<8; i++)//
 {
   mySerial.write(Send_buf[i]) ;//send bit to serial mp3
   Serial.print(Send_buf[i],HEX);//send bit to serial monitor in pc
 }
 Serial.println();
}

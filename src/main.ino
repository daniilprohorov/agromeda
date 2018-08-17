#include <SPI.h>
#include <Wire.h>
#include "DHT.h"

#define DHTPIN D6 
#define DHTTYPE DHT22

// max7219 registers
#define MAX7219_REG_NOOP         0x0
#define MAX7219_REG_DIGIT0       0x1
#define MAX7219_REG_DIGIT1       0x2
#define MAX7219_REG_DIGIT2       0x3
#define MAX7219_REG_DIGIT3       0x4
#define MAX7219_REG_DIGIT4       0x5
#define MAX7219_REG_DIGIT5       0x6
#define MAX7219_REG_DIGIT6       0x7
#define MAX7219_REG_DIGIT7       0x8
#define MAX7219_REG_DECODEMODE   0x9
#define MAX7219_REG_INTENSITY    0xA
#define MAX7219_REG_SCANLIMIT    0xB
#define MAX7219_REG_SHUTDOWN     0xC
#define MAX7219_REG_DISPLAYTEST  0xF

//pin 16 can't interrupt, so use it as SPI chip select
#define CS_PIN D0
//interrupt pin for sqw
#define INTERRUPT_PIN D3
#define PUMP_PIN D8 
#define LED_PIN A0 
#define DS3231_ADDRESS 0x68


int var_last_num_int = 0;
int time;

boolean ledOn = false;
boolean pumpOn = false;

DHT dht(DHTPIN, DHTTYPE);
float t; //temp
float h; //humidity

byte port_state = 3;

boolean timeI = false;

void spiSend (const byte reg, const byte data);
void initDisplay();
void initClock();
void initDHT();
void readDHT();
void printTemp();
void printDHT();
void TimeInterrupt();
void pinWrite(byte pin, byte state);
byte trWrite(byte c);
byte trRead(byte c);
int readTime();
void setDS3231(byte hour, byte minute, byte second, byte week, byte day, byte month, byte year);


void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200); 
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    initDisplay();
    initClock();
    initDHT();
    
    //interrupt for clock
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), TimeInterrupt, FALLING);
    //setDS3231(11, 39, 0, 6, 14, 7, 18);
}
void pinWrite(byte pin, byte state) {
    //port_state = port_state | ~(~state<<pin);
    if(state == 0){
        port_state = ~(1 << pin) & port_state;
    }
    else 
        port_state = (1 << pin) | port_state;
    Wire.beginTransmission(0x20);
    Wire.write(0x20);   
    Wire.write(port_state);
    Wire.endTransmission();
    
}
void TimeInterrupt(){

    time = readTime();
  /*  Serial.print("\nTime ");
    Serial.print(time / 10000);
    Serial.print(":");
    Serial.print(time / 100 % 100 );
    Serial.print(":");
    Serial.println(time % 100 );
*/
    if (timeI == false){
        printNum(time/100);
    }
    else {
        printNum((int)t);
    }
    //pump
    if (((((time/100) % 100) % 1) == 0 ) && ((time % 100) <= 30)){
        pinWrite(0, 1);    
    }
    else {
     
       pinWrite(0, 0); 
    }
    //led
    if ((time / 10000 <= 21) && (time / 10000 >= 8)) {
        pinWrite(1, 1);
        pinWrite(2, 1);
    }
    else {
        pinWrite(1, 0);
        pinWrite(2, 0);
    }

    if ((time % 100) % 10 == 0 ) {
        readDHT();
        timeI = !timeI; 
    } 
}

void spiSend (const byte reg, const byte data) {
  // enable the line
  digitalWrite(CS_PIN, 0);
  // now shift out the data
  SPI.transfer (reg);
  SPI.transfer (data);
  digitalWrite (CS_PIN, 1);

}
void initClock(){
    Wire.begin();
    Wire.beginTransmission(DS3231_ADDRESS);
    //turn on sqw pin 
    Wire.write(0x0E);
    Wire.write(0b01000000);

    Wire.endTransmission();
}
void initDHT() {

    dht.begin();
    delay(2000);
}
void readDHT() {

    float h_local;
    float t_local;
        h_local = dht.readHumidity();
        t_local = dht.readTemperature();
        if(!(isnan(h_local) || isnan(t_local))) {
            t = t_local; 
            h = h_local;
        }        
        else 
            Serial.println("ERROR DHT");

}
void printDHT() {
    //printNum((int)t);
    
}
//transfer from 10-bin to dec
byte trRead(byte c)
{
    byte ch = ((c>>4)*10+(0x0F&c));
    return ch;

}
//transfer from dec to 10-bin
byte trWrite(byte c)
{
    byte ch = ((c/10)<<4)|(c%10);
    return ch;
}

int readTime(){
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 7);
    byte second = trRead(Wire.read());
    byte minute = trRead(Wire.read());
    byte hour = trRead(Wire.read()&0b111111);
    byte week = trRead(Wire.read());
    byte day = trRead(Wire.read());
    byte month = trRead(Wire.read());
    byte year = trRead(Wire.read());
    
    return (hour * 10000) + (minute * 100) + second;
}

void setDS3231(byte hour, byte minute, byte second, byte week, byte day, byte month, byte year) {

    Wire.beginTransmission(DS3231_ADDRESS);    
    Wire.write(0x00);
  
    Wire.write(trWrite(second)); 
    Wire.write(trWrite(minute)); 
    Wire.write(trWrite(hour)); 
    Wire.write(trWrite(week)); 
    Wire.write(trWrite(day)); 
    Wire.write(trWrite(month)); 
    Wire.write(trWrite(year)); 

    Wire.write(0x00);
    Wire.endTransmission();
    
}
void initDisplay() { 
        pinMode(CS_PIN, OUTPUT);
        digitalWrite (CS_PIN, 1);
        SPI.begin ();
        SPI.setDataMode(SPI_MODE0);
        SPI.setClockDivider(SPI_CLOCK_DIV128);
        spiSend(MAX7219_REG_SCANLIMIT, 3);   // show only 4 digits
        spiSend(MAX7219_REG_DECODEMODE, 1);  // using digits
        spiSend(MAX7219_REG_DISPLAYTEST, 0); // no display test
        spiSend(MAX7219_REG_INTENSITY, 15);   // character intensity: range: 0 to 15
        spiSend(MAX7219_REG_SHUTDOWN, 1);    // not in shutdown mode (ie. start it up)
        spiSend(MAX7219_REG_DECODEMODE, 0x0F);  // using digits

        for(byte digit = 1; digit <= 4; digit++)
            spiSend(digit, 0);  // обнуляем

        delay(1000);
        spiSend(MAX7219_REG_SHUTDOWN, 0);    // sleep 
        delay(1000);
        pinMode(CS_PIN, OUTPUT);
        digitalWrite (CS_PIN, 1);
    }    
void printNum(int num){
        if(num != var_last_num_int){ 
            var_last_num_int = num;
            spiSend(MAX7219_REG_SCANLIMIT, 3);   // show only 4 digits
            spiSend(MAX7219_REG_DECODEMODE, 1);  // using digits
            spiSend(MAX7219_REG_DISPLAYTEST, 0); // no display test
            spiSend(MAX7219_REG_INTENSITY, 15);   // character intensity: range: 0 to 15
            spiSend(MAX7219_REG_SHUTDOWN, 1);    // not in shutdown mode (ie. start it up)
            spiSend(MAX7219_REG_DECODEMODE, 0x0F);  // using digits
        
            for(int digit = 4; digit >= 1; digit--) {
                spiSend(digit, num % 10);
                num /= 10;
            
            }
            delay(50);
        }        
}

void printTemp() {
            int tmp = (int)t;

            spiSend(MAX7219_REG_DECODEMODE, 0x00);  // using digits
            spiSend(MAX7219_REG_SHUTDOWN, 1);    // not in shutdown mode (ie. start it up)
            spiSend(MAX7219_REG_DIGIT2, 0b00100011); 
            spiSend(MAX7219_REG_DIGIT3, 0b01100011); 

            spiSend(MAX7219_REG_DECODEMODE, 0x0F);  // using digits
            for(int digit = 2; digit >= 1; digit--) {
                spiSend(digit, tmp % 10);
                tmp /= 10;
            
            }

            delay(50);
        }        


void loop() {
}

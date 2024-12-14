#include <LiquidCrystal.h>
#include "DHT.h"
#include "RTClib.h"
#define DHTPIN 7
#define DHTTYPE DHT11
#define ENABLE 8
#define DIRA 9

#define switched true
#define debounce 10

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
DHT dht (DHTPIN, DHT11); 
int resval = 0;  
//int respin = A5;
bool fanIndicator;

RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Pointers to Use portA/pinA
  volatile unsigned int* portA = (unsigned int) 0x22;
  volatile unsigned int* pinA = (unsigned int) 0x20;
  volatile unsigned int* ddrA = (unsigned int) 0x21;

//Pointers to Use portH
  volatile unsigned int* portH = (unsigned int) 0x102;
  volatile unsigned int* ddrH = (unsigned int) 0x101;

//ADC Pointers
  volatile unsigned char* my_ADMUX = 0x7C;
  volatile unsigned char* my_ADCSRB = 0x7B;
  volatile unsigned char* my_ADCSRA = 0x7A;
  volatile unsigned int* my_ADC_DATA = 0x78;

  //Enable state Flag
  bool enable;

void setup() { 
  dht.begin();
  lcd.begin(16, 2);
  //pinMode(ENABLE, OUTPUT);
  //pinMode(DIRA, OUTPUT);
  Serial.begin(9600);

  if (! rtc.begin()) {
   Serial.println("Couldn't find RTC");
   while (1);
 }
 if (! rtc.isrunning()) {
   Serial.println("RTC is NOT running!");
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 }
 //set ddrA bit 0 for input, and bits 1-4 for output
 *ddrA &= 0xFE;
 *ddrA |= 0x1E;
 //set ddrH bits 5-6 for output
 *ddrH |= 0x60;

  // setup the ADC
  adc_init();
  //clear enable flag
  enable = 0;
} 
 
void loop() { 

  if(read_button() == switched)
  {
    enable ^= 1;
    if(enable == 0)
    {
      //Disabled state, turn off LEDs, set yellow LED
      *portA &= 0xE1;
      *portA |= 0x10;
      //Turn off Fan
      *portH &= 0xDF;
      //Write Disable message to LCD
      lcd.setCursor(0, 0);
      lcd.print("Disabled");
    }
    else if(enable == 1)
    {
      while(read_button() != switched)
      {
          float humi = dht.readHumidity();
          float tempC = dht.readTemperature();
          
          lcd.clear();
          if (isnan(humi) || isnan(tempC)) {
            lcd.setCursor(0, 0);
            lcd.print("Failed");
            *portH &= 0xDF;
            //Enter error state
            //Write non-red LEDs low
            *portA &= 0xE5;
            //Write red LED high
            *portA |= 0x04;
            
          } else {
            

            lcd.setCursor(0, 0); 
            lcd.print("Temp:");
            lcd.print(tempC);     
            lcd.print((char)223); 
            lcd.print("C");

            lcd.setCursor(0, 1);  
            lcd.print("Humi:");
            lcd.print(humi);    
            lcd.print("%");
          }

          if (tempC >= 23.5){
            //Enter running state
            //Write non-blue LEDs low
            *portA &= 0xE2;
            //Write blue LED high
            *portA |= 0x02;

          //digitalWrite(ENABLE, HIGH);
          //Write portH bit 5 high
          *portH |= 0x20;
          //digitalWrite(DIRA, HIGH);
          //Write portH bit 6 high
          *portH |= 0x40;

          fanIndicator = true;
          }
          else if(tempC < 23.5){
            //Enter idle state
            //Write non-green LEDs low
            *portA &= 0xE8;
            //Write green LED high
            *portA |= 0x08;
            //digitalWrite(ENABLE, LOW);
            //write portH bit 5 low
            *portH &= 0xDF;

            fanIndicator = false;  
          }
            
          
          lcd.setCursor(12, 1); 
          //read from adc
          resval = adc_read(5);
          //resval = analogRead(respin); 
          if (resval<=195){ lcd.println("EMPT"); 
          } 
          else if (resval>195 && resval<=270){ lcd.println(" LOW"); 
          } 
          else if (resval>270 && resval<=310){ lcd.println(" MED"); 
          } else if (resval>310){ 
            lcd.println("HIGH"); 
          }

        DateTime now = rtc.now();
        if(fanIndicator == true){
          Serial.print(now.year(), DEC);
          Serial.print('/');
          Serial.print(now.month(), DEC);
          Serial.print('/');
          Serial.print(now.day(), DEC);
          Serial.print(" (");
          Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
          Serial.print(") ");
          Serial.print(now.hour(), DEC);
          Serial.print(':');
          Serial.print(now.minute(), DEC);
          Serial.print(':');
          Serial.print(now.second(), DEC);
          Serial.println();
        }
        if(fanIndicator == false){
          Serial.print("Motor is off");
          Serial.print(now.year(), DEC);
          Serial.print('/');
          Serial.print(now.month(), DEC);
          Serial.print('/');
          Serial.print(now.day(), DEC);
          Serial.print(" (");
          Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
          Serial.print(") ");
          Serial.print(now.hour(), DEC);
          Serial.print(':');
          Serial.print(now.minute(), DEC);
          Serial.print(':');
          Serial.print(now.second(), DEC);
          Serial.println();
        }
      }
    }
  }
  delay(1000);
}

void adc_init()
{
  // set up the A register
  *my_ADCSRA |= 0x80; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0xDF; // clear bit 5 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0xF7; // clear bit 3 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0xF8; // clear bit 2-0 to 0 to set prescaler selection to slow reading
  
  // set up the B register
  *my_ADCSRB &= 0xF7; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0xF8; // clear bit 2-0 to 0 to set free running mode
  
  // set up the MUX Register
  *my_ADMUX  &= 0x7F; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0x40; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0xDF; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0xE0; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // reset the channel and gain bits
  *my_ADMUX &= 0xE0;
  
  // clear the channel selection bits
  *my_ADCSRB &= 0xF8;
  
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    
    // set MUX bit 
    *my_ADCSRB |= 0x08;
  }
  
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  
  // wait for the conversion to complete
  while(*my_ADCSRA & 0x40);
  
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

bool read_button() 
{
  
  // static variables because we need to retain old values between function calls
  static bool     switching_pending = false;
  static long int elapse_timer;
 
    if (*pinA & 0x01) 
    {
      // switch is pressed, so start/restart wait for button relealse, plus end of debounce process
      switching_pending = true;
      elapse_timer = millis(); // start elapse timing for debounce checking
    }
    if (switching_pending && !(*pinA & 0x01)) {
      // switch was pressed, now released, so check if debounce time elapsed
      if (millis() - elapse_timer >= debounce) {
        // dounce time elapsed, so switch press cycle complete
        switching_pending = false;             // reset for next button press interrupt cycle
       // interrupt_process_status = !triggered; // reopen ISR for business now button on/off/debounce cycle complete
        return switched;                       // advise that switch has been pressed
      }
    }
     return !switched; // either no press request or debounce period not elapsed
}

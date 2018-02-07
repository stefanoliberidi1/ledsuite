///////////////////////////////////////////////////////////////////////
//#define FASTLED_ALLOW_INTERRUPTS 0

//#include arduino.h
//#include <power
#include "FastLED.h"

// #include "String.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

//numero strisce
#define NUM_STRIPS 7 //

//led X striscia
#define NUM_LEDS_PIN1 16 //testa
#define NUM_LEDS_PIN2 34 //br dx
#define NUM_LEDS_PIN3 19 //petto
#define NUM_LEDS_PIN4 33 //br sx
#define NUM_LEDS_PIN5 40 //gb sx
#define NUM_LEDS_PIN6 40 //gb dx
#define NUM_LEDS_PIN7 10 //

#define VOLTS 5
#define MILLIAMPERE 1000


/*
  PARAMETRI DI RETE
*/

//CESCO!!!!!
//const char* ssid = "XHOME3G";
//const char* password = "9dylan9dog9";

//ASUS!!!!!
const char* ssid = "ASUS";
const char* password = "nodemcul";

//CASA PRIBAZ
//const char* ssid = "FASTWEB-1-4A43E3";
//const char* password = "D9D28FE40A";

//CAPANNO
//const char* ssid = "FASTWEB-1-F73F7F";
//const char* password = "11826B4F5D";

ESP8266WebServer server(80);

////////////////////////////////////////////////////////
/*
    START NEO PATTERN CLASS
*/
////////////////////////////////////////////////////////

// Pattern types supported:
enum  pattern { NONE, STROBE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE, SPARKLE, FIRE, REFILL };
//                0     1            2              3            4         5       6      7       8      9//

// Patern directions supported:
enum  direction { FORWARD, REVERSE };
//                   0        1       //

////////////////////////////////////////////////////////

// NeoPattern Class -
class NeoPatterns
{
  public:

    // Member Variables:
    CRGB *leds; //vettore di led
    int Num_Leds;
    CLEDController *controller;
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    CRGB Color1, Color2;  // What colors are in use
    int TotalSteps;  // total number of steps in the pattern
    int Index;  // current step within the pattern
    void (*OnComplete)();  // Callback on completion of pattern
    byte heat[100]; //usato solo da FIRE
    int brightness; //luminosità default. usato in showStrip 
    

    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(CLEDController *ctrl_strip, void (*callback)())
    {
      controller = ctrl_strip;
      leds = controller->leds();
      Num_Leds = controller->size();
      controller->clearLeds(Num_Leds);
      OnComplete = callback;
      brightness=255;
      setAll(0, 0, 0);
    }

    void setCompleteFunction( void (*callback)()) {
      OnComplete = callback;
    }


    void setPixel(int Pixel, byte red, byte green, byte blue) {
      //Serial.printf("setPixel i=%i \n",Pixel);
      leds[Pixel].r = red;
      leds[Pixel].g = green;
      leds[Pixel].b = blue;
    }

    void setAll(byte red, byte green, byte blue) {
      for (int i = 0; (i < Num_Leds); i++ )
      {
        setPixel(i, red, green, blue);
      }
      showStrip();
    }

    void showStrip() {
      //Serial.println("showstrip");
      controller->showLeds(brightness);
      //Serial.println("showstrip end");
    }

    // Update the pattern
    void Update()
    {
      //Serial.printf("millis.Interval=%i \n", millis() - lastUpdate );
      if ((millis() - lastUpdate) > Interval) // time to update
      {
        lastUpdate = millis();
        switch (ActivePattern)
        {
          case STROBE:
            StrobeUpdate();
            break;
          case RAINBOW_CYCLE:
            RainbowCycleUpdate();
            break;
          case THEATER_CHASE:
            TheaterChaseUpdate();
            break;
          case COLOR_WIPE:
            ColorWipeUpdate();
            break;
          case SCANNER:
            ScannerUpdate();
            break;
          case FADE:
            FadeUpdate();
            break;
          case SPARKLE:
            SparkleUpdate();
            break;
          case FIRE:
            FireUpdate();
            break;
          case REFILL:
            RefillUpdate();
            break;
          default:
            break;
        }
      }
    }

    // Increment the Index and reset at the end
    void Increment()
    {
      //if (ActivePattern = NONE) return;
      if (Direction == FORWARD) {
        //Serial.println("dir =F");
        Index++;
        //Serial.printf("INDEX =%i",Index);
        if (Index >= TotalSteps)
        {
          Index = 0;
          if (OnComplete != NULL)
          {
            OnComplete(); // call the comlpetion callback

          }
        }
      }
      else { // Direction == REVERSE
        //Index--;
        if (Index-- <= 0) 
        {
          Index = TotalSteps - 1;
          if (OnComplete != NULL)
          {
            OnComplete(); // call the comlpetion callback
          }
        }

      }

    }

    // Reverse pattern direction
    void Reverse()
    {
      if (Direction == FORWARD)
      {
        Direction = REVERSE;
        Index = TotalSteps - 1;
      }
      else
      {
        Direction = FORWARD;
        Index = 0;
      }
    }

     // Initialize for a Refill
    void Refill(CRGB color1, int interval, direction dir = FORWARD)
    {
      ActivePattern = REFILL;
      Color1 = color1;
      Interval = interval;
      Index = 0;
      Direction = dir;
      TotalSteps = Num_Leds;
      brightness=255;
      setAll(0, 0, 0);
    }

    void RefillUpdate(){
      //code
      int i;   
      i=Index;
      //Serial.printf("refupdt-i= ");
      //Serial.println(i);
      //Serial.printf("refupdt-TotalSteps= ");
      //Serial.println(TotalSteps);
      setPixel(i, Color1.r, Color1.g, Color1.b);
      showStrip();
      if (isLastToRefill()){
        //Serial.println("IsLast!");
        TotalSteps--;
      }else{
        setPixel(i, 0, 0, 0);
      }
      Increment();
    }
    
    int isLastToRefill(){    
      if (Direction == FORWARD){
        if  ( ( Index >= 0 ) && (TotalSteps - Index == 1) )
          return true;
        else
          return false;   
      }else{
        if  ( Num_Leds - TotalSteps - Index == 0 ){
          Index=0;
          return true;
        }
        else
        return false;   
      }
    }




    // Initialize for a Fire
    void Fire(int interval, direction dir = FORWARD)
    {
      ActivePattern = FIRE;
      Interval = interval;
      Index = 0;
      Direction = dir;
      TotalSteps = 255;
      brightness=120;
      setAll(0, 0, 0);

    }
    // Update the Fire Cycle Pattern
    void FireUpdate()
    {

      if (Direction == FORWARD) {
        FireFwd(55,120);
      } else {
        FireRev(55,120);
      }
      //
      showStrip();
      Increment();
    }



    void FireRev(int Cooling, int Sparking) {

      int cooldown;

      // Step 1.  Cool down every cell a little
      for ( int i = Num_Leds-1; i >= 0; i--) {
        cooldown = random(0, ((Cooling * 10) / Num_Leds) + 2);

        if (cooldown > heat[i]) {
          heat[i] = 0;
        } else {
          heat[i] = heat[i] - cooldown;
        }
      }

      // Step 2.  Heat from each cell drifts 'up' and diffuses a little
      for ( int k = 2; k < Num_Leds; k++) {
        heat[k] = (heat[k - 2] + heat[k - 1] + heat[k - 1]) / 3;
      }

      // Step 3.  Randomly ignite new 'sparks' near the bottom
      if ( random(255) < Sparking ) {
        int y = random(Num_Leds-8, Num_Leds-1); 
        heat[y] = heat[y] + random(160, 255);
        //heat[y] = random(160,255);
      }

      // Step 4.  Convert heat to LED colors
      for ( int j = Num_Leds-1; j >=0; j--) {
        setPixelHeatColor(j, heat[j] );
      }

    }

    void FireFwd(int Cooling, int Sparking) {

      int cooldown;

      // Step 1.  Cool down every cell a little
      for ( int i = 0; i < Num_Leds; i++) {
        cooldown = random(0, ((Cooling * 10) / Num_Leds) + 2);

        if (cooldown > heat[i]) {
          heat[i] = 0;
        } else {
          heat[i] = heat[i] - cooldown;
        }
      }

      // Step 2.  Heat from each cell drifts 'up' and diffuses a little
      for ( int k = Num_Leds - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
      }

      // Step 3.  Randomly ignite new 'sparks' near the bottom
      if ( random(255) < Sparking ) {
        int y = random(7);
        heat[y] = heat[y] + random(160, 255);
        //heat[y] = random(160,255);
      }

      // Step 4.  Convert heat to LED colors
      for ( int j = 0; j < Num_Leds; j++) {
        setPixelHeatColor(j, heat[j] );
      }
      //showStrip();
      //delay(SpeedDelay);
    }

    void setPixelHeatColor (int Pixel, byte temperature) {
      // Scale 'heat' down from 0-255 to 0-191
      byte t192 = round((temperature / 255.0) * 191);

      // calculate ramp up from
      byte heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252

      // figure out which third of the spectrum we're in:
      if ( t192 > 0x80) {                    // hottest
        setPixel(Pixel, 255, 255, heatramp);
      } else if ( t192 > 0x40 ) {            // middle
        setPixel(Pixel, 255, heatramp, 0);
      } else {                               // coolest
        setPixel(Pixel, heatramp, 0, 0);
      }
    }

    // Initialize for a Sparkle
    void Sparkle(CRGB color1, int interval, direction dir = FORWARD)
    {
      ActivePattern = SPARKLE;
      Interval = interval;
      Color1 = color1;
      Index = 0;
      Direction = dir;
      TotalSteps = 255;
      brightness=255;
    }

    // Update the Sparkle Cycle Pattern
    void SparkleUpdate()
    {
      // TO DO
      int pxl = random(Num_Leds);
      if (Direction == FORWARD) {
        setPixel(pxl, Color1.red, Color1.green, Color1.blue);
      } else {
        setPixel(pxl, 0, 0, 0);
      }
      //
      showStrip();
      //
      if (Direction == FORWARD) {
        setPixel(pxl, 0, 0, 0);
      } else {
        setPixel(pxl, Color1.red, Color1.green, Color1.blue);
      }

      //Serial.println("pre increment");
      Increment();
    }

    // Initialize for a Fade
    void Fade(CRGB color1, CRGB color2, int steps, int interval, direction dir = FORWARD)
    {
      ActivePattern = FADE;
      Interval = interval;
      TotalSteps = steps;
      Color1 = color1;
      Color2 = color2;
      Index = 0;
      Direction = dir;
      brightness=255;
    }

    // Update the Fade Pattern
    void FadeUpdate()
    {
      // Calculate linear interpolation between Color1 and Color2
      // Optimise order of operations to minimize truncation error
      //uint8_t red = (   ( Red(Color1) * (TotalSteps - Index)  ) + (Red(Color2) * Index)) / TotalSteps;
      //uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
      //uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;

      byte red = ( (Color1.r * (TotalSteps - Index))  + Color2.r * Index ) / TotalSteps;
      byte green = ( (Color1.g * (TotalSteps - Index))  + Color2.g * Index ) / TotalSteps;
      byte blue = ( (Color1.b * (TotalSteps - Index))  + Color2.b * Index ) / TotalSteps;

      setAll(red, green, blue);
      showStrip();
      Increment();
    }

    // Initialize for a SCANNNER
    void Scanner(CRGB color1, long interval)
    {
      ActivePattern = SCANNER;
      Interval = interval;
      TotalSteps = (Num_Leds - 1) * 2;
      Color1 = color1;
      Index = 0;
      brightness=255;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    {
      for (int i = 0; i < Num_Leds; i++)
      {
        if (i == Index)  // Scan Pixel to the right
        {
          setPixel(i, Color1.r, Color1.g, Color1.b);
        }
        else if (i == TotalSteps - Index) // Scan Pixel to the left
        {
          setPixel(i, Color1.r, Color1.g, Color1.b);
        }
        else // Fading tail
        {
          CRGB c, d;
          c.r = leds[i].r;
          c.g = leds[i].g;
          c.b = leds[i].b;
          d = DimColor(c);
          setPixel(i, d.r, d.g, d.b);
        }
      }
      showStrip();
      Increment();
    }

    // Initialize for a RainbowCycle
    void RainbowCycle(long interval, direction dir = FORWARD)
    {
      ActivePattern = RAINBOW_CYCLE;
      Interval = interval;
      TotalSteps = 255;
      Index = 0;
      Direction = dir;
      //controller->setDither()
      brightness=100;
    }
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
      byte *c;
      for (int i = 0; i < Num_Leds; i++)
      {
        c = Wheel(((i * 256 / Num_Leds) + Index) & 255);
        setPixel(i, c[0], c[1], c[2]); 
      }
      showStrip();
      Increment();
    }

    // Initialize for a Strobe
    void Strobe(CRGB color, long interval, direction dir = FORWARD)
    {
      Color1 = color;
      CRGB black;
      black.r = 0;
      black.g = 0;
      black.b = 0;
      Color2 = black;
      ActivePattern = STROBE;
      Interval = interval;
      TotalSteps = 100;
      Index = 0;
      Direction = dir;
      brightness=255;
    }

    // Update the Strobe Cycle Pattern
    void StrobeUpdate()
    {
      //Serial.println("strobe update");
      if (Index % 2 == 0) {
        //Serial.printf("index=%i pari\n",Index);
        setAll(Color1.r, Color1.g, Color1.b);
      }
      else {
        //Serial.printf("index=%i dispari\n",Index);
        setAll(Color2.r, Color2.g, Color2.b);
      }
      //Serial.println("pre showStrip");
      showStrip();
      //Serial.println("pre increment");
      Increment();
    }

    // Initialize for a Theater Chase
    void TheaterChase(CRGB color1, CRGB color2, long interval, direction dir = FORWARD)
    {
      ActivePattern = THEATER_CHASE;
      Interval = interval;
      TotalSteps = Num_Leds;
      Color1 = color1;
      Color2 = color2;
      Index = 0;
      Direction = dir;
      brightness=255;
    }

    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
      for (int i = 0; i < Num_Leds; i++)
      {
        if ((i + Index) % 3 == 0)
        {
          setPixel(i, Color1.r, Color1.g, Color1.b);
        }
        else
        {
          setPixel(i, Color2.r, Color2.g, Color2.b);
        }
      }
      showStrip();
      Increment();
    }

    // Initialize for a ColorWipe
    void ColorWipe(CRGB color, long interval, direction dir = FORWARD)
    {
      ActivePattern = COLOR_WIPE;
      Interval = interval;
      TotalSteps = Num_Leds;
      Color1 = color;
      Index = 0;
      Direction = dir;
      brightness=255;
      setAll(0, 0, 0);
    }

    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {

      if (Direction == FORWARD) {
        //Serial.printf("ColorWupeUpdate FORWARD Index=%i TotalSteps=%i \n", Index, TotalSteps);
        setPixel(Index, Color1.r, Color1.g, Color1.b);
      }

      if (Direction == REVERSE) {
        //Serial.printf("ColorWupeUpdate REVERSE Index=%i TotalSteps=%i \n", Index, TotalSteps);
        setPixel(Index, Color1.r, Color1.g, Color1.b);
      }
      showStrip();
      Increment();
    }

    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    CRGB DimColor(CRGB color)
    {
      // Shift R, G and B components one bit to the right
      CRGB dimColor;
      dimColor.r = color.r >> 1;
      dimColor.g = color.g >> 1;
      dimColor.b = color.b >> 1;
      return dimColor;
    }

    byte * Wheel(byte WheelPos) {
      static byte c[3];

      if (WheelPos < 85) {
        c[0] = WheelPos * 3;
        c[1] = 255 - WheelPos * 3;
        c[2] = 0;
      } else if (WheelPos < 170) {
        WheelPos -= 85;
        c[0] = 255 - WheelPos * 3;
        c[1] = 0;
        c[2] = WheelPos * 3;
      } else {
        WheelPos -= 170;
        c[0] = 0;
        c[1] = WheelPos * 3;
        c[2] = 255 - WheelPos * 3;
      }

      return c;
    }
};




////////////////////////////////////////////////////////
/*
    END NEO PATTERN CLASS
*/
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////






////////////////////////////////////////////////////////

//PRE SETUP VARIABLES

////////////////////////////////////////////////////////

long lu;
long cont;


//
//////////////////////////////////////////////////////////


/////////////////////////////////////////
/////////////////////////////////////////////////////////
/*
  LUNGHEZZE STRISCIE LED
*/
//
CRGB led_array_1[NUM_LEDS_PIN1]; //braccio dx
CRGB led_array_2[NUM_LEDS_PIN2]; //braccio sx
CRGB led_array_3[NUM_LEDS_PIN3]; // busto
CRGB led_array_4[NUM_LEDS_PIN4]; // gamba dx
CRGB led_array_5[NUM_LEDS_PIN5]; //gamba sx
CRGB led_array_6[NUM_LEDS_PIN6]; //testa
CRGB led_array_7[NUM_LEDS_PIN7]; //in piÃ¹
//
/*
  CONTROLLORI STRISCIE LED
*/
CLEDController* c1 = &FastLED.addLeds<WS2812, 1, GRB>(led_array_1, NUM_LEDS_PIN1);
CLEDController* c2 = &FastLED.addLeds<WS2812, 2, GRB>(led_array_2, NUM_LEDS_PIN2);
CLEDController* c3 = &FastLED.addLeds<WS2812, 3, GRB>(led_array_3, NUM_LEDS_PIN3);
CLEDController* c4 = &FastLED.addLeds<WS2812, 7, GRB>(led_array_4, NUM_LEDS_PIN4);
CLEDController* c5 = &FastLED.addLeds<WS2812, 5, GRB>(led_array_5, NUM_LEDS_PIN5);
CLEDController* c6 = &FastLED.addLeds<WS2812, 6, GRB>(led_array_6, NUM_LEDS_PIN6);
CLEDController* c7 = &FastLED.addLeds<WS2812, 8, GRB>(led_array_7, NUM_LEDS_PIN7);


//
///////////////////////////////////////////////////////////////////

NeoPatterns *led_strips[NUM_STRIPS];

NeoPatterns strip1(c1, &stripComplete1);
NeoPatterns strip2(c2, &stripComplete2);
NeoPatterns strip3(c3, &stripComplete3);
NeoPatterns strip4(c4, &stripComplete4);
NeoPatterns strip5(c5, &stripComplete5);
NeoPatterns strip6(c6, &stripComplete6);
NeoPatterns strip7(c7, &stripComplete7);

int paramspin[NUM_STRIPS];

void stripComplete(int i) {

  switch (led_strips[i]->ActivePattern) {
      
    case COLOR_WIPE:
      //
      led_strips[i]->setAll(0, 0, 0);
      break;

    case FADE:
      //
      led_strips[i]->ActivePattern=NONE;
      led_strips[i]->setAll(led_strips[i]->Color2.r,led_strips[i]->Color2.g,led_strips[i]->Color2.b);
      break;

    case REFILL:
     // Serial.println("stripcomplete[i].refill ");
      if (led_strips[i]->Direction==FORWARD){
        if (led_strips[i]->Index == led_strips[i] ->TotalSteps ){
          led_strips[i]->setAll(led_strips[i]->Color1.red, led_strips[i]->Color1.green, led_strips[i]->Color1.blue);
          led_strips[i]->ActivePattern = NONE;
          led_strips[i]->Index=0;
          led_strips[i]->TotalSteps=led_strips[i]->Num_Leds;
         // Serial.println("strip[i] coomplete ands index updated ");
        }
      }else{
        //Serial.println("--stripComplete-refill-REVERSE");
        //Serial.print("--Index=");
        //Serial.println(led_strips[i]->Index);
        //Serial.print("--TotalSteps=");
       // Serial.println(led_strips[i]->TotalSteps);
        led_strips[i]->Index=led_strips[i]->Num_Leds-1;
        //Serial.print("--Index=");
       // Serial.println(led_strips[i]->Index);
        if ( led_strips[i]->TotalSteps==1 ){
          led_strips[i]->setAll(led_strips[i]->Color1.red, led_strips[i]->Color1.green, led_strips[i]->Color1.blue);
          led_strips[i]->ActivePattern = NONE;
          led_strips[i]->Index=0;
          led_strips[i]->TotalSteps=led_strips[i]->Num_Leds;
          //Serial.println("strip[i] -rev- coomplete  ands index updated ");
        }
      }
      break;

    default:
      break;
  }
}


void stripComplete7() {
  stripComplete(6); 
}

///////////////////////////////////////////////////////////////////
void stripComplete1() {
  stripComplete(0); 
}
//////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
void stripComplete2() {
  //Serial.println("stripComplete2 called");
  //delay(1);
  stripComplete(1);
}
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
void stripComplete3() {
  //Serial.println("stripComplete3 called");
  //delay(1);
  stripComplete(2);  
}
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
void stripComplete4() {
  //Serial.println("stripComplete4 called");
  //delay(1);
  stripComplete(3);  
}
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
void stripComplete5() {
  //Serial.println("stripComplete5 called");
  //delay(1);
  stripComplete(4);
}
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
void stripComplete6() {
  //Serial.println("stripComplete6 called");
  //delay(1);
  stripComplete(5);
}
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Initialize everything and prepare to start
void setup()
{

  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS,MILLIAMPERE);

  led_strips[0] = &strip1;
  led_strips[1] = &strip2;
  led_strips[2] = &strip3;
  led_strips[3] = &strip4;
  led_strips[4] = &strip5;
  led_strips[5] = &strip6;
  led_strips[6] = &strip7;

  Serial.begin(9600);
  Serial.println("Booting Sketch...");

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.println("WiFi.begin!");
  delay(500);
  Serial.print("Waiting for network...");
  Serial.println(ssid);
  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  //CONNECTION?????
  if (WiFi.status() == WL_CONNECTED) {

    server.on("/function", HTTP_GET, []() {

      unsigned long time1;
      unsigned long time2;
      time1 = millis();

      

      setPinFromRequest(); //da 1 a 7  -- pin[0] - pin[6]

      int r = getArgValue("r");
      int g = getArgValue("g");
      int b = getArgValue("b");
      int r2 = getArgValue("r2");
      int g2 = getArgValue("g2");
      int b2 = getArgValue("b2");

      int fd = getArgValue("FlashDelay");
      int dir = getArgValue("direction");
      int reverse = getArgValue("reverse");
      int func = getArgValue("FUNCTION");
      int numsteps = getArgValue("StrobeCount");

      CRGB new_color;
      new_color.r = r;
      new_color.g = g;
      new_color.b = b;

      CRGB color2;
      color2.r = r2;
      color2.g = g2;
      color2.b = b2;

      for (int i = 0; i < NUM_STRIPS; i++) {
        //Serial.printf("paramspin[%i] =",i);
        //Serial.println(paramspin[i]);
        if (paramspin[i] > 0) {
          //Serial.printf("paramspin[%i] =", i);
          //Serial.println(paramspin[i]);
          switch (func) {
            case NONE:
              //Serial.println("Ricevuto NONE"); //0
              led_strips[i]->setAll( (r ? r : 0), (g ? g : 0), (b ? b : 0) );
              led_strips[i]->ActivePattern = NONE;
              led_strips[i]->Interval = fd;
              break;
            case STROBE:
              //Serial.println("Ricevuto STROBE"); //1
              led_strips[i]->Strobe(new_color, fd);
              break;
            case RAINBOW_CYCLE:
              //Serial.println("Ricevuto RAINBOW_CYCLE"); //2
              led_strips[i]->RainbowCycle(fd);
              if (reverse == REVERSE) {
                //Serial.println("Ricevuto REVERSE on RAINBOW_CYCLE");
                led_strips[i]->Reverse();
              }
              break;
            case THEATER_CHASE:
              led_strips[i]->TheaterChase(new_color, color2, fd);
              //Serial.println("Ricevuto THEATER_CHASE"); //3

              if (reverse == REVERSE) {
                //Serial.println("Ricevuto REVERSE on THEATER_CHASE");
                led_strips[i]->Reverse();
              }
              break;

            case COLOR_WIPE:
              //Serial.println("Ricevuto COLOR_WIPE"); //4
              led_strips[i]->ColorWipe(new_color, fd);
              if (reverse == REVERSE) {
                //Serial.println("Ricevuto REVERSE on COLOR_WIPE");
                led_strips[i]->Reverse();
              }
              break;
            case SCANNER:
              //Serial.println("Ricevuto SCANNER"); //5
              led_strips[i]->Scanner(new_color, fd);
               
              if (reverse == REVERSE) {
                //Serial.println("Ricevuto REVERSE on SCANNER");
                led_strips[i]->Reverse();
              }
              break;
            case FADE:
              //Serial.println("Ricevuto FADE"); //6
              //void Fade(CRGB color1, CRGB color2, long steps, long interval, direction dir = FORWARD)
              led_strips[i]->Fade(new_color, color2, numsteps, fd);
              break;

            case SPARKLE:
              //Serial.println("Ricevuto SPARKLE"); //7
              led_strips[i]->Sparkle(new_color, fd);
              if (reverse == REVERSE) {
                //Serial.println("Ricevuto REVERSE on SPARKLE");
                led_strips[i]->Reverse();
              }
              break;
              case FIRE:
                //Serial.println("Ricevuto FIRE"); //8
                led_strips[i]->Fire(fd);
                if (reverse == REVERSE) {
                  //Serial.println("Ricevuto REVERSE on FIRE");
                  led_strips[i]->Reverse();
              }
              break; 
              case REFILL:
                led_strips[i]->Refill(new_color,fd,FORWARD);
                if (reverse == REVERSE) {
                  //Serial.println("Ricevuto REVERSE on REFILL"); //9
                  led_strips[i]->Reverse();
                }
              break;
            default:
              break;
          }//fine switch
        }//fine if paramspin[i]>0

      }//fine ciclo strisce

      String serverIndex = "OK";
      
      //time2 = millis();
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/html", serverIndex);
      //server.client().stop();
      //writeESPinfo();
      //server.client().flush();
      //yeld();
    });
    ///
    ///


    server.on("/form", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      String serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
      server.send(200, "text/html", serverIndex);
      
    });
    server.on("/rst", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      String serverIndex = "Rebooting System";
      server.send(200, "text/html", serverIndex);
      //server.client().stop();
      //server.client().flush();
      delay(100);
      ESP.restart();
    });
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError()) ? "upload FAIL" : "upload OK");
      delay(100);
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      //yield();
    });

    server.begin();

    //MDNS.addService("http", "tcp", 80);

    Serial.print("Connect to http://");
    Serial.print(WiFi.localIP());
    Serial.println("/form in your browser\n");
    delay(1000);
  } else {
    Serial.println("WiFi Failed");
  }

  lu = millis();
  Serial.println("end setup");
}

// Main loop
void loop()
{
  for (int i = 0; i < NUM_STRIPS; i++) {
    led_strips[i]->Update();
  }
  //yeld();//////////////////////////////////////////////////////////////////////////////
  server.handleClient();
}


int getArgValue(String name)
{
  for (int i = 0; i < server.args(); i++)
    if (server.argName(i) == name)
      return server.arg(i).toInt();
  return -1;
}

String getArgStrValue(String name)
{
  for (int i = 0; i < server.args(); i++)
    if (server.argName(i) == name)
      return server.arg(i);
  return "";
}

void setPinFromRequest() {
  for (int i = 0; i < NUM_STRIPS; i++) {
    String pinname = String("pin");
    pinname += i;
    paramspin[i] = getArgValue(pinname);
    //Serial.printf("paramspin[%i]=%i\n", i, paramspin[i]);
  }
}
/*
void writeESPinfo(){
  Serial.println("----INFO----");
  Serial.print("getChipId=");
  Serial.println(ESP.getChipId());
  
  Serial.print("getCoreVersion=");
  Serial.println(ESP.getCoreVersion());
  
  Serial.print("getCpuFreqMHz=");
  Serial.println(ESP.getCpuFreqMHz());
  
  Serial.print("getCycleCount=");
  Serial.println(ESP.getCycleCount());
  
  Serial.print("getFreeHeap=");
  Serial.println(ESP.getFreeHeap());

  Serial.print("getFreeSketchSpace=");
  Serial.println(ESP.getFreeSketchSpace());
  
  Serial.print("getResetInfo=");
  Serial.println(ESP.getResetInfo());
  
  Serial.print("getResetReason=");
  
  Serial.println(ESP.getResetReason());  
  }
*/
  
  


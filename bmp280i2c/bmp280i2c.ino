// endereco do modulo 0x76 - precisa alterar na biblioteca

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp; // I2C

void setup() {
  Serial1.begin(19200);
  Serial1.println(F("BMP280 test"));

  if (!bmp.begin()) {
    Serial1.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

char c;
void loop() {
    /*Serial1.print(F("Temperature = "));
    Serial1.print(bmp.readTemperature());
    Serial.println(" *C");

    Serial1.print(F("Pressure = "));
    Serial1.print(bmp.readPressure());
    Serial1.println(" Pa");

    Serial1.print(F("Approx altitude = "));
    Serial1.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! 
     Serial1.println(" m");*/
    
if (Serial1.available()>0)
  {
   c = Serial1.read();      
   if (c == 'm')
   { 
    Serial1.print(bmp.readTemperature());
    Serial1.print(',');
    Serial1.print(bmp.readPressure());
    Serial1.print(',');
    Serial1.print(bmp.readAltitude(1023.0));
    Serial1.print(',');
    Serial1.print('\n');   
    delay(100);
   }
   }
}

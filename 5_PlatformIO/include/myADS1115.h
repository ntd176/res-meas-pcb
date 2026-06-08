#ifndef my_ADS1115
#define my_ADS1115

#include <Arduino.h>
#include <Wire.h>

/*
  * 0x48 --> I2C Addr
  * 0x00 --> Measure Result
  * 0x01 --> Gain, channel, sample rate
*/
#define ADS_ADDR 0x48
#define REG_CONV 0x00
#define REG_CONF 0x01

class MyADS1115 {
  private:
    // extract 7.1 table, 15 page (datasheet ads1115)
    const float LSB_Gain_1 = 0.125f; // FSR = +/- 4.096V (mV/count unit)
    const float LSB_Gain_8 = 0.015625f; // FSR = +/- 0.512V (mV/count unit)
    
    // write function 16-bit into CONFIG register
    void writeConfig(uint16_t config){
      Wire.beginTransmission(ADS_ADDR);
      Wire.write(REG_CONF);
      Wire.write((uint8_t)(config >> 8)); // (15->8) byte
      Wire.write((uint8_t)(config & 0x00FF)); // (7->0) byte
      Wire.endTransmission();
    }

    // write 16-bit from CONV register
    int16_t readRaw(){
      Wire.beginTransmission(ADS_ADDR);
      Wire.write(REG_CONV);
      Wire.endTransmission();
      Wire.requestFrom(ADS_ADDR, 2);
      if(Wire.available()==2){
        return (int16_t)(Wire.read()<<8 | Wire.read());
      }
      return 0;
    }
  public:
    // I2C init
    bool begin(int sda=21, int scl=22){
      return Wire.begin(sda, scl);
    }
    //---------------------------------
    /*
    Meas differential wheatstone briedge (A0-A1) --> (page 25-26 datasheet ads1115)
    *Config: 0x8983 (1000_1001_1000_0011)
      * OS[15]=1 -> start meas
      * MUX[14:12]=000 -> diff AIN0 - AIN1
      * PGA[11:9]=100 -> gain 8 (+/-0.512V) LSB=15.625uV
      * MODE[8]=1 -> single shoot
      * DR[7:5]=100 -> 128 SPS (Sample per Second)
      * COMP[4:0]=00011 --> default
    */
    float readVoltage_Bridge(){
      writeConfig(0x8983);
      delay(10); //10ms
      int16_t raw=readRaw();
      return (raw*LSB_Gain_8)/1000.0f; //mV->V
    }
    //---------------------------------
    /*
    Meas Eb actual (A2)
    *Config: 0xE383 (1110_0011_1000_0011)
      * OS[15]=1 -> start meas
      * MUX[14:12]=110 -> single-ended AIN2
      * PGA[11:9]=001 -> gain 1 (+/-4.096V) LSB=125uV
      * MODE[8]=1 -> single shoot
      * DR[7:5]=100 -> 128 SPS (Sample per Second)
      * COMP[4:0]=00011 --> default
    */
    float readVoltage_Source(){
      writeConfig(0xE383);
      delay(10); 
      int16_t raw=readRaw();
      return (raw*LSB_Gain_1)/1000.0f;
    }
    //---------------------------------
    /*
    Average voltage function
    */
    float getStableVoltage(int sample=10){
      float sum = 0;
      for(int i=0; i<sample; i++){
        sum = sum + readVoltage_Bridge();
      }
      return sum/(float)sample;
    }
};
#endif
#ifndef Configuration_H
#define Configuration_H

#include "Arduino.h"


// This to On/Off the sensors
extern bool DSTemp = true ;
extern bool AirTempEnable = true;           // derived from Si7021
extern bool AirHumEnable = true;            // derived from Si7021
extern bool ElectronicsTempEnable = true;   // derived from DHT11
extern bool ElectronicsHumEnable = true;    // derived from DHT11
extern bool AirPressureEnable = true;       // derived from ADC
extern bool AccelerometerEnable = true;     // derived from MPU-6050

// This to sample the rate of the sensor of the Dallas Temperature 
extern int DSTempSamp = 400 ;

// This to sample the rate of the sensor of the Acceleromemter
extern int MPUSensorSamp = 400;


// This to On/Off the sensor of the Acceleromemter
extern bool MPUSensor = true;



#endif

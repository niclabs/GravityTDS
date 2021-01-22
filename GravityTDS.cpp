/***************************************************
 DFRobot Gravity: Analog TDS Sensor/Meter
 <https://www.dfrobot.com/wiki/index.php/Gravity:_Analog_TDS_Sensor_/_Meter_For_Arduino_SKU:_SEN0244>
 
 ***************************************************
 This sample code shows how to read the tds value and calibrate it with the standard buffer solution.
 707ppm(1413us/cm)@25^c standard buffer solution is recommended.
 
 Created 2018-1-3
 By Jason <jason.ling@dfrobot.com@dfrobot.com>
 
 GNU Lesser General Public License.
 See <http://www.gnu.org/licenses/> for details.
 All above must be included in any redistribution.
 ****************************************************/

#include <EEPROM.h>
#include "GravityTDS.h"

#define EEPROM_write(address, p) {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) EEPROM.write(address+i, pp[i]);}
#define EEPROM_read(address, p)  {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) pp[i]=EEPROM.read(address+i);}

GravityTDS::GravityTDS(int device)
{
    //this->pin = A1;
    this->voltage = 0;
    this->temperature = 25.0;
    //this->aref = 5.0;
    //this->adcRange = 1024.0;
    this->kValueAddress = 8;
    this->kValue = 1.0;
    this->mem_offset = device*DEVICE_MEM_OFFSET;
}

GravityTDS::~GravityTDS() {
}

void GravityTDS::setTemperature(float temp) {
	this->temperature = temp;
}


void GravityTDS::setKvalueAddress(int address) {
      this->kValueAddress = address;
}

void GravityTDS::begin() {
	readKValues();
}

void GravityTDS::update(float voltage /* in Volts */, float temperature) {
	this->voltage = voltage;
    if ((*(uint32_t*)&temperature) != 0xc2c80000 /* -100f binary representation in 4-byte floats*/) {
        this->temperature = temperature;
    }
	this->ecValue = voltageToEC(this->voltage);
	this->ecValue25 = compensateTemperature(this->ecValue); //temperature compensation, returns equivalent EC at 25 °C
	this->tdsValue = ecValue25 * TdsFactor;
}

float GravityTDS::getKvalue() {
	return this->kValue;
}

float GravityTDS::getTdsValue() {
	return tdsValue;
}

float GravityTDS::getEcValue() {
      return ecValue25;
}

void GravityTDS::readKValues() {
    EEPROM_read(this->kValueAddress+this->mem_offset, this->kValue);
    if(EEPROM.read(this->kValueAddress+this->mem_offset)==0xFF &&
       EEPROM.read(this->kValueAddress+1+this->mem_offset)==0xFF &&
       EEPROM.read(this->kValueAddress+2+this->mem_offset)==0xFF &&
       EEPROM.read(this->kValueAddress+3+this->mem_offset)==0xFF) {
      this->kValue = 1.0;   // default value: K = 1.0
      EEPROM_write(this->kValueAddress+this->mem_offset, this->kValue);
    }
}

float GravityTDS::voltageToEC(float voltage /* in Volts */) {
    return (133.42*voltage*voltage*voltage - 255.86*voltage*voltage + 857.39*voltage)*this->kValue;
}

bool GravityTDS::isInRange1413(float ecValue) {
    return (0<ecValue) && (ecValue<2000);
}

bool GravityTDS::isInRangeKValue(float kValue) {
    return (0.25<kValue) && (kValue<0.4);
}

float GravityTDS::compensateTemperature(float ecValue) {
    return ecValue*(1.0+0.02*(this->temperature-25.0));
}

bool GravityTDS::calibrate1413() {
    // to be called after update(voltage, temperature)
    float KValueTemp = this->ecValue25/(133.42*voltage*voltage*voltage - 255.86*voltage*voltage + 857.39*voltage);
    if (isInRange1413(ecValue25) && isInRangeKValue(KValueTemp)) {
        saveKValue(KValueTemp);
        return true;
    }
    return false;
}

void GravityTDS::saveKValue(float kValue) {
   EEPROM_write(kValueAddress+this->mem_offset, kValue);
}


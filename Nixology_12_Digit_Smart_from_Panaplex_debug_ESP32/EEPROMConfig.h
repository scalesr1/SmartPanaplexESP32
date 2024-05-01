/*
 * EEPROMConfig.h
 *
 *  Created on: Dec 6, 2017
 *      Author: Paul
 */

#ifndef LIBRARIES_CONFIGS_EEPROMCONFIG_H_
#define LIBRARIES_CONFIGS_EEPROMCONFIG_H_
#include "Arduino.h"

#include "ConfigItem.h"

class EEPROMConfig {
public:
	EEPROMConfig(BaseConfigItem &root);

	void init();
	void setDebugPrint(Print *debugPrint) {
		this->debugPrint = debugPrint;
		int marker = 0;
		EEPROM.get(0, marker);
		debugPrint->println("");
		debugPrint->println(size);
		debugPrint->println(marker);
	}
	bool commit();

private:
	Print *debugPrint;
	BaseConfigItem &root;
	int size;
	unsigned int checksum;
};


#endif /* LIBRARIES_CONFIGS_EEPROMCONFIG_H_ */

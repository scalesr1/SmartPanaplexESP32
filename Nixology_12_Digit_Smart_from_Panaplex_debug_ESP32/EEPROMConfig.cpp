/*
 * EEPROMConfig.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: Paul Andrews
 */

#include <EEPROM.h>
#include "EEPROMConfig.h"

#define DEBUG(...) { if (debugPrint) debugPrint->println(__VA_ARGS__); }

EEPROMConfig::EEPROMConfig(BaseConfigItem &root) :
	debugPrint(0),
	root(root),
	size(0),
	checksum(0)
{
	int start = sizeof(int);	// Leave room for checksum
	int i=0;
	int maxSize = root.init(start);
	checksum = root.getChecksum(0);

	size = maxSize-2;
}

void EEPROMConfig::init() {
	// Init from defaults if what is being stored has changed
	unsigned int storedChecksum = 0;
	EEPROM.get(0, storedChecksum);
	if (storedChecksum != checksum) {
		DEBUG("checksum: ")
		DEBUG(checksum)
		DEBUG("stored checksum: ")
		DEBUG(storedChecksum)
		DEBUG("Initializing EEPROM")
		root.put();
		EEPROM.put(0, checksum);
		commit();
	}
}

bool EEPROMConfig::commit() {
	DEBUG("Committing")
	return EEPROM.commit();
}

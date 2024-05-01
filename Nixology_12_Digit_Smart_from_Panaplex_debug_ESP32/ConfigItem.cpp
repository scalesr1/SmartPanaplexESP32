/*
 * Configcpp
 *
 *  Created on: Dec 6, 2017
 *      Author: Paul
 */

#include "ConfigItem.h"

String escape_json(const String &s) {
    String ret("\"");
    ret.reserve(s.length() + 10);
    const char *start = s.c_str();
    const char *end = start + strlen(s.c_str());
    for (const char *c = start; c != end; c++) {
        switch (*c) {
        case '"': ret += "\\\""; break;
        case '\\': ret += "\\\\"; break;
        case '\b': ret += "\\b"; break;
        case '\f': ret += "\\f"; break;
        case '\n': ret += "\\n"; break;
        case '\r': ret += "\\r"; break;
        case '\t': ret += "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
            	char buf[10];
            	sprintf(buf, "\\u%04x", (int)*c);
                ret += buf;
            } else {
                ret += *c;
            }
        }
    }
    ret += "\"";

	return ret;
}

template <class T>
void ConfigItem<T>::debug(Print *debugPrint) const {
	if (debugPrint != 0) {
		debugPrint->print(name);
		debugPrint->print(":");
		debugPrint->print(value);
		debugPrint->print(" (");
		debugPrint->print(maxSize);
		debugPrint->println(")");
    
	}
}

template void ConfigItem<byte>::debug(Print *debugPrint) const;
template void ConfigItem<bool>::debug(Print *debugPrint) const;
template void ConfigItem<int>::debug(Print *debugPrint) const;
template void ConfigItem<String>::debug(Print *debugPrint) const;
template void ConfigItem<float>::debug(Print *debugPrint) const;


void StringConfigItem::put() const {
	int end = start + maxSize;
	for (int i = start; i < end; i++) {
		if (i - start < value.length()) {
			EEPROM.write(i, value[i - start]);
		} else {
			EEPROM.write(i, 0);
			break;
		}
	}
}

BaseConfigItem& StringConfigItem::get() {
	value = String();
	value.reserve(maxSize+1);
	int end = start + maxSize;
	for (int i = start; i < end; i++) {
		byte readByte = EEPROM.read(i);
		if (readByte > 0 && readByte < 128) {
			value += char(readByte);
		} else {
			break;
		}
	}

	return *this;
}

 String StringConfigItem::toJSON(bool bare, const char **excludes) const
{
	return escape_json(value);
}

void CompositeConfigItem::forEach(std::function<void(BaseConfigItem&)> pFunc) {
	for (int i=0; value[i] != 0; i++) {
		value[i]->forEach(pFunc);
	}
}

void CompositeConfigItem::debug(Print *debugPrint) const {
	if (debugPrint != 0) {
		debugPrint->print(name);
		debugPrint->print(": {");
		char *sep = "";
		for (int i=0; value[i] != 0; i++) {
			debugPrint->print(sep);
			value[i]->debug(debugPrint);
			sep = ",";
		}
		debugPrint->print("}");
	}
}

String CompositeConfigItem::toJSON(bool bare, const char **excludes) const {
	String json;
	json.reserve(200);

	if (!bare) {
		json.concat("{");
	}
	char *sep = "";
	for (int i=0; value[i] != 0; i++) {
		bool exclude = false;
		for (int j=0; excludes != 0 && excludes[j] != 0; j++) {
			if (strcmp(value[i]->name, excludes[j]) == 0) {
				exclude = true;
			}
		}
		if (!exclude) {
			json.concat(sep);
			json.concat("\"");
			json.concat(value[i]->name);
			json.concat("\"");
			json.concat(":");
			json.concat(value[i]->toJSON());
			sep = ",";
		}
	}
	if (!bare) {
		json.concat("}");
	}

	return json;
}

// This is not a good idea, just return the JSON representation
String CompositeConfigItem::toString(const char **excludes) const {
	return toJSON(false, excludes);
}

void CompositeConfigItem::put() const {
	for (int i=0; value[i] != 0; i++) {
		value[i]->put();
	}
}

BaseConfigItem& CompositeConfigItem::get() {
	for (int i=0; value[i] != 0; i++) {
		value[i]->get();
	}

	return *this;
}

int CompositeConfigItem::init(int start) {
	this->start = start;

	for (int i=0; value[i] != 0; i++) {
		start = value[i]->init(start);
	}

	this->maxSize = start - this->start;

	return start;
}

BaseConfigItem* CompositeConfigItem::get(const char *name) {
	if (strcmp(name, this->name) == 0) {
		return this;
	}

	for (int i=0; value[i] != 0; i++) {
		BaseConfigItem *itemP = value[i]->get(name);
		if (itemP != 0) {
			return itemP;
		}
	}

	return 0;
}

unsigned int CompositeConfigItem::getChecksum(int index) {
	unsigned int checksum = BaseConfigItem::getChecksum(index);

	for (int i=0; value[i] != 0; i++) {
		checksum += value[i]->getChecksum(index + i) ^ index;
	}

	return checksum;
}

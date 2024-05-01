/*
 * ConfigItem.h
 *
 *  Created on: Dec 6, 2017
 *      Author: Paul
 */

#ifndef LIBRARIES_CONFIGS_CONFIGITEM_H_
#define LIBRARIES_CONFIGS_CONFIGITEM_H_
#include "Arduino.h"
#include <EEPROM.h>

#ifndef DEBUG
#define DEBUG(...) {}
#endif

struct BaseConfigItem {
	BaseConfigItem(const char *name, int maxSize) :
		name(name),
		maxSize(maxSize),
		start(-1)
	{}
	virtual ~BaseConfigItem() {}

	virtual int init(int start) { this->start = start; return start + maxSize; }
	virtual unsigned int getChecksum(int index) {
		unsigned int checksum = 0;
		for (int i=0; name[i] != 0; i++) {
			checksum += name[i] ^ index;
		}
		return checksum;
	}
	virtual BaseConfigItem* get(const char *name) {
		if (strcmp(name, this->name) == 0) {
			return this;
		}

		return 0;
	}

	virtual void fromString(const String &s) = 0;
	virtual void put() const = 0;
	virtual BaseConfigItem& get() = 0;
	virtual String toJSON(bool bare = false, const char **excludes = 0) const = 0;
	virtual String toString(const char **excludes = 0) const = 0;
	virtual void notify() = 0;
  virtual void forEach(std::function<void(BaseConfigItem&)>) = 0;
	virtual void debug(Print *debugPrint) const = 0;
	const char *name;
	byte maxSize;
	int start;
};

template<typename T>
struct ConfigItem : public BaseConfigItem {
	ConfigItem(const char *name, const byte maxSize, const T value)
	: BaseConfigItem(name, maxSize), value(value)
	{}

	T value;

	virtual void put() const { EEPROM.put(start, value); }
	virtual BaseConfigItem& get() { EEPROM.get(start, value); return *this; }
	virtual void debug(Print *debugPrint) const;
	operator T () const { return value; }
	virtual void notify() { if (cb) cb(*this); }
  virtual void forEach(std::function<void(BaseConfigItem&)> pFunc) { pFunc(*this); };
	void setCallback(void (*cb)(ConfigItem<T>&)) { this->cb = cb; }

	void (*cb)(ConfigItem<T>&) = 0;
};

struct BooleanConfigItem : public ConfigItem<bool> {
	BooleanConfigItem(const char *name, const bool value)
	: ConfigItem(name, sizeof(bool), value)
	{}

	virtual void fromString(const String &s) { value = s.equalsIgnoreCase("true") ? 1 : 0; }
	virtual String toJSON(bool bare = false, const char **excludes = 0) const { return value ? "true" : "false"; }
	virtual String toString(const char **excludes = 0) const { return value ? "true" : "false"; }
	BooleanConfigItem& operator=(const bool val) { value = val; return *this; }
};

struct ByteConfigItem : public ConfigItem<byte> {
	ByteConfigItem(const char *name, const byte value)
	: ConfigItem(name, sizeof(byte), value)
	{}

	virtual void fromString(const String &s) { value = s.toInt(); }
	virtual String toJSON(bool bare = false, const char **excludes = 0) const { return String(value); }
	virtual String toString(const char **excludes = 0) const { return String(value); }
	ByteConfigItem& operator=(const byte val) { value = val; return *this; }
};

struct IntConfigItem : public ConfigItem<int> {
	IntConfigItem(const char *name, const int value)
	: ConfigItem(name, sizeof(int), value)
	{}

	virtual void fromString(const String &s) { value = s.toInt(); }
	virtual String toJSON(bool bare = false, const char **excludes = 0) const { return String(value); }
	virtual String toString(const char **excludes = 0) const { return String(value); }
	IntConfigItem& operator=(const int val) { value = val; return *this; }
};

struct FloatConfigItem : public ConfigItem<float> {
	FloatConfigItem(const char *name, const float value)
	: ConfigItem(name, sizeof(float), value)
	{}

	virtual void fromString(const String &s) { value = s.toFloat(); }
	virtual String toJSON(bool bare = false, const char **excludes = 0) const { return String(value); }
	virtual String toString(const char **excludes = 0) const { return String(value); }
	FloatConfigItem& operator=(const float val) { value = val; return *this; }
};

struct StringConfigItem : public ConfigItem<String> {
	StringConfigItem(const char *name, const byte maxSize, const String &value)
	: ConfigItem(name, maxSize, value)
	{}

	virtual void fromString(const String &s) { value = s.substring(0, maxSize); }
	virtual String toJSON(bool bare = false, const char **excludes = 0) const;
	virtual String toString(const char **excludes = 0) const { return value; };
	virtual void put() const;
	virtual BaseConfigItem& get();
	StringConfigItem& operator=(const String &val) { value = val.substring(0, maxSize); return *this; }
};

/**
 * TODO: parse JSON to populate all items!
 */
struct CompositeConfigItem : public BaseConfigItem {
	CompositeConfigItem(const char *name, const byte maxSize, BaseConfigItem** value)
	: BaseConfigItem(name, maxSize), value(value)
	{}

	BaseConfigItem** value;	// an array of pointers to BaseConfigItems

	virtual int init(int start);
	virtual BaseConfigItem* get(const char *name);
	virtual unsigned int getChecksum(int index);

	virtual void fromString(const String &s) {  }
	virtual String toJSON(bool bare = false, const char **excludes = 0) const;
	virtual String toString(const char **excludes = 0) const;
	virtual void put() const;
	virtual BaseConfigItem& get();
  virtual void forEach(std::function<void(BaseConfigItem&)>);
	CompositeConfigItem& operator=(BaseConfigItem** val) { value = val; return *this; }
	virtual void debug(Print *debugPrint) const;

	virtual void notify() { if (cb) cb(*this); }
	void setCallback(void (*cb)(CompositeConfigItem&)) { this->cb = cb; }

	void (*cb)(CompositeConfigItem&) = 0;
};

#endif /* LIBRARIES_CONFIGS_CONFIGITEM_H_ */

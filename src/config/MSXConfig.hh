// $Id$

#ifndef __MSXCONFIG_HH__
#define __MSXCONFIG_HH__

#include <string>
#include <list>
#include "ConfigException.hh"
#include "libxmlx/xmlx.hh"
#include "FileContext.hh"
#include "File.hh"

using std::string;
using std::list;

namespace openmsx {

class Config;
class Device;

class MSXConfig
{
public:
	static MSXConfig& instance();

	/**
	 * load a config file's content, and add it to
	 *  the config data [can be called multiple times]
	 */
	void loadHardware(FileContext& context, const string &filename)
		throw(FileException, ConfigException);
	void loadSetting(FileContext& context, const string &filename)
		throw(FileException, ConfigException);
	void loadStream(FileContext& context, const ostringstream &stream)
		throw(ConfigException);

	/**
	 * get a config or device or customconfig by id
	 */
	Config* getConfigById(const string& id) throw(ConfigException);
	Config* findConfigById(const string& id);
	bool hasConfigWithId(const string& id);
	Device* getDeviceById(const string& id) throw(ConfigException);

	/**
	 * get a device
	 */
	void initDeviceIterator();
	Device* getNextDevice();

private:
	MSXConfig();
	~MSXConfig();

	void handleDoc(XML::Document* doc, FileContext& context)
		throw(ConfigException);

	list<XML::Document*> docs;
	list<Config*> configs;
	list<Device*> devices;

	list<Device*>::const_iterator device_iterator;

	// Let GCC-3.2.3 be quiet...
	friend class dontGenerateWarningOnOlderCompilers;
};

} // namespace openmsx

#endif

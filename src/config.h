#ifndef HVH_WC_CONFIG_H
#define HVH_WC_CONFIG_H

#include <rapidjson/document.h>

namespace wc {
namespace config {

	static constexpr const char* CONFIG_FILENAME = "config.json";

	// Loads the config file from disc into memory.
	// userpath_utf8: the UTF8-encoded directory where the config file is saved.
	// Returns false on failure.
	bool Init(const char* userpath_utf8);

	// Saves the config file to disc, if neccesary.
	void Shutdown();

	// Notifies that the config has been modified and needs to be saves.
	void setModified();

	// Gets the part of the config pertaining to a particular subsystem.
	rapidjson::GenericObject* getSubconfig(const char* name);

}} // namespace wc::config

#endif // HVH_WC_CONFIG_H
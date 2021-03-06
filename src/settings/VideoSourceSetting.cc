#include "VideoSourceSetting.hh"
#include "CommandException.hh"
#include "Completer.hh"
#include "StringOp.hh"

namespace openmsx {

VideoSourceSetting::VideoSourceSetting(CommandController& commandController)
	: Setting(commandController, "videosource",
	          "selects the video source to display on the screen",
	          "none", DONT_SAVE)
{
	sources = { { "none", 0 } };

	setChecker([this](TclObject& newValue) {
		checkSetValue(newValue.getString()); // may throw
	});
	init();
}

void VideoSourceSetting::checkSetValue(string_ref value) const
{
	// Special case: in case there are no videosources registered (yet),
	// the only allowed value is "none". In case there is at least one
	// registered source, this special value "none" should be hidden.
	if (((value == "none") && (sources.size() >  1)) ||
	    ((value != "none") && !has(value))) {
		throw CommandException("video source not available");
	}
}

int VideoSourceSetting::getSource()
{
	// Always try to find a better value than "none".
	string_ref str = getValue().getString();
	if (str != "none") {
		// If current value is allowed, then keep it.
		if (int id = has(str)) {
			return id;
		}
	}
	// Search the best value from the current set of allowed values.
	int id = 0;
	if (!id) { id = has("Video9000"); } // in
	if (!id) { id = has("MSX");       } //  order
	if (!id) { id = has("GFX9000");   } //   of
	if (!id) { id = has("Laserdisc"); } //    preference
	if (!id) {
		// This handles the "none" case, but also stuff like
		// multiple V99x8/V9990 chips. Prefer the source with
		// highest id (=newest).
		for (auto& p : sources) {
			id = std::max(id, p.second);
		}
	}
	setSource(id); // store new value
	return id;
}

void VideoSourceSetting::setSource(int id)
{
	auto it = find_if(sources.begin(), sources.end(),
		[&](const Sources::value_type& p) { return p.second == id; });
	assert(it != sources.end());
	setString(it->first);
}

string_ref VideoSourceSetting::getTypeString() const
{
	return "enumeration";
}

std::vector<string_ref> VideoSourceSetting::getPossibleValues() const
{
	std::vector<string_ref> result;
	if (sources.size() == 1) {
		assert(sources.front().first == "none");
		result.push_back("none");
	} else {
		for (auto& p : sources) {
			if (p.second != 0) {
				result.push_back(p.first);
			}
		}
	}
	return result;
}

void VideoSourceSetting::additionalInfo(TclObject& result) const
{
	TclObject valueList(result.getInterpreter());
	valueList.addListElements(getPossibleValues());
	result.addListElement(valueList);
}

void VideoSourceSetting::tabCompletion(std::vector<std::string>& tokens) const
{
	Completer::completeString(tokens, getPossibleValues(),
	                          false); // case insensitive
}

int VideoSourceSetting::registerVideoSource(const std::string& source)
{
	static int counter = 0; // id's are globally unique

	assert(!has(source));
	sources.emplace_back(source, ++counter);

	// First announce extended set of allowed values before announcing a
	// (possibly) different value.
	notifyPropertyChange();
	setSource(getSource()); // via source to (possibly) adjust value

	return counter;
}

void VideoSourceSetting::unregisterVideoSource(int source)
{
	auto it = find_if(sources.begin(), sources.end(),
		[&](Sources::value_type& p) { return p.second == source; });
	assert(it != sources.end());
	sources.erase(it);

	// First notify the (possibly) changed value before announcing the
	// shrinked set of values.
	setSource(getSource()); // via source to (possibly) adjust value
	notifyPropertyChange();
}

bool VideoSourceSetting::has(int value) const
{
	for (auto& p : sources) {
		if (p.second == value) return true;
	}
	return false;
}

int VideoSourceSetting::has(string_ref value) const
{
	auto it = find_if(sources.begin(), sources.end(),
		[&](const Sources::value_type& p) {
			StringOp::casecmp cmp;
			return cmp(p.first, value); });
	return (it != sources.end()) ? it->second : 0;
}


VideoSourceActivator::VideoSourceActivator(
	VideoSourceSetting& setting_, const std::string& name)
	: setting(setting_)
{
	id = setting.registerVideoSource(name);
}

VideoSourceActivator::~VideoSourceActivator()
{
	setting.unregisterVideoSource(id);
}

} // namespace openmsx

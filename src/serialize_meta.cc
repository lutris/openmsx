#include "serialize_meta.hh"
#include "serialize.hh"
#include "MSXException.hh"
#include "StringOp.hh"
#include "stl.hh"
#include <algorithm>
#include <cassert>
#include <iostream>

namespace openmsx {

template<typename Archive>
PolymorphicSaverRegistry<Archive>::PolymorphicSaverRegistry()
	: initialized(false)
{
}

template<typename Archive>
PolymorphicSaverRegistry<Archive>::~PolymorphicSaverRegistry()
{
}

template<typename Archive>
PolymorphicSaverRegistry<Archive>& PolymorphicSaverRegistry<Archive>::instance()
{
	static PolymorphicSaverRegistry oneInstance;
	return oneInstance;
}

template<typename Archive>
void PolymorphicSaverRegistry<Archive>::registerHelper(
	const std::type_info& type,
	std::unique_ptr<PolymorphicSaverBase<Archive>> saver)
{
	assert(!initialized);
	assert(find_if(saverMap.begin(), saverMap.end(), EqualTupleValue<0>(type))
	       == saverMap.end());
	saverMap.emplace_back(type, std::move(saver));
}

template<typename Archive>
void PolymorphicSaverRegistry<Archive>::save(
	Archive& ar, const void* t, const std::type_info& typeInfo)
{
	auto& reg = PolymorphicSaverRegistry<Archive>::instance();
	if (unlikely(!reg.initialized)) {
		reg.initialized = true;
		sort(reg.saverMap.begin(), reg.saverMap.end(),
		     LessTupleElement<0>());
	}
	auto it = lower_bound(reg.saverMap.begin(), reg.saverMap.end(), typeInfo,
		LessTupleElement<0>());
	if ((it == reg.saverMap.end()) || (it->first != typeInfo)) {
		std::cerr << "Trying to save an unregistered polymorphic type: "
			  << typeInfo.name() << std::endl;
		assert(false); return;
	}
	it->second->save(ar, t);
}
template<typename Archive>
void PolymorphicSaverRegistry<Archive>::save(
	const char* tag, Archive& ar, const void* t, const std::type_info& typeInfo)
{
	ar.beginTag(tag);
	save(ar, t, typeInfo);
	ar.endTag(tag);
}

template class PolymorphicSaverRegistry<MemOutputArchive>;
template class PolymorphicSaverRegistry<XmlOutputArchive>;

////

template<typename Archive>
PolymorphicLoaderRegistry<Archive>::PolymorphicLoaderRegistry()
{
}

template<typename Archive>
PolymorphicLoaderRegistry<Archive>::~PolymorphicLoaderRegistry()
{
}

template<typename Archive>
PolymorphicLoaderRegistry<Archive>& PolymorphicLoaderRegistry<Archive>::instance()
{
	static PolymorphicLoaderRegistry oneInstance;
	return oneInstance;
}

template<typename Archive>
void PolymorphicLoaderRegistry<Archive>::registerHelper(
	const char* name,
	std::unique_ptr<PolymorphicLoaderBase<Archive>> loader)
{
	assert(loaderMap.find(name) == loaderMap.end());
	loaderMap[name] = std::move(loader);
}

template<typename Archive>
void* PolymorphicLoaderRegistry<Archive>::load(
	Archive& ar, unsigned id, const void* args)
{
	std::string type;
	ar.attribute("type", type);
	auto& reg = PolymorphicLoaderRegistry<Archive>::instance();
	auto it = reg.loaderMap.find(type);
	assert(it != reg.loaderMap.end());
	return it->second->load(ar, id, args);
}

template class PolymorphicLoaderRegistry<MemInputArchive>;
template class PolymorphicLoaderRegistry<XmlInputArchive>;

////

void polyInitError(const char* expected, const char* actual)
{
	throw MSXException(StringOp::Builder() <<
		"Expected type: " << expected << " but got: " << actual << '.');
}

template<typename Archive>
PolymorphicInitializerRegistry<Archive>::PolymorphicInitializerRegistry()
{
}

template<typename Archive>
PolymorphicInitializerRegistry<Archive>::~PolymorphicInitializerRegistry()
{
}

template<typename Archive>
PolymorphicInitializerRegistry<Archive>& PolymorphicInitializerRegistry<Archive>::instance()
{
	static PolymorphicInitializerRegistry oneInstance;
	return oneInstance;
}

template<typename Archive>
void PolymorphicInitializerRegistry<Archive>::registerHelper(
	const char* name,
	std::unique_ptr<PolymorphicInitializerBase<Archive>> initializer)
{
	assert(initializerMap.find(name) == initializerMap.end());
	initializerMap[name] = std::move(initializer);
}

template<typename Archive>
void PolymorphicInitializerRegistry<Archive>::init(
	const char* tag, Archive& ar, void* t)
{
	ar.beginTag(tag);
	unsigned id;
	ar.attribute("id", id);
	assert(id);
	std::string type;
	ar.attribute("type", type);

	auto& reg = PolymorphicInitializerRegistry<Archive>::instance();
	auto it = reg.initializerMap.find(type);
	assert(it != reg.initializerMap.end());
	it->second->init(ar, t, id);

	ar.endTag(tag);
}

template class PolymorphicInitializerRegistry<MemInputArchive>;
template class PolymorphicInitializerRegistry<XmlInputArchive>;

} // namespace openmsx

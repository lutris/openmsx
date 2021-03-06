#include "Filename.hh"
#include "FileContext.hh"
#include "FileOperations.hh"
#include "MSXException.hh"
#include "serialize.hh"
#include <cassert>

using std::string;

namespace openmsx {

// dummy constructor, to be able to serialize vector<Filename>
Filename::Filename()
{
}

Filename::Filename(const string& filename)
	: originalFilename(filename)
	, resolvedFilename(filename)
{
}

Filename::Filename(const string& filename, const FileContext& context)
	: originalFilename(filename)
	, resolvedFilename(FileOperations::getAbsolutePath(
		context.resolve(originalFilename)))
{
}

void Filename::updateAfterLoadState()
{
	if (empty()) return;
	if (FileOperations::exists(resolvedFilename)) return;

	try {
		resolvedFilename = FileOperations::getAbsolutePath(
			UserFileContext().resolve(originalFilename));
	} catch (MSXException&) {
		// nothing
	}
}

bool Filename::empty() const
{
	assert(getOriginal().empty() == getResolved().empty());
	return getOriginal().empty();
}

void Filename::setResolved(const std::string& resolved)
{
	resolvedFilename = resolved;
}

template<typename Archive>
void Filename::serialize(Archive& ar, unsigned /*version*/)
{
	ar.serialize("original", originalFilename);
	ar.serialize("resolved", resolvedFilename);
}
INSTANTIATE_SERIALIZE_METHODS(Filename);

} // namespace openmsx

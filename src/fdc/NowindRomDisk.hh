#ifndef NOWINDROMDISK_HH
#define NOWINDROMDISK_HH

#include "DiskContainer.hh"

namespace openmsx {

class NowindRomDisk : public DiskContainer
{
public:
	virtual SectorAccessibleDisk* getSectorAccessibleDisk();
	virtual const std::string& getContainerName() const;
	virtual bool diskChanged();
	virtual int insertDisk(const std::string& filename);

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);
};

} // namespace openmsx

#endif

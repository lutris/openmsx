#ifndef SECTORACCESSIBLEDISK_HH
#define SECTORACCESSIBLEDISK_HH

#include "DiskImageUtils.hh"
#include "Filename.hh"
#include "sha1.hh"
#include <vector>
#include <memory>

namespace openmsx {

class PatchInterface;

class SectorAccessibleDisk
{
public:
	static const size_t SECTOR_SIZE = sizeof(SectorBuffer);

	virtual ~SectorAccessibleDisk();

	// sector stuff
	void readSector (size_t sector,       SectorBuffer& buf);
	void writeSector(size_t sector, const SectorBuffer& buf);
	size_t getNbSectors() const;

	// write protected stuff
	bool isWriteProtected() const;
	void forceWriteProtect();

	virtual bool isDummyDisk() const;

	// patch stuff
	void applyPatch(const Filename& patchFile);
	std::vector<Filename> getPatches() const;
	bool hasPatches() const;

	/** Calculate SHA1 of the content of this disk.
	 * This value is cached (and flushed on writes).
	 */
	virtual Sha1Sum getSha1Sum();

	// For compatibility with nowind
	//  - read/write multiple sectors instead of one-per-one
	//  - use error codes instead of exceptions
	//  - different order of parameters
	int readSectors (      SectorBuffer* buffers, size_t startSector,
	                 size_t nbSectors);
	int writeSectors(const SectorBuffer* buffers, size_t startSector,
	                 size_t nbSectors);

protected:
	SectorAccessibleDisk();

	// Peek-mode changes the behaviour of readSector(). ATM it only has
	// an effect on DirAsDSK. See comment in DirAsDSK::readSectorImpl()
	// for more details.
	void setPeekMode(bool peek) { peekMode = peek; }
	bool isPeekMode() const { return peekMode; }

	virtual void checkCaches();
	virtual void flushCaches();

private:
	virtual void readSectorImpl (size_t sector,       SectorBuffer& buf) = 0;
	virtual void writeSectorImpl(size_t sector, const SectorBuffer& buf) = 0;
	virtual size_t getNbSectorsImpl() const = 0;
	virtual bool isWriteProtectedImpl() const = 0;

	std::unique_ptr<const PatchInterface> patch;
	Sha1Sum sha1cache;
	bool forcedWriteProtect;
	bool peekMode;

	friend class EmptyDiskPatch;
};

} // namespace openmsx

#endif

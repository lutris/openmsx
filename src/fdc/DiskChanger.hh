#ifndef DISKCHANGER_HH
#define DISKCHANGER_HH

#include "DiskContainer.hh"
#include "StateChangeListener.hh"
#include "serialize_meta.hh"
#include "noncopyable.hh"
#include <vector>
#include <string>
#include <memory>

namespace openmsx {

class CommandController;
class StateChangeDistributor;
class Scheduler;
class FilePool;
class MSXMotherBoard;
class DiskFactory;
class DiskManipulator;
class Disk;
class DiskCommand;
class TclObject;
class DiskName;

class DiskChanger : public DiskContainer, private StateChangeListener,
                    private noncopyable
{
public:
	DiskChanger(MSXMotherBoard& board,
	            const std::string& driveName,
	            bool createCommand = true,
	            bool isDoubleSidedDrive = true);
	DiskChanger(const std::string& driveName,
	            CommandController& commandController,
	            DiskFactory& diskFactory,
	            DiskManipulator& manipulator,
	            bool createCommand);
	~DiskChanger();

	void createCommand();

	const std::string& getDriveName() const;
	const DiskName& getDiskName() const;
	bool peekDiskChanged() const;
	void forceDiskChange();
	Disk& getDisk();

	// DiskContainer
	virtual SectorAccessibleDisk* getSectorAccessibleDisk();
	virtual const std::string& getContainerName() const;
	virtual bool diskChanged();
	virtual int insertDisk(const std::string& filename);

	// for NowindCommand
	void changeDisk(std::unique_ptr<Disk> newDisk);

	// for DirAsDSK
	Scheduler* getScheduler() const { return scheduler; }
	bool isDoubleSidedDrive() const { return doubleSidedDrive; }

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	void init(const std::string& prefix, bool createCmd);
	void insertDisk(const std::vector<TclObject>& args);
	void ejectDisk();
	void sendChangeDiskEvent(const std::vector<std::string>& args);

	// StateChangeListener
	virtual void signalStateChange(const std::shared_ptr<StateChange>& event);
	virtual void stopReplay(EmuTime::param time);

	CommandController& controller;
	StateChangeDistributor* stateChangeDistributor;
	Scheduler* scheduler;
	FilePool* filePool;
	DiskFactory& diskFactory;
	DiskManipulator& manipulator;

	const std::string driveName;
	std::unique_ptr<Disk> disk;

	friend class DiskCommand;
	std::unique_ptr<DiskCommand> diskCommand; // must come after driveName
	const bool doubleSidedDrive; // for DirAsDSK

	bool diskChangedFlag;
};
SERIALIZE_CLASS_VERSION(DiskChanger, 2);

} // namespace openmsx

#endif

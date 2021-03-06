#include "DiskChanger.hh"
#include "DiskFactory.hh"
#include "DummyDisk.hh"
#include "RamDSKDiskImage.hh"
#include "DirAsDSK.hh"
#include "CommandController.hh"
#include "RecordedCommand.hh"
#include "StateChangeDistributor.hh"
#include "Scheduler.hh"
#include "FilePool.hh"
#include "File.hh"
#include "MSXMotherBoard.hh"
#include "Reactor.hh"
#include "DiskManipulator.hh"
#include "FileContext.hh"
#include "FileOperations.hh"
#include "FileException.hh"
#include "CommandException.hh"
#include "CliComm.hh"
#include "TclObject.hh"
#include "EmuTime.hh"
#include "serialize.hh"
#include "serialize_stl.hh"
#include "serialize_constr.hh"
#include "memory.hh"
#include <functional>

using std::string;
using std::vector;

namespace openmsx {

class DiskCommand : public Command
{
public:
	DiskCommand(CommandController& commandController,
	            DiskChanger& diskChanger);
	virtual void execute(const vector<TclObject>& tokens,
	                     TclObject& result);
	virtual string help(const vector<string>& tokens) const;
	virtual void tabCompletion(vector<string>& tokens) const;
	virtual bool needRecord(const vector<string>& tokens) const;
private:
	DiskChanger& diskChanger;
};

DiskChanger::DiskChanger(MSXMotherBoard& board,
                         const string& driveName_,
                         bool createCmd,
                         bool isDoubleSidedDrive)
	: controller(board.getCommandController())
	, stateChangeDistributor(&board.getStateChangeDistributor())
	, scheduler(&board.getScheduler())
	, filePool(&board.getReactor().getFilePool())
	, diskFactory(board.getReactor().getDiskFactory())
	, manipulator(board.getReactor().getDiskManipulator())
	, driveName(driveName_)
	, doubleSidedDrive(isDoubleSidedDrive)
{
	init(board.getMachineID() + "::", createCmd);
}

DiskChanger::DiskChanger(const string& driveName_,
                         CommandController& controller_,
                         DiskFactory& diskFactory_,
                         DiskManipulator& manipulator_,
                         bool createCmd)
	: controller(controller_)
	, stateChangeDistributor(nullptr)
	, scheduler(nullptr)
	, filePool(nullptr)
	, diskFactory(diskFactory_)
	, manipulator(manipulator_)
	, driveName(driveName_)
	, doubleSidedDrive(true) // irrelevant, but needs a value
{
	init("", createCmd);
}

void DiskChanger::init(const string& prefix, bool createCmd)
{
	if (createCmd) createCommand();
	ejectDisk();
	manipulator.registerDrive(*this, prefix);
	if (stateChangeDistributor) {
		stateChangeDistributor->registerListener(*this);
	}
}

void DiskChanger::createCommand()
{
	if (diskCommand) return;
	diskCommand = make_unique<DiskCommand>(controller, *this);
}

DiskChanger::~DiskChanger()
{
	if (stateChangeDistributor) {
		stateChangeDistributor->unregisterListener(*this);
	}
	manipulator.unregisterDrive(*this);
}

const string& DiskChanger::getDriveName() const
{
	return driveName;
}

const DiskName& DiskChanger::getDiskName() const
{
	return disk->getName();
}

bool DiskChanger::diskChanged()
{
	bool ret = diskChangedFlag;
	diskChangedFlag = false;
	return ret;
}

bool DiskChanger::peekDiskChanged() const
{
	return diskChangedFlag;
}

void DiskChanger::forceDiskChange()
{
	diskChangedFlag = true;
}

Disk& DiskChanger::getDisk()
{
	return *disk;
}

SectorAccessibleDisk* DiskChanger::getSectorAccessibleDisk()
{
	if (dynamic_cast<DummyDisk*>(disk.get())) {
		return nullptr;
	}
	return dynamic_cast<SectorAccessibleDisk*>(disk.get());
}

const std::string& DiskChanger::getContainerName() const
{
	return getDriveName();
}

void DiskChanger::sendChangeDiskEvent(const vector<string>& args)
{
	// note: might throw MSXException
	if (stateChangeDistributor) {
		stateChangeDistributor->distributeNew(
			std::make_shared<MSXCommandEvent>(
				args, scheduler->getCurrentTime()));
	} else {
		signalStateChange(std::make_shared<MSXCommandEvent>(
			args, EmuTime::zero));
	}
}

void DiskChanger::signalStateChange(const std::shared_ptr<StateChange>& event)
{
	auto commandEvent = dynamic_cast<MSXCommandEvent*>(event.get());
	if (!commandEvent) return;

	const vector<TclObject>& tokens = commandEvent->getTokens();
	if (tokens[0].getString() == getDriveName()) {
		if (tokens[1].getString() == "eject") {
			ejectDisk();
		} else {
			insertDisk(tokens);
		}
	}
}

void DiskChanger::stopReplay(EmuTime::param /*time*/)
{
	// nothing
}

int DiskChanger::insertDisk(const string& filename)
{
	vector<TclObject> args = { TclObject("dummy"), TclObject(filename) };
	try {
		insertDisk(args);
		return 0;
	} catch (MSXException&) {
		return -1;
	}
}

void DiskChanger::insertDisk(const vector<TclObject>& args)
{
	UserFileContext context;
	const string& diskImage = FileOperations::getConventionalPath(args[1].getString());
	std::unique_ptr<Disk> newDisk(diskFactory.createDisk(diskImage, *this));
	for (unsigned i = 2; i < args.size(); ++i) {
		Filename filename(args[i].getString().str(), context);
		newDisk->applyPatch(filename);
	}

	// no errors, only now replace original disk
	changeDisk(std::move(newDisk));
}

void DiskChanger::ejectDisk()
{
	changeDisk(make_unique<DummyDisk>());
}

void DiskChanger::changeDisk(std::unique_ptr<Disk> newDisk)
{
	disk = std::move(newDisk);
	diskChangedFlag = true;
	controller.getCliComm().update(CliComm::MEDIA, getDriveName(),
	                               getDiskName().getResolved());
}


// class DiskCommand

DiskCommand::DiskCommand(CommandController& commandController,
                         DiskChanger& diskChanger_)
	: Command(commandController, diskChanger_.driveName)
	, diskChanger(diskChanger_)
{
}

void DiskCommand::execute(const vector<TclObject>& tokens, TclObject& result)
{
	if (tokens.size() == 1) {
		result.addListElement(diskChanger.getDriveName() + ':');
		result.addListElement(diskChanger.getDiskName().getResolved());

		TclObject options(result.getInterpreter());
		if (dynamic_cast<DummyDisk*>(diskChanger.disk.get())) {
			options.addListElement("empty");
		} else if (dynamic_cast<DirAsDSK*>(diskChanger.disk.get())) {
			options.addListElement("dirasdisk");
		} else if (dynamic_cast<RamDSKDiskImage*>(diskChanger.disk.get())) {
			options.addListElement("ramdsk");
		}
		if (diskChanger.disk->isWriteProtected()) {
			options.addListElement("readonly");
		}
		if (options.getListLength() != 0) {
			result.addListElement(options);
		}

	} else if (tokens[1].getString() == "ramdsk") {
		diskChanger.sendChangeDiskEvent({
			diskChanger.getDriveName(), tokens[1].getString().str()});
	} else if (tokens[1].getString() == "-ramdsk") {
		diskChanger.sendChangeDiskEvent({
			diskChanger.getDriveName(), "ramdsk"});
		result.setString(
			"Warning: use of '-ramdsk' is deprecated, instead use the 'ramdsk' subcommand");
	} else if (tokens[1].getString() == "-eject") {
		diskChanger.sendChangeDiskEvent({
			diskChanger.getDriveName(), "eject"});
		result.setString(
			"Warning: use of '-eject' is deprecated, instead use the 'eject' subcommand");
	} else if (tokens[1].getString() == "eject") {
		diskChanger.sendChangeDiskEvent({
			diskChanger.getDriveName(), "eject"});
	} else {
		int firstFileToken = 1;
		if (tokens[1].getString() == "insert") {
			if (tokens.size() > 2) {
				firstFileToken = 2; // skip this subcommand as filearg
			} else {
				throw CommandException("Missing argument to insert subcommand");
			}
		}
		try {
			vector<string> args = { diskChanger.getDriveName() };
			for (unsigned i = firstFileToken; i < tokens.size(); ++i) {
				string_ref option = tokens[i].getString();
				if (option == "-ips") {
					if (++i == tokens.size()) {
						throw MSXException(
							"Missing argument for option \"" + option + '\"');
					}
					args.push_back(tokens[i].getString().str());
				} else {
					// backwards compatibility
					args.push_back(option.str());
				}
			}
			diskChanger.sendChangeDiskEvent(args);
		} catch (FileException& e) {
			throw CommandException(e.getMessage());
		}
	}
}

string DiskCommand::help(const vector<string>& /*tokens*/) const
{
	const string& name = diskChanger.getDriveName();
	return name + " eject             : remove disk from virtual drive\n" +
	       name + " ramdsk            : create a virtual disk in RAM\n" +
	       name + " insert <filename> : change the disk file\n" +
	       name + " <filename>        : change the disk file\n" +
	       name + "                   : show which disk image is in drive";
}

void DiskCommand::tabCompletion(vector<string>& tokens) const
{
	if (tokens.size() >= 2) {
		static const char* const extra[] = {
			"eject", "ramdsk", "insert",
		};
		completeFileName(tokens, UserFileContext(), extra);
	}
}

bool DiskCommand::needRecord(const vector<string>& tokens) const
{
	return tokens.size() > 1;
}

static string calcSha1(SectorAccessibleDisk* disk)
{
	return disk ? disk->getSha1Sum().toString() : "";
}

// version 1:  initial version
// version 2:  replaced Filename with DiskName
template<typename Archive>
void DiskChanger::serialize(Archive& ar, unsigned version)
{
	DiskName diskname = disk->getName();
	if (ar.versionBelow(version, 2)) {
		// there was no DiskName yet, just a plain Filename
		Filename filename;
		ar.serialize("disk", filename);
		if (filename.getOriginal() == "ramdisk") {
			diskname = DiskName(Filename(), "ramdisk");
		} else {
			diskname = DiskName(filename, "");
		}
	} else {
		ar.serialize("disk", diskname);
	}

	vector<Filename> patches;
	if (!ar.isLoader()) {
		patches = disk->getPatches();
	}
	ar.serialize("patches", patches);

	string oldChecksum;
	if (!ar.isLoader()) {
		oldChecksum = calcSha1(getSectorAccessibleDisk());
	}
	ar.serialize("checksum", oldChecksum);

	if (ar.isLoader()) {
		diskname.updateAfterLoadState();
		string name = diskname.getResolved(); // TODO use Filename
		if (!name.empty()) {
			// Only when the original file doesn't exist on this
			// system, try to search by sha1sum. This means we
			// prefer the original file over a file with a matching
			// sha1sum (the original file may have changed). An
			// alternative is to prefer the exact sha1sum match.
			// I'm not sure which alternative is better.
			if (!FileOperations::exists(name)) {
				assert(filePool);
				assert(!oldChecksum.empty());
				if (auto file = filePool->getFile(
				          FilePool::DISK, Sha1Sum(oldChecksum))) {
					name = file->getURL();
				}
			}
			vector<TclObject> args =
				{ TclObject("dummy"), TclObject(name) };
			for (auto& p : patches) {
				p.updateAfterLoadState();
				args.emplace_back(p.getResolved()); // TODO
			}

			try {
				insertDisk(args);
			} catch (MSXException& e) {
				throw MSXException(
					"Couldn't reinsert disk in drive " +
					getDriveName() + ": " + e.getMessage());
				// Alternative: Print warning and continue
				//   without diskimage. Is this better?
			}
		}

		string newChecksum = calcSha1(getSectorAccessibleDisk());
		if (oldChecksum != newChecksum) {
			controller.getCliComm().printWarning(
				"The content of the diskimage " +
				diskname.getResolved() +
				" has changed since the time this savestate was "
				"created. This might result in emulation problems "
				"or even diskcorruption. To prevent the latter, "
				"the disk is now write-protected (eject and "
				"reinsert the disk if you want to override this).");
			disk->forceWriteProtect();
		}
	}

	// This should only be restored after disk is inserted
	ar.serialize("diskChanged", diskChangedFlag);
}

// extra (local) constructor arguments for polymorphic de-serialization
template<> struct SerializeConstructorArgs<DiskChanger>
{
	typedef std::tuple<std::string> type;

	template<typename Archive>
	void save(Archive& ar, const DiskChanger& changer)
	{
		ar.serialize("driveName", changer.getDriveName());
	}

	template<typename Archive> type load(Archive& ar, unsigned /*version*/)
	{
		string driveName;
		ar.serialize("driveName", driveName);
		return make_tuple(driveName);
	}
};

INSTANTIATE_SERIALIZE_METHODS(DiskChanger);
REGISTER_POLYMORPHIC_CLASS_1(DiskContainer, DiskChanger, "DiskChanger",
                             std::reference_wrapper<MSXMotherBoard>);

} // namespace openmsx

// $Id$

#include "openmsx.hh"
#include "TCLInterp.hh"
#include "CommandConsole.hh"
#include "TCLCommandResult.hh"
#include "MSXException.hh"

namespace openmsx {
	
int dummyClose(ClientData instanceData, Tcl_Interp *interp)
{
	return 0;
}
int dummyInput(ClientData instanceData, char *buf, int bufSize, int *errorCodePtr)
{
	return 0;
}
void dummyWatch(ClientData instanceData, int mask)
{
}
int dummyGetHandle(ClientData instanceData, int direction, ClientData *handlePtr)
{
	return TCL_ERROR;
}
Tcl_ChannelType TCLInterp::channelType = {
	"openMSX console",	// Type name
	NULL,			// Always non-blocking
	dummyClose,		// Close proc
	dummyInput,		// Input proc
	TCLInterp::outputProc,	// Output proc
	NULL,			// Seek proc
	NULL,			// Set option proc
	NULL,			// Get option proc
	dummyWatch,		// Watch for events on console
	dummyGetHandle,		// Get a handle from the device
};

void TCLInterp::init(const char* programName)
{
	Tcl_FindExecutable(programName);
}

TCLInterp::TCLInterp()
{
	interp = Tcl_CreateInterp();
	Tcl_Preserve(interp);
	
	// TODO need to investigate this: doesn't work on windows
	//if (Tcl_Init(interp) != TCL_OK) {
	//	cout << "Tcl_Init: " << interp->result << endl;
	//}

	Tcl_Channel channel = Tcl_CreateChannel(&channelType,
		"openMSX console", NULL, TCL_WRITABLE);
	if (channel != NULL) {
		Tcl_SetChannelOption(interp, channel, "-translation", "binary");
		Tcl_SetChannelOption(interp, channel, "-buffering", "line");
		Tcl_SetChannelOption(interp, channel, "-encoding", "binary");
	}
	Tcl_SetStdChannel(channel, TCL_STDOUT);
}

TCLInterp::~TCLInterp()
{
	cleanup();
}

void TCLInterp::cleanup()
{
	if (!Tcl_InterpDeleted(interp)) {
		Tcl_DeleteInterp(interp);
	}
	Tcl_Release(interp);
}

	
int TCLInterp::outputProc(ClientData clientData, const char* buf,
                 int toWrite, int* errorCodePtr)
{
	string output(buf, toWrite);
	if (!output.empty()) {
		CommandConsole::instance().print(output);
	}
	return toWrite;
}

void TCLInterp::registerCommand(const string& name, Command& command)
{
	Tcl_CreateObjCommand(interp, name.c_str(), commandProc,
	                     static_cast<ClientData>(&command), NULL);
}

void TCLInterp::unregisterCommand(const string& name, Command& command)
{
	Tcl_DeleteCommand(interp, name.c_str());
}

int TCLInterp::commandProc(ClientData clientData, Tcl_Interp* interp,
                           int objc, Tcl_Obj* const objv[])
{
	Command& command = *static_cast<Command*>(clientData);
	vector<string> tokens;
	tokens.reserve(objc);
	for (int i = 0; i < objc; ++i) {
		// TODO optimization: use TCL objects to avoid string conversion
		int length;
		char* data = (char*)Tcl_GetByteArrayFromObj(objv[i], &length);
		tokens.push_back(string(data, length));
	}
	TCLCommandResult result(interp);
	try {
		command.execute(tokens, result);
		return TCL_OK;
	} catch (CommandException& e) {
		result.setString(e.getMessage());
		return TCL_ERROR;
	}
}

// Returns
//   - build-in TCL commands
//   - openmsx commands
//   - user-defined procs
void TCLInterp::getCommandNames(set<string>& result)
{
	string list = execute("info commands");
	
	int argc;
	const char** argv;
	if (Tcl_SplitList(interp, list.c_str(), &argc, &argv) != TCL_OK) {
		return;
	}
	for (int i = 0; i < argc; ++i) {
		result.insert(argv[i]);
	}
	Tcl_Free((char*)argv);
}

bool TCLInterp::isComplete(const string& command) const
{
	return Tcl_CommandComplete(command.c_str());
}

string TCLInterp::execute(const string& command) throw(CommandException)
{
	int success = Tcl_Eval(interp, command.c_str());
	string result =  Tcl_GetStringResult(interp);
	if (success != TCL_OK) {
		throw CommandException(result);
	}
	return result;
}

string TCLInterp::executeFile(const string& filename) throw(CommandException)
{
	int success = Tcl_EvalFile(interp, filename.c_str());
	string result =  Tcl_GetStringResult(interp);
	if (success != TCL_OK) {
		throw CommandException(result);
	}
	return result;
}

void TCLInterp::setVariable(const string& name, const string& value)
{
	Tcl_SetVar(interp, name.c_str(), value.c_str(), 0);
}

void TCLInterp::unsetVariable(const string& name)
{
	Tcl_UnsetVar(interp, name.c_str(), 0);
}

string TCLInterp::getVariable(const string& name) const
{
	return Tcl_GetVar(interp, name.c_str(), 0);
}

void TCLInterp::registerSetting(SettingLeafNode& variable)
{
	const string& name = variable.getName();
	setVariable(name, variable.getValueString());
	Tcl_TraceVar(interp, name.c_str(),
	             TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	             traceProc, static_cast<ClientData>(&variable));
}

void TCLInterp::unregisterSetting(SettingLeafNode& variable)
{
	const string& name = variable.getName();
	Tcl_UntraceVar(interp, name.c_str(),
	               TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	               traceProc, static_cast<ClientData>(&variable));
	unsetVariable(name);
}

char* TCLInterp::traceProc(ClientData clientData, Tcl_Interp* interp,
                           const char* part1, const char* part2, int flags)
{
	static string static_string;
	
	SettingLeafNode* variable = static_cast<SettingLeafNode*>(clientData);
	
	if (flags & TCL_TRACE_READS) {
		Tcl_SetVar(interp, part1, variable->getValueString().c_str(), 0);
	}
	if (flags & TCL_TRACE_WRITES) {
		try {
			string newValue = Tcl_GetVar(interp, part1, 0);
			variable->setValueString(newValue);
		} catch (CommandException& e) {
			Tcl_SetVar(interp, part1, variable->getValueString().c_str(), 0);
			static_string = e.getMessage();
			return const_cast<char*>(static_string.c_str());
		}
	}
	if (flags & TCL_TRACE_UNSETS) {
		variable->restoreDefault();
		Tcl_SetVar(interp, part1, variable->getValueString().c_str(), 0);
		Tcl_TraceVar(interp, part1,
		             TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		             traceProc, static_cast<ClientData>(variable));
	}
	return NULL;
}

} // namespace openmsx

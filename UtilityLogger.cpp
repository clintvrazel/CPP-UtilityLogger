// UtilityLogger.cpp : main project file.
------------------------------------------------------------------------------
/* Author: clintvrazel@gmail.com
 Logger class allows logging of errors and events
 to an output text file or the system event log 
 with a threshold of detail.

 Logger settings can be written or loaded with configuration files
 for easy reuse and modification. 

 ------------------------------------------------------------------------------
 *Project Motivation*
 Not all errors are the same, and the need to see or 
 screen out some messages but not others in different parts
 of a program is not easily managed with asserts or exceptions.

 Further, there is often a need for different logfiles throughout, 
 with the option to append to or overwrite existing files.

 Finally, some applications may need logged files
 or to use a Windows Event Log.
 
 For these needs, I have created the Logger class.
 ------------------------------------------------------------------------------
 
 Logger class usage:

 *Declaring and Initializing*
 Logger mylog;
 mylog.Initialize();  - all Logger object must be Initialized.
 ------------------------------------------------------------------------------
 
 **Configuration**
 You can pass all configurations here as parameters, or let defaults be assigned.
 Configurations can be changed individually with corresponding set_ methods or by 
 specifying a configuration file

 mylog.set_config_file_name("CustomConfigFileName");    - loads a custom config file
 mylog.get_config_file_name();							- returns currently associated config filename
 mylog.make_config_file_ok(true) - defaults to true, turn off if you don't want to overwrite or make a config file with current settings
 mylog.WriteConfigFile( optional string <filename> )	- writes current config to file
 ------------------------------------------------------------------------------
 
 *Config File:
 - Must be plain text, separate by whitespace. Tabs, spaces, returns okay.
 - Each line look like the following:  property_name property_value 
 - properties can come in any order
 - if the same property occurs twice in a config file, the last valid property_value will be used
 ------------------------------------------------------------------------------
 
 *Sample Config File*
 *******************************
 log_file_name	"MyLogFile.log"*	- logs to this file, default is DEFAULT_LOG_FILE_NAME, or LoggerDefault.log
 log_mode	0				   *	- 0 or to_log, 1 or to_system
 verbosity	failureaudit	   *	- from 0 to 5 (NUM_VERBOSITY_LEVELS), or none, information, warning, error, successaudit, FailureAudit
 append_logs_ok	1			   *    - 1 or true, to append to log file, 0 or false to overwrite
 make_config_file_ok	1	   *	- 1 or true to write config file with Logger::WriteConfigFile(optional filename string)
 *******************************
 ------------------------------------------------------------------------------
 
 *Level of Verbosity*
 The verbosity levels are the same as those used by Windows.  
 From least to most messages: none, information, warning, error, successaudit, failureaudit, all
 
 mylog.set_verbosity_threshold( verbosity )  - defaults to information. All messages above the threshold will not be logged. 
 ------------------------------------------------------------------------------
 
 *Log Modes*
 There are two modes of logging: to a logfile and to a system log

 mylog.set_log_mode(to_log); 
 -OR-
 mylog.set_log_mode(to_system);

 ------------------------------------------------------------------------------
 
 *Logfiles*
 
 Logfiles are written by default to DEFAULT_LOG_FILE_NAME. 
 This is easy to change, and can be changed throughout the 
 life of the Logger object, or different logfiles can be 
 associated with other Logger ojects.

 - mylog.set_log_file_name("YourLogFile.log"); 
 ------------------------------------------------------------------------------
 
 *Overwriting or appending to logfiles*
 
 Logger objects append by default.
 If you wish to overwrite a logfile: 

 mylog.set_append_logs_ok(false) 
 ------------------------------------------------------------------------------
 
 *Windows Event Logs*
 log_mode must be set to to_system for Windows Event Logging. Default is to_log.

 Windows Log choices are the following: app_log (Application Log), sys_log (System Log), 
 or a custom log given by a string. 
 
 NOTE: successaudit and failureaudit will show up as "Information" 
 if written to Windows Event Logs other than security.

 mylog.set_win_log_name( app_log OR sys_log OR custom_log );
 
 Custom Log NOTE: If using a custom_log, set the win_log_name to custom, then
 use mylog.set_log_file_name("YourCustomWinLogName");
 
 ------------------------------------------------------------------------------
 
 *Logging Messages"
 
 There two equivalent methods to log messages:

 Method 1: Writes a <verbosity> <message> log item if the message verbosity does not exceed verbosity_threshold
 Ex: mylog.Log("Very minor detail", error) will only be logged if verbossity_level is successaudit or failureaudit
 Ex log: error very minor detail

 The general syntax is: 
 mylog.Log( string message, optional verbosity level, optional windows log name) 

 Method 2: - specify the verbosity in the method name. 

 mylog.Information( std::string message )
 mylog.Warning( std::string message )
 mylog.Error( std::string message )
 mylog.SuccessAudit( std::string message ) 
 mylog.FailureAudit( std::string message ) 
 ------------------------------------------------------------------------------
*/

#include "stdafx.h" 
#include <stdio.h>  
#include <string>
#include <fstream>  
#include <iostream>
#include <istream>

// Logger is an unmanaged class, gcroot is used
// for managed EventLog class pointer member of
// Logger for system logging.
#using <mscorlib.dll>
#include <vcclr.h>

// EventLog class
#using <System.dll>	
using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

using namespace std;

enum mode { to_log = 0, to_system };
enum verbosity { none = 0, information, warning, error, successaudit, failureaudit, all };
enum win_log { app_log = 0, sys_log, custom_log };

const string DEFAULT_SOURCE_NAME = "YourCPPApplication";
const string DEFAULT_LOG_FILE_NAME = "LoggerDefault.log";
const string DEFAULT_CONFIG_FILE_NAME = "LoggerConfig.ini";
const mode DEFAULT_LOG_MODE = to_log;
const verbosity DEFAULT_VERBOSITY = information;
const win_log DEFAULT_WIN_LOG_NAME = app_log;

const int NUM_CONFIG_OPTIONS = 5;
const int NUM_VERBOSITY_LEVELS = 7;
const int NUM_MODE_NAMES = 2;

const string mode_names [NUM_MODE_NAMES] = { "to_log", "to_system" };

const string verb_names [NUM_VERBOSITY_LEVELS] = { "none", "information", "warning", "error", 
								"successaudit", "failureaudit", "all" };

const string config_options [NUM_CONFIG_OPTIONS] = { "log_file_name", "log_mode", 
	"verbosity", "append_logs_ok", "make_config_file_ok" };

ostream& operator<<(ostream &out, verbosity v) {
	out << verb_names[v];
	return out;
}

ostream& operator<<(ostream &out, mode m) {
	out << mode_names[m];
	return out;
}

class Logger {
	
  private: 
	string config_file_name;  
	string log_file_name;      

	mode log_mode;
	verbosity verbosity_threshold;
	
	ifstream config_file;
	ofstream log_file;

	bool make_config_file_ok;
	bool append_logs_ok;

	gcroot<EventLog^> system_log; // managed EventLog inside unmanaged C++

	string source_name;
	win_log win_log_name;

private: 
	// Helper Functions
	bool MakeBoolFromString(const string& bool_string) {
		if (bool_string == "true") {  return true; }
		else if (bool_string == "false") { return false; } 
	}

	bool IsBool(const string& str) {
		if (MakeBoolFromString(str) && !(!MakeBoolFromString(str))) { 
			return true; 
		}
		else {
			return false;
		}
	}

	String^ CStringToSystemString(string c_string) {
		return gcnew String(c_string.c_str());
	}

	String^ WinLogEnumToSystemString(win_log user_win_log) {
		switch (user_win_log) {
		case 0: 
			return "Application"; 
		case 1:
			return "System";
			break;
		case 2:
			return CStringToSystemString(log_file_name);
			break;
		default:
			return "Application";
			break;
		}
	}

  public:
	
	// Logger must be initialized. Afterwards, each property can be set individually
	// using the appropriate setter
	void Initialize();
	
	// Tags a message with a given verbosity for logging. 
	// Will only log if verbosity >= verbosity_threshold (Default: 1 = information)
	// verbosity argument is optional - defaults to current verbosity threshold
	void Log(const string& message, const verbosity& message_verbosity);

	// if make_config_file_ok is set to true, will write the current configuration to file
	void WriteConfigFile(string);
	
	// alternate, easier calling syntax that makes verbosity of message clear
	void Information(const string& message) { this->Log(message, information); }
	void Warning(const string& message) { this->Log(message, warning); }
	void Error(const string& message) { this->Log(message, error); }
	void SuccessAudit(const string& message) { this->Log(message, successaudit); }
	void FailureAudit(const string& message) { this->Log(message, failureaudit); }

	// Getters and Setters

	// *verbosity_threshold*
	verbosity get_verbosity_threshold() { return verbosity_threshold; }

	// overloaded to accept int verbosity levels: 0 - NUM_VERBOSITY_LEVELS or 
	// enumerated: none, information, warning, error, critical, successaudit, failureaudit, all
	void set_verbosity_threshold(int user_verbosity) {
		if (user_verbosity >= 0 && user_verbosity <= NUM_VERBOSITY_LEVELS) {
			verbosity_threshold = static_cast<verbosity>(user_verbosity);
		}
	}

	void set_verbosity_threshold(verbosity user_verbosity) {
		verbosity_threshold = user_verbosity;
	}
	
	// * config_file_name *
	string get_config_file_name() { return config_file_name; } 

	int set_config_file_name(string user_config_name) { 

		if (user_config_name == config_file_name) {  
			cout << "no change no load" << endl;  // no changes if already using the file
			return 1;
		}

		if (user_config_name.size() > FILENAME_MAX) {
			user_config_name = DEFAULT_CONFIG_FILE_NAME;
			cout << "Error: User filename too long. Using " << config_file_name << endl;			
		}
		
		if (config_file.is_open()) { config_file.close(); } 
	
		// Use default LoggerConfig.ini if specified file not found or none indicated
		
		config_file.open(user_config_name, ios::in);
		if (!config_file) {
			cout << user_config_name << " could not be opened as config file." << endl;
			config_file.open(DEFAULT_CONFIG_FILE_NAME);
		}
		
		// Load config file - separate function?
		int config_count = 0;
		while (config_file) { 
			string config_property, config_parameter, config_line; 
			config_file >> config_property >> config_parameter;  // pull a property and value from a line
			if(config_file) { 
				//cout << "property: " << config_property << " value: " << config_parameter << endl; 
				
				int config_pos = -1;
				for (int pos = 0; pos < NUM_CONFIG_OPTIONS; pos++) {
					if (config_property == config_options[pos]) {  
						config_pos = pos;
					}
				}
				// load parameters into their corresponding members of Logger
				int index;
				bool flag;
					switch (config_pos) {  
					case 0: 
						 log_file_name = config_parameter;
						 config_count++;
					     break;
					case 1: 
						 for (index = 0; index < NUM_MODE_NAMES; index++) {
							 if (config_parameter == mode_names[index]) {
								 this->set_log_mode(index);
								 config_count++;
								 break;
							 }
						 }
					     break;
					case 2:
						 for (index = 0; index < NUM_VERBOSITY_LEVELS; index++) {
								if (config_parameter == verb_names[index]) {
									this->set_verbosity_threshold(index);
									config_count++;
									break;
								}
						 }
						 break;
					case 3:
						if (IsBool(config_parameter)) {
							flag = MakeBoolFromString(config_parameter);
							if (flag != append_logs_ok) {
								this->set_append_logs_ok(flag);	    
							}
							config_count++;
						}
						break;
					case 4:
						if (IsBool(config_parameter)) {
							flag = MakeBoolFromString(config_parameter);
							if (flag != make_config_file_ok) {
							  this->set_make_config_file_ok(flag);
							}
							config_count++;
						}
						break;
					
					case -1: 
					default:
						cout << "not a valid configuration" << endl;
						 break;
				}
				//cout << config_count << " configurations loaded" << endl;
			}
		}
		
		if (config_count > 0) { 
			config_file_name = user_config_name; 
			//cout << "config file " << config_file_name << " loaded." << endl;
			config_file.close();
			return 1;
		}
		else if (config_count == 0) {
			//cout << "no valid configurations found in " << user_config_name << endl; 
			return 0;
		}
		else {
			return 0;		
		}
	cout << "ERROR: no load" << endl;
	return 0;
	}

	// * log_file_name *
	string get_log_file_name() { return log_file_name; }

	
	// For string log_file_name for file logging in mode to_log
	void set_log_file_name(const string& user_log_file_name) {
		if (log_file.is_open()) {
			log_file.close();
		}
		if (user_log_file_name.size() < FILENAME_MAX + 1) {
			log_file_name = user_log_file_name;
		}
	}

	// * win_log_name
	win_log get_win_log_name() { return win_log_name; } 
	// For enum win_log for Windows Event logging in mode to_system
	void set_win_log_name(const win_log& user_win_log_name) {
		if (user_win_log_name != win_log_name) {
			win_log_name = user_win_log_name;
			system_log->Source = CStringToSystemString(source_name);
		}
	}

	// Can create a custom Windows Event Log
	void set_win_log_name(const string& custom_win_log_name) {
		String^ s_source_name = CStringToSystemString(source_name);
		String^ s_custom_win_log_name = CStringToSystemString(custom_win_log_name);
		if (!EventLog::SourceExists(s_source_name)) {
			EventLog::CreateEventSource(s_source_name, CStringToSystemString(custom_win_log_name));
		}
		system_log = gcnew EventLog;
		system_log->Source = s_source_name;
		system_log->Log = CStringToSystemString(custom_win_log_name);
	}

	// * log_mode *
	mode get_log_mode() { return log_mode; }

	// overloaded to accept integers 0-1 or enumerated to_log or to_system
	void set_log_mode(int user_log_choice) {	
		set_log_mode(static_cast<mode>(user_log_choice));
	}

	void set_log_mode(mode user_log_choice) {
		if (log_mode != user_log_choice) {
			// switching from file to system event logging
			if (user_log_choice == to_system || static_cast<mode>(user_log_choice) == 1) { 
				String^ s_source_name = CStringToSystemString(source_name);
				if (!EventLog::SourceExists(s_source_name)) {
					cout << "creating source " << source_name << endl;
					EventLog::CreateEventSource(s_source_name, WinLogEnumToSystemString(win_log_name));
				}
				system_log = gcnew EventLog;
				system_log->Source = s_source_name;
			}
			// switching from system event to file logging
			else if (user_log_choice == to_log || static_cast<mode>(user_log_choice) == 0) { 
				system_log->Close();
			}
			log_mode = user_log_choice;
		}
	}
	// * make_config_file_ok *
	bool get_make_config_file_ok() { return make_config_file_ok; }

	void set_make_config_file_ok(const bool& make_config_ok) {
		// only change setting if different from current setting
		if ((make_config_ok || !make_config_ok) && make_config_ok != make_config_file_ok) {
			make_config_file_ok = make_config_ok;
		}
	} 
		
	// * append_logs_ok() *
	bool get_append_logs_ok() { return append_logs_ok; }

	void set_append_logs_ok(const bool& user_append_ok) {
		if (append_logs_ok && !user_append_ok) {  // changing from append to not 
			if (log_file.is_open()) {
				log_file.close();	
			}
			append_logs_ok = user_append_ok;
			log_file.open(log_file_name);
			if (log_file) {  // check file is available for writing
				log_file.close();
			}
		}
		else if (!append_logs_ok && user_append_ok) {  // changing from no append to append
			if (log_file.is_open()) {			  
				log_file.close();
			}
			append_logs_ok = user_append_ok;
			log_file.open(log_file_name, ios::app);
			if (log_file) {
			log_file.close(); 
			}
		}
	}
};

// Initialize must be called once a Logger object is declared
void Logger::Initialize() {
	verbosity_threshold = DEFAULT_VERBOSITY;
	log_file_name = DEFAULT_LOG_FILE_NAME;
	config_file_name = DEFAULT_CONFIG_FILE_NAME;
	make_config_file_ok = true;
	append_logs_ok = true;
	log_mode = DEFAULT_LOG_MODE;
	source_name = DEFAULT_SOURCE_NAME;
	win_log_name = DEFAULT_WIN_LOG_NAME;
	system_log = nullptr;
}

// Logs a message to the specified destination. Defaults LoggerDefault.log. 
// Change DEFAULT_LOG_FILE_NAME or use mylog.set_log_file_name("InsertNameHere")

void Logger::Log(const string& message, const verbosity& message_verbosity = all) {
	// logging to file
	if (message_verbosity <= verbosity_threshold && message_verbosity != 0) { 
		if (log_mode == to_log || log_mode == 0) {
			if (append_logs_ok) {
				log_file.open(log_file_name, ios::app);
				if (log_file) {
					log_file << message_verbosity << "\t" << message << endl;
					//cout << "logging a " << message_verbosity << " message now under or matching " << verbosity_threshold << endl;
					log_file.close();
				}
			}
			else if (log_file) {
				log_file << message_verbosity << "\t " << message << endl;
				cout << "logging a " << message_verbosity << " message now under or matching " << 
				verbosity_threshold << "without appending" << endl;
				log_file.close();
			}
		}
		// log to Windows Application Log
		else if (log_mode == to_system || log_mode == 1) {
			String^ s_message = gcnew String(message.c_str());
			String^ s_source_name = gcnew String(source_name.c_str());
			switch (message_verbosity) {
			case 0: 
				cout << "message_verbosity set to none, unable to log" << endl;
				return;
				break;
			case 1: 
				system_log->WriteEntry(s_source_name, s_message, EventLogEntryType::Information);
				break;
			case 2: 
				system_log->WriteEntry(s_source_name, s_message, EventLogEntryType::Warning);
				break;
			case 3: 
				system_log->WriteEntry(s_source_name, s_message, EventLogEntryType::Error);
				break;
			case 4: 
				system_log->WriteEntry(s_source_name, s_message, EventLogEntryType::SuccessAudit);
				break;
			case 5:
				system_log->WriteEntry(s_source_name, s_message, EventLogEntryType::FailureAudit);
			default: 
				cout << "Invalid verbosity_threshold " << endl;
				break;
			}
		}
	}
}

void Logger::WriteConfigFile(string user_config_file = "") {

	if (user_config_file == "") { user_config_file = config_file_name; }
	
	if (make_config_file_ok) {
		ofstream config_file_out;
		config_file_out.open(user_config_file);
		if (config_file_out) {
				config_file_out << config_options[0] << "\t" << log_file_name << endl;
				config_file_out << config_options[1] << "\t" << log_mode << endl;
				config_file_out << config_options[2] << "\t" << verbosity_threshold << endl;
				config_file_out << config_options[3] << "\t" << append_logs_ok << endl;
				config_file_out << config_options[4] << "\t" << make_config_file_ok << endl;
				config_file_out.close();
				cout << "Config file " << user_config_file << " written successfully." << endl;
		}
		else {
			cout << "Unable to open " << user_config_file << " for writing." << endl;
		}
	}
	else {
		cout << "make_config_file_ok flag not set to \"true\", use \"mylog.set_make_config_file_ok(true)\"" << endl;
	}
}

void TestAccessors() {
	Logger log_accessor_tester;
	log_accessor_tester.Initialize();
	
	log_accessor_tester.set_verbosity_threshold(all);
	if (log_accessor_tester.get_verbosity_threshold() != all) { cout << "verbosity_threshold accessor fail" << endl; }
	else { cout << "PASS verbosity_threshold" << endl; }

	log_accessor_tester.set_log_file_name("RecentLog.log");
	if (log_accessor_tester.get_log_file_name() != "RecentLog.log") { cout << "log_file_name accessor fail" << endl; }
	else { cout << "PASS log_file_name" << endl; }

	log_accessor_tester.set_log_mode(to_system);
	if (log_accessor_tester.get_log_mode() != to_system) { cout << "log_mode accessor fail" << endl; }
	else { cout << "PASS log_mode" << endl; }

	log_accessor_tester.set_win_log_name(app_log);
	if (log_accessor_tester.get_win_log_name() != app_log) { cout << "win_log_name accessor fail" << endl; }
	else { cout << "PASS win_log_name" << endl; }

	if (log_accessor_tester.get_config_file_name() != DEFAULT_CONFIG_FILE_NAME) {
		cout << "default config_file_name fail" << endl;
	}
	else { cout << "PASS default config_file_name" << endl; }

	log_accessor_tester.set_config_file_name("LogAccessorConfig");
	if (log_accessor_tester.get_config_file_name() != "LogAccessorConfig") {
		cout << "config_file_name fail" << endl;
	}
	else { cout << "PASS config_file_name change" << endl; }
	
}

void TestConfigMethods() {

	Logger config_method_tester;
	config_method_tester.Initialize();
	
	config_method_tester.set_make_config_file_ok(false);
	// should fail
	config_method_tester.WriteConfigFile();

	config_method_tester.set_make_config_file_ok(true);
	// test overwriting an existing config file
	config_method_tester.set_config_file_name("TestConfigName.ini");
	config_method_tester.WriteConfigFile();
	config_method_tester.set_config_file_name("TestConfigName.ini");

	// test writing a config file with changes
	config_method_tester.set_config_file_name("TestConfigName2.ini");
	config_method_tester.set_verbosity_threshold(information);
	config_method_tester.set_log_mode(to_system);
	config_method_tester.WriteConfigFile();
	config_method_tester.WriteConfigFile();

	// Appending Logs Test
	config_method_tester.set_log_file_name("AppendLogsTest.test");
	config_method_tester.set_verbosity_threshold(all);
	config_method_tester.set_append_logs_ok(false);
	// shoud overwrite the file log
	config_method_tester.Warning("Should be at top of log Line A");
	config_method_tester.set_append_logs_ok(true);
	config_method_tester.Information("Logged to the bottom after Line A");

}

void TestLogMethods() {

	Logger log_method_tester;
	log_method_tester.Initialize();

	log_method_tester.set_log_mode(to_log);
	log_method_tester.set_log_file_name("log methods.test");
	log_method_tester.set_verbosity_threshold(all);

	log_method_tester.Warning("Warning to Application log");
	log_method_tester.Error("An Error");
	log_method_tester.Information("Some Info");
	log_method_tester.SuccessAudit("A successful audit");
	log_method_tester.FailureAudit("now a failing audit");

}

// Tests all the functionality of the Logger class
void TestSuite() {
	TestAccessors();
	TestConfigMethods();
	TestLogMethods();
}

int main(int argc, char *argv[])
{
	TestSuite();
	system("Pause");
	return 0;
}

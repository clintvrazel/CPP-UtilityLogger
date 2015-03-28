 Author: clintvrazel@gmail.com
 Date: March 2015
 IDE: Microsoft Visual Studio 2010 Express 
 
 Logger class allows logging of errors and events
 to an output text file or the Windows system event log 
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
 
 *Project Challenges*
 - Mixing managed EventLog and System::Strings with 
   native C++ unmanaged types
 - Gathering user feedback for future refinements
 - Separating functionality
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

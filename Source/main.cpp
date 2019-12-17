/**************************************************************************************\
** File: main.cpp
** Project: Lex - Custom Linux Shell
** Author: David Leksen
** Date:
\**************************************************************************************/
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#include <unistd.h>
#include <sys/wait.h>

// Operators/Separators
const char WHITE_SPACE_CHAR_1{ ' ' };
const char WHITE_SPACE_CHAR_2{ '\t' };
const std::string COMMAND_SEPARATOR_1{ "&&" };
const std::string COMMAND_SEPARATOR_2{ ";" };
const std::string REDIRECT_OUTPUT_OPERATOR{ ">" };
const std::string REDIRECT_OUTPUT_APPEND_OPERATOR{ ">>" };
const std::string REDIRECT_INPUT_OPERATOR{ "<" };

// Commands
const std::string SHELL_NAME{ "lex" };
const std::string QUIT_COMMAND_1{ "exit" };
const std::string QUIT_COMMAND_2{ "quit" };
const std::string CHANGE_DIRECTORY_COMMAND{ "cd" };
const std::string CHANGE_TO_LAST_DIRECTORY_COMMAND{ "cdl" };
const std::string DISPLAY_HISTORY_COMMAND{ "history" };
const std::string EXECUTE_HISTORY_COMMAND{ "!" };

// Console text formatting
const std::string TEXT_STYLE_DEFAULT{ "\033[0m" };
const std::string TEXT_STYLE_GREEN_ON_DEFAULT{ "\033[0;32;49m" };
const std::string TEXT_STYLE_BLUE_ON_DEFAULT{ "\033[0;34;49m" };
const std::string TEXT_STYLE_RED_ON_DEFAULT{ "\033[0;31;49m" };
const std::string TEXT_STYLE_YELLOW_ON_DEFAULT{ "\033[0;33;49m" };
const std::string TEXT_STYLE_GREEN_ON_DEFAULT_BOLD{ "\033[1;32;49m" };
const std::string TEXT_STYLE_BLUE_ON_DEFAULT_BOLD{ "\033[1;34;49m" };
const std::string TEXT_STYLE_RED_ON_DEFAULT_BOLD{ "\033[1;31;49m" };
const std::string TEXT_STYLE_YELLOW_ON_DEFAULT_BOLD{ "\033[1;33;49m" };
const std::string TEXT_STYLE_CYAN_ON_DEFAULT_BOLD{ "\033[1;36;49m" };

struct Command
{
	std::string name;
	std::vector<std::string> arguments;

	bool operator==(const Command& other) const
	{
		if(name != other.name)
			return false;
		if(arguments != other.arguments)
			return false;
		return true;
	}
	bool operator!=(const Command& other) const
	{
		return !(*this == other);
	}
};
class ExecutedCommand
{
public:
	ExecutedCommand(std::list<Command>::iterator newHistoryIter, std::list<Command>& newCommandListRef)
		: m_isHistory{ true },
		m_historyIter{ newHistoryIter },
		m_commandListRef{ newCommandListRef }{ }
	ExecutedCommand(const Command& newCommand, std::list<Command>& newCommandListRef)
		: m_isHistory{ false },
		m_command{ newCommand },
		m_commandListRef{ newCommandListRef }{ }
	void MoveToFront() const
	{
		if(m_isHistory)
			m_commandListRef.splice(m_commandListRef.begin(), m_commandListRef, m_historyIter);
		else
			m_commandListRef.push_front(m_command);
	}
	bool operator==(const ExecutedCommand& other) const
	{
		if(m_isHistory != other.m_isHistory)
			return false;
		if(m_commandListRef != other.m_commandListRef)
			return false;
		if(m_isHistory)
		{
			if(m_historyIter == other.m_historyIter)
				return true;
			else
				return false;
		}
		else
		{
			if(m_command == other.m_command)
				return true;
			else
				return false;
		}
	}
	bool operator!=(const ExecutedCommand& other) const 
	{
		return !(*this == other);
	}

private:
	bool m_isHistory;
	std::list<Command>::iterator m_historyIter;
	Command m_command;
	std::list<Command>& m_commandListRef;
};

// Supports cdl command
std::string lastWorkingDirectory;

// This is small so the terminal doesn't get flooded
const std::list<Command>::size_type MAX_HISTORY_SIZE{ 10u };

// Front represents most recent command; this is persistent
std::list<Command> historyList;

// Back represents most recent command; this gets used and cleared between every user entry
std::list<ExecutedCommand> executedCommandList;

//std::list< std::list<Command>::iterator > historyCommandsExecutedList;

//+------------------------\----------------------------------
//|		   Commands		   |
//\------------------------/----------------------------------
void ExecuteCommand(const Command& command, 
	const std::string& inputFilename = "",
	const std::string& outputFilename = "", bool append = false);
void ChangeWorkingDirectory(std::string directory);
void ExecuteExternalApp(const Command& command);

//+------------------------\----------------------------------
//|		   Utility		   |
//\------------------------/----------------------------------
void ClearTerminal();
void PrintPrompt();
void SeparateIntoCommands(const std::vector<std::string>& wordList, std::vector<Command>& commandListOut);
void SeparateIntoWords(const std::string& input, std::vector<std::string>& wordListOut);
void PrintCommand(const Command& command);
void PrintWordList(const std::vector<std::string>& wordList);
void PrintHistory();
void GetUser(std::string& userOut);
void ExpandDirectory(std::string& directory);
void GetHomeDirectory(std::string& homeOut);
void GetWorkingDirectory(std::string& pathOut);

//+------------------------\----------------------------------
//|			 Main		   |
//\------------------------/----------------------------------
void DoShell();
int main()
{
	try
	{
		DoShell();
	}
	catch(const std::exception & e)
	{
		std::cerr << SHELL_NAME << ": Fatal Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
void DoShell()
{
	ClearTerminal();
	GetWorkingDirectory(lastWorkingDirectory);
	while(true)
	{
		PrintPrompt();

		// Get list of commands from command-line
		std::vector<Command> commandList;
		{
			// Get line of input, and tokenize
			std::vector<std::string> wordList;
			{
				std::string inputLine;
				getline(std::cin, inputLine);
				SeparateIntoWords(inputLine, wordList);
			}

			// Convert to list of commands
			SeparateIntoCommands(wordList, commandList);
		}

		// Execute list of commands
		for(const Command& command : commandList)
		{
			if(command.name == QUIT_COMMAND_1 || command.name == QUIT_COMMAND_2)
				return;

			ExecuteCommand(command);
		}

		// Move/add executed commands to front
		for(const ExecutedCommand& ec : executedCommandList)
			ec.MoveToFront();
		executedCommandList.clear();
		
		
		// Erase oldest entries if history is overflowing
		if(historyList.size() > MAX_HISTORY_SIZE)
		{
			std::list<Command>::iterator rangeStart{ historyList.begin() };
			std::advance(rangeStart, MAX_HISTORY_SIZE);

			historyList.erase(rangeStart, historyList.end());
		}
		
	}
}

//+------------------------\----------------------------------
//|		   Commands		   |
//\------------------------/----------------------------------
void ExecuteCommand(const Command& command,
	const std::string& inputFilename,
	const std::string& outputFilename, bool append)
{
	if(command.name.empty())
		return;

	if(command.name == DISPLAY_HISTORY_COMMAND)
	{
		PrintHistory();
		return;
	}

	// Set default new, non-history command
	std::unique_ptr<ExecutedCommand> newExecutedCommandPtr = std::make_unique<ExecutedCommand>(command, historyList);
	const Command* commandToExecutePtr{ &command };

	// If history execution command, reset previous two variables accordingly
	if(command.name == EXECUTE_HISTORY_COMMAND)
	{
		if(command.arguments.size() > 1)
			std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": too many arguments" << std::endl;
		else if(command.arguments.empty())
			std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": missing operand" << std::endl;
		else
		{
			try
			{
				// Convert index from string to number and range check
				std::list<Command>::size_type historyIndex{ std::stoul(command.arguments[0]) };
				if(historyIndex > MAX_HISTORY_SIZE || historyIndex >= historyList.size())
				{
					std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": index out of range" << std::endl;
					return;
				}
				else
				{
					// Get iterator at index
					std::list<Command>::iterator commandIter{ historyList.begin() };
					std::advance(commandIter, historyIndex);

					// Setup command in history for execution
					newExecutedCommandPtr = std::make_unique<ExecutedCommand>(commandIter, historyList);
					commandToExecutePtr = &*commandIter;
				}
			}
			catch(const std::logic_error & e)
			{
				std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": invalid operand" << std::endl;
				return;
			}
		}
	}

	// Add new executed command or move existing executed command to back
	auto executedCommandIter = std::find(executedCommandList.begin(), executedCommandList.end(), *newExecutedCommandPtr);
	if(executedCommandIter != executedCommandList.end())
		executedCommandList.splice(executedCommandList.end(), executedCommandList, executedCommandIter);
	else
		executedCommandList.push_back(*newExecutedCommandPtr);

	if(commandToExecutePtr->name == CHANGE_DIRECTORY_COMMAND)
	{
		if(commandToExecutePtr->arguments.size() > 1)
			std::cerr << SHELL_NAME << ": " << CHANGE_DIRECTORY_COMMAND << ": too many arguments" << std::endl;
		else if(commandToExecutePtr->arguments.empty())
			ChangeWorkingDirectory("");
		else
			ChangeWorkingDirectory(commandToExecutePtr->arguments[0]);
	}
	else if(commandToExecutePtr->name == CHANGE_TO_LAST_DIRECTORY_COMMAND)
	{
		if(!commandToExecutePtr->arguments.empty())
			std::cerr << SHELL_NAME << ": " << CHANGE_TO_LAST_DIRECTORY_COMMAND << ": too many arguments" << std::endl;
		else
			ChangeWorkingDirectory(lastWorkingDirectory);
	}
	else
		ExecuteExternalApp(*commandToExecutePtr);
}

void ChangeWorkingDirectory(std::string directory)
{
	// Retain current working directory for cdl command
	std::string savedWorkingDirectory;
	GetWorkingDirectory(savedWorkingDirectory);

	// Change directories
	ExpandDirectory(directory);
	errno = 0;
	if(!chdir(directory.c_str()))
	{
		// Overwrite last directory if we successfully changed directories.
		lastWorkingDirectory = savedWorkingDirectory;
	}
	else
	{
		std::string message{ SHELL_NAME + ": " + CHANGE_DIRECTORY_COMMAND + ": \'" + directory + "\'" };
		perror(message.c_str());
	}
}

void ExecuteExternalApp(const Command& command)
{
	if(command.name.empty())
		return;

	errno = 0;
	pid_t child_pid;
	child_pid = fork();

	// If fork failed, print error
	if(child_pid < 0)
	{
		std::string message{ SHELL_NAME + ": " + command.name };
		perror(message.c_str());
	}

	// Child
	if(child_pid == 0)
	{
		// Convert command to c-style string list 
		std::vector<char*> cStyleStringList;
		{
			// Separate path from app name and only put name in argv[0] while sending the full path to execvp 
			{
				std::string appName;
				{
					std::size_t endOfPathIndex{ command.name.find_last_of('/') };
					if(endOfPathIndex == std::string::npos)
						appName = command.name;
					else
						appName = command.name.substr(endOfPathIndex + 1);
				}
				cStyleStringList.emplace_back(const_cast<char*>(appName.c_str()));
			}

			// Add arguments
			for(auto const& arg : command.arguments)
				cStyleStringList.emplace_back(const_cast<char*>(arg.c_str()));

			// exec expects null-terminated array
			cStyleStringList.push_back(nullptr);
		}

		// execute program
		errno = 0;
		int result = execvp(command.name.c_str(), cStyleStringList.data());

		// If execvp failed, print error and terminate child process
		if(result < 0)
		{
			std::string message{ SHELL_NAME + ": " + command.name };
			perror(message.c_str());
			exit(EXIT_FAILURE);
		}
	}
	// Parent
	else
	{
		// Wait for child to finish
		errno = 0;
		if(waitpid(child_pid, nullptr, WUNTRACED) < 0)
		{
			// Print error, if any
			std::string message{ SHELL_NAME + ": " + command.name };
			perror(message.c_str());
		}
	}
}

//+------------------------\----------------------------------
//|		   Utility		   |
//\------------------------/----------------------------------
void ClearTerminal()
{
	Command clearCommand;
	clearCommand.name = "clear";
	ExecuteExternalApp(clearCommand);
}
void PrintPrompt()
{
	const std::string& shellStyle{ TEXT_STYLE_BLUE_ON_DEFAULT_BOLD };
	const std::string& userStyle{ TEXT_STYLE_GREEN_ON_DEFAULT_BOLD };
	const std::string& directoryStyle{ TEXT_STYLE_BLUE_ON_DEFAULT_BOLD };
	const std::string& punctuationStyle{ TEXT_STYLE_DEFAULT };

	std::cout << shellStyle << SHELL_NAME;
	{
		std::string user;
		GetUser(user);
		if(!user.empty())
			std::cout << punctuationStyle << '(' << userStyle << user << punctuationStyle << ')';
	}
	std::cout << punctuationStyle << ':';
	{
		std::string workingDirectory;
		GetWorkingDirectory(workingDirectory);

		std::cout << directoryStyle << workingDirectory
			<< punctuationStyle << "$ " << TEXT_STYLE_DEFAULT;
	}
}
void SeparateIntoCommands(const std::vector<std::string>& wordList, std::vector<Command>& commandListOut)
{
	// Go word by word, finding the start and end of each command, and making a list
	commandListOut.clear();
	for(std::vector<std::string>::size_type currentWord = 0, currentCommandStart = 0; currentWord < wordList.size(); ++currentWord)
	{
		// If current word is command separator
		if(wordList[currentWord] == COMMAND_SEPARATOR_1 || wordList[currentWord] == COMMAND_SEPARATOR_2)
		{
			// If current command has any words, record the command
			if(currentWord > currentCommandStart)
			{
				// Record command name
				Command command;
				command.name = wordList[currentCommandStart];

				// Record arguments, if any
				for(std::vector<std::string>::size_type i = currentCommandStart + 1; i < currentWord; ++i)
					command.arguments.push_back(wordList[i]);

				// Record command
				commandListOut.push_back(command);
			}

			// Next command needs to start on next word
			currentCommandStart = currentWord + 1;
		}

		// If we are at the last word
		if(currentWord + 1 == wordList.size())
		{
			// If current command has any words, record the command
			if(currentWord + 1 > currentCommandStart)
			{
				// Record command name
				Command command;
				command.name = wordList[currentCommandStart];

				// Record arguments, if any
				for(std::vector<std::string>::size_type i = currentCommandStart + 1; i <= currentWord; ++i)
					command.arguments.push_back(wordList[i]);

				// Record command
				commandListOut.push_back(command);
			}
		}
	}
}
void SeparateIntoWords(const std::string& input, std::vector<std::string>& outputWordList)
{
	// Go char by char, finding the start and end of each word, and making a list
	outputWordList.clear();
	for(std::string::size_type currentChar = 0, currentWordStart = 0; currentChar < input.size(); ++currentChar)
	{
		// If current character is whitespace
		if(input[currentChar] == WHITE_SPACE_CHAR_1 || input[currentChar] == WHITE_SPACE_CHAR_2)
		{
			// If current word has any characters, record the word
			if(currentChar > currentWordStart)
				outputWordList.push_back(input.substr(currentWordStart, currentChar - currentWordStart));

			// Next word needs to start on next character, any time a whitespace character is encountered, no matter if a word was recorded or not
			currentWordStart = currentChar + 1;
		}

		// If we are at the last character
		if(currentChar + 1 == input.size())
		{
			// If current word has any characters, record the word
			if(currentChar + 1 > currentWordStart)
				outputWordList.push_back(input.substr(currentWordStart, (currentChar + 1) - currentWordStart));
		}
	}
}

void PrintCommand(const Command& command)
{
	std::cout << command.name << ' ';
	PrintWordList(command.arguments);
}
void PrintWordList(const std::vector<std::string>& wordList)
{
	for(std::vector<std::string>::size_type i = 0; i < wordList.size(); ++i)
	{
		std::cout << wordList[i];
		if(i < wordList.size() - 1)
			std::cout << ' ';
	}
}
void PrintHistory()
{
	if(historyList.empty()) 
	{
		std::cout << SHELL_NAME << ": " << DISPLAY_HISTORY_COMMAND << ": empty" << std::endl;
		return;
	}

	// Print history list in reverse so that most recent command is printed last
	std::list<Command>::size_type i{ historyList.size() - 1 };
	for(auto commandIter = historyList.rbegin(); commandIter != historyList.rend(); ++commandIter, --i)
	{
		std::cout << SHELL_NAME << ": ! " << i << ": ";
		PrintCommand(*commandIter);
		std::cout << std::endl;
	}
}

void GetUser(std::string& userOut)
{
	char* user{ getenv("USER") };
	if(!user)
		userOut.clear();
	else
		userOut = user;
}
void ExpandDirectory(std::string& directory)
{
	if(directory.empty())
	{
		// If no directory specified, change to root directory
		directory = "/";
	}
	else if(directory[0] == '~')
	{
		// Expand ~ to home directory (if we can find it)
		std::string expandedDirectory;
		GetHomeDirectory(expandedDirectory);
		if(expandedDirectory.empty())
		{
			std::cerr << SHELL_NAME << ": ~: Failed to find home directory" << std::endl;
			return;
		}

		// Add the remaining subdirectories
		if(directory.size() > 1)
			expandedDirectory += directory.substr(1, directory.length() - 1);

		directory = expandedDirectory;
	}
}
void GetHomeDirectory(std::string& homeOut)
{
	char* home{ getenv("HOME") };
	if(!home)
		homeOut.clear();
	else
		homeOut = home;
}
void GetWorkingDirectory(std::string& pathOut)
{
	char cwd[256];
	char* result;
	result = getcwd(cwd, sizeof(cwd));
	if(!result)
	{
		// Print error
		std::string message{ SHELL_NAME + ": Failed to get current working directory" };
		perror(message.c_str());
		pathOut.clear();
	}
	else
		pathOut = cwd;
}



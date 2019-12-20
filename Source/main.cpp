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
const std::string PIPING_OPERATOR{ "<" };

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

// Prompt styles
const std::string& SHELL_STYLE{ TEXT_STYLE_BLUE_ON_DEFAULT_BOLD };
const std::string& USER_STYLE{ TEXT_STYLE_GREEN_ON_DEFAULT_BOLD };
const std::string& DIRECTORY_STYLE{ TEXT_STYLE_BLUE_ON_DEFAULT_BOLD };
const std::string& PUNCUATION_STYLE{ TEXT_STYLE_DEFAULT };

struct Command
{
	std::string name;
	std::vector<std::string> arguments;
	//std::string inputFilename;
	//std::string outputFilename;
	//bool outputAppend;

	bool operator==(const Command& other) const
	{
		if(name != other.name)
			return false;
		if(arguments != other.arguments)
			return false;
		return true;
	}
	friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};

// Supports cdl command
std::string lastWorkingDirectory;

// History
using CommandListIndex = std::list<Command>::size_type;
CommandListIndex HISTORY_MAX_SIZE{ 1000 };
CommandListIndex HISTORY_DEFAULT_DISPLAY_SIZE{ 10 };
std::list<Command> commandHistory;
std::list<Command> executedCommands;

//+------------------------\----------------------------------
//|		    Parse		   |
//\------------------------/----------------------------------
void SeparateIntoWords(const std::string& input, std::vector<std::string>& wordListOut);
void SeparateIntoCommands(const std::vector<std::string>& wordList, std::vector<Command>& commandListOut);

//+------------------------\----------------------------------
//|		   Execute		   |
//\------------------------/----------------------------------
void ExecuteCommand(const Command& command);
bool StringToCommandListIndex(const std::string& stringIn, CommandListIndex& indexOut);
const Command* GetCommandPtr(const std::list<Command>& commandList, CommandListIndex index);
void ExecuteExternalApp(const Command& command);
void AddUniqueCommandToFront(const Command& sourceCommand, std::list<Command>& destinationList);

//+------------------------\----------------------------------
//|		   Directory	   |
//\------------------------/----------------------------------
void ChangeWorkingDirectory(std::string directory);
void ExpandDirectory(std::string& directory);
void GetHomeDirectory(std::string& homeOut);
void GetWorkingDirectory(std::string& pathOut);

//+------------------------\----------------------------------
//|		    Print		   |
//\------------------------/----------------------------------
void ClearTerminal();
void PrintPrompt();
void GetUser(std::string& userOut);
void PrintHistory(CommandListIndex numCommands);
void PrintWordList(const std::vector<std::string>& wordList);

//+------------------------\----------------------------------
//|			 Main		   |
//\------------------------/----------------------------------
int main()
{
	try
	{
		ClearTerminal();
		GetWorkingDirectory(lastWorkingDirectory);
		while(true)
		{
			PrintPrompt();

			// Get list of commands from command-line
			std::vector<Command> commands;
			{
				std::vector<std::string> wordList;
				{
					std::string inputLine;
					getline(std::cin, inputLine);
					SeparateIntoWords(inputLine, wordList);
				}
				SeparateIntoCommands(wordList, commands);
			}

			// Execute list of commands
			for(Command cmd : commands)
			{
				if(cmd.name == QUIT_COMMAND_1 || cmd.name == QUIT_COMMAND_2)
					return 0;

				ExecuteCommand(cmd);
			}

			// Record executed commands in history, oldest to most recent
			for(auto sourceIter{ executedCommands.rbegin() }; sourceIter != executedCommands.rend(); ++sourceIter)
				AddUniqueCommandToFront(*sourceIter, commandHistory);
			executedCommands.clear();

			// Trim history
			while(commandHistory.size() > HISTORY_MAX_SIZE)
				commandHistory.pop_back();
		}
	}
	catch(const std::exception & e)
	{
		std::cerr << SHELL_NAME << ": Fatal Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

//+------------------------\----------------------------------
//|		    Parse		   |
//\------------------------/----------------------------------
void SeparateIntoWords(const std::string& input, std::vector<std::string>& wordsOut)
{
	// Go char by char, finding the start and end of each word, and making a list
	wordsOut.clear();
	for(std::string::size_type currentChar = 0, currentWordStart = 0; currentChar < input.size(); ++currentChar)
	{
		// If current character is whitespace
		if(input[currentChar] == WHITE_SPACE_CHAR_1 || input[currentChar] == WHITE_SPACE_CHAR_2)
		{
			// If current word has any characters, record the word
			if(currentChar > currentWordStart)
				wordsOut.push_back(input.substr(currentWordStart, currentChar - currentWordStart));

			// Next word needs to start on next character, any time a whitespace character is encountered, no matter if a word was recorded or not
			currentWordStart = currentChar + 1;
		}

		// If we are at the last character
		if(currentChar + 1 == input.size())
		{
			// If current word has any characters, record the word
			if(currentChar + 1 > currentWordStart)
				wordsOut.push_back(input.substr(currentWordStart, (currentChar + 1) - currentWordStart));
		}
	}
}
void SeparateIntoCommands(const std::vector<std::string>& wordList, std::vector<Command>& commandsOut)
{
	// Go word by word, finding the start and end of each command, and making a list
	commandsOut.clear();
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
				commandsOut.push_back(command);
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
				commandsOut.push_back(command);
			}
		}
	}
}

//+------------------------\----------------------------------
//|		   Execute		   |
//\------------------------/----------------------------------
void ExecuteCommand(const Command& command)
{
	if(command.name.empty())
		return;

	if(command.name == DISPLAY_HISTORY_COMMAND)
	{
		CommandListIndex numCommands{ HISTORY_DEFAULT_DISPLAY_SIZE };
		if(command.arguments.size() > 1)
		{
			std::cerr << SHELL_NAME << ": " << DISPLAY_HISTORY_COMMAND << ": too many parameters" << std::endl;
			return;
		}
		else if(command.arguments.size() == 1)
		{
			if(!StringToCommandListIndex(command.arguments[0], numCommands))
			{
				std::cerr << SHELL_NAME << ": " << DISPLAY_HISTORY_COMMAND << ": invalid parameter" << std::endl;
				return;
			}
		}
		PrintHistory(numCommands);
		return;
	}

	// Point commandToExecutePtr to either input or entry in history
	const Command* commandToExecutePtr{ &command };
	if(command.name == EXECUTE_HISTORY_COMMAND)
	{
		if(command.arguments.size() > 1)
		{
			std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": too many parameters" << std::endl;
			return;
		}
		else if(command.arguments.empty())
		{
			std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": missing parameter" << std::endl;
			return;
		}
		else
		{
			CommandListIndex input;
			if(StringToCommandListIndex(command.arguments[0], input) && input != 0)
				commandToExecutePtr = GetCommandPtr(commandHistory, input - 1);
			else
				commandToExecutePtr = nullptr;

			if(!commandToExecutePtr)
			{
				std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": invalid parameter" << std::endl;
				return;
			}
		}
	}

	// Record command
	AddUniqueCommandToFront(*commandToExecutePtr, executedCommands);

	// Execute command
	if(commandToExecutePtr->name == CHANGE_DIRECTORY_COMMAND)
	{
		if(commandToExecutePtr->arguments.size() > 1)
			std::cerr << SHELL_NAME << ": " << CHANGE_DIRECTORY_COMMAND << ": too many parameters" << std::endl;
		else if(commandToExecutePtr->arguments.empty())
			ChangeWorkingDirectory("");
		else
			ChangeWorkingDirectory(commandToExecutePtr->arguments[0]);
	}
	else if(commandToExecutePtr->name == CHANGE_TO_LAST_DIRECTORY_COMMAND)
	{
		if(!commandToExecutePtr->arguments.empty())
			std::cerr << SHELL_NAME << ": " << CHANGE_TO_LAST_DIRECTORY_COMMAND << ": too many parameters" << std::endl;
		else
			ChangeWorkingDirectory(lastWorkingDirectory);
	}
	else
		ExecuteExternalApp(*commandToExecutePtr);
}
bool StringToCommandListIndex(const std::string& stringIn, CommandListIndex& indexOut)
{
	try {
		indexOut = (CommandListIndex)std::stoul(stringIn);
	}
	catch(const std::logic_error& e) {
		return false;
	}
	return true;
}
const Command* GetCommandPtr(const std::list<Command>& commandList, CommandListIndex index)
{
	if(index >= commandList.size())
		return nullptr;
	else if(index == commandList.size() - 1)
		return &commandList.back();
	else if(index == 0)
		return &commandList.front();
	else
	{
		auto it{ commandList.begin() };
		std::advance(it, index);
		return &*it;
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
void AddUniqueCommandToFront(const Command& sourceCommand, std::list<Command>& destinationList)
{
	auto foundExistingCommandIter{ std::find(destinationList.begin(), destinationList.end(), sourceCommand) };
	if(foundExistingCommandIter != destinationList.end())
	{
		if(foundExistingCommandIter != destinationList.begin())
			destinationList.splice(destinationList.begin(), destinationList, foundExistingCommandIter);
	}
	else
		destinationList.push_front(sourceCommand);
}

//+------------------------\----------------------------------
//|		   Directory	   |
//\------------------------/----------------------------------
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

//+------------------------\----------------------------------
//|		    Print		   |
//\------------------------/----------------------------------
void ClearTerminal()
{
	Command clearCommand;
	clearCommand.name = "clear";
	ExecuteExternalApp(clearCommand);
}
void PrintPrompt()
{
	std::cout << SHELL_STYLE << SHELL_NAME;
	{
		std::string user;
		GetUser(user);
		if(!user.empty())
			std::cout << PUNCUATION_STYLE << '(' << USER_STYLE << user << PUNCUATION_STYLE << ')';
	}
	std::cout << PUNCUATION_STYLE << ':';
	{
		std::string workingDirectory;
		GetWorkingDirectory(workingDirectory);

		std::cout << DIRECTORY_STYLE << workingDirectory
			<< PUNCUATION_STYLE << "$ " << TEXT_STYLE_DEFAULT;
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
void PrintHistory(CommandListIndex numCommands)
{
	// Print history list in reverse so that most recent command is printed last
	if(commandHistory.empty())
	{
		std::cout << SHELL_NAME << ": " << DISPLAY_HISTORY_COMMAND << ": empty" << std::endl;
		return;
	}
	if(numCommands > HISTORY_MAX_SIZE)
		numCommands = HISTORY_MAX_SIZE;
	CommandListIndex i{ commandHistory.size() };
	for(auto cmdIter = commandHistory.rbegin(); cmdIter != commandHistory.rend(); ++cmdIter, --i)
	{
		if(i <= numCommands)
			std::cout << SHELL_NAME << ": ! " << i << ": " << *cmdIter << std::endl;
	}
}
std::ostream& operator<<(std::ostream& os, const Command& cmd)
{
	os << cmd.name << ' ';
	PrintWordList(cmd.arguments);
	return os;
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





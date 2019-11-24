/**************************************************************************************\
** File: main.cpp
** Project: Lex - Custom Linux Shell
** Author: David Leksen
** Date:
\**************************************************************************************/
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

// Limit command numChars to make sure SeparateIntoWords works correctly:
//		std::string::size_type is converted to int, which is potentially a narrowing conversion.
//		So we must put a limit on the value, but we could make this limit as high as INT_MAX if we wanted.
//		If a different splitting algorithm was used without this conversion, this limit could be removed.
const std::string::size_type MAX_COMMAND_LENGTH{ 1024 };

// Separators
const char WHITE_SPACE_CHAR_1{ ' ' };
const char WHITE_SPACE_CHAR_2{ '\t' };
const std::string COMMAND_SEPARATOR_1{ "&&" };
const std::string COMMAND_SEPARATOR_2{ ";" };

// Commands
const std::string SHELL_NAME{ "lex" };
const std::string QUIT_COMMAND_1{ "exit" };
const std::string QUIT_COMMAND_2{ "quit" };
const std::string CHANGE_DIRECTORY_COMMAND{ "cd" };
const std::string CHANGE_TO_LAST_DIRECTORY_COMMAND{ "cdl" };

// Console text formatting
const std::string TEXT_STYLE_DEFAULT{ "\033[0m" };
const std::string TEXT_STYLE_GREEN_ON_DEFAULT{ "\033[0;32;49m" };
const std::string TEXT_STYLE_BLUE_ON_DEFAULT{ "\033[0;34;49m" };
const std::string TEXT_STYLE_GREEN_ON_DEFAULT_BOLD{ "\033[1;32;49m" };
const std::string TEXT_STYLE_BLUE_ON_DEFAULT_BOLD{ "\033[1;34;49m" };

struct Command
{
	std::string name;
	std::vector<std::string> arguments;
};

std::string lastWorkingDirectory;

//+------------------------\----------------------------------
//|		   Commands		   |
//\------------------------/----------------------------------
void ExecuteCommand(const Command& command);
void ChangeWorkingDirectory(const std::string& directory);
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
	}
}

//+------------------------\----------------------------------
//|		   Commands		   |
//\------------------------/----------------------------------
void ExecuteCommand(const Command& command)
{
	if(command.name.empty())
		return;
	else if(command.name == CHANGE_DIRECTORY_COMMAND)
	{
		if(command.arguments.size() > 1)
			std::cerr << SHELL_NAME << ": " << CHANGE_DIRECTORY_COMMAND << ": too many arguments" << std::endl;
		else if(command.arguments.empty())
			ChangeWorkingDirectory("");
		else
			ChangeWorkingDirectory(command.arguments[0]);
	}
	else if(command.name == CHANGE_TO_LAST_DIRECTORY_COMMAND)
	{
		if(!command.arguments.empty())
			std::cerr << SHELL_NAME << ": " << CHANGE_TO_LAST_DIRECTORY_COMMAND << ": too many arguments" << std::endl;
		else
			ChangeWorkingDirectory(lastWorkingDirectory);
	}
	else
		ExecuteExternalApp(command);
}
void ChangeWorkingDirectory(const std::string& directory)
{
	// If no directory specified, change to root directory
	std::string finalDirectory;
	if(directory.empty())
		finalDirectory = "/";

	// If first char is ~ expand it to home directory
	else if(directory[0] == '~')
	{
		// Try to get home directory
		char* home{ getenv("HOME") };

		// If home directory does not exist, bail
		if(!home)
		{
			std::cerr << SHELL_NAME << ": cd: ~: Failed to find home directory" << std::endl;
			return;
		}

		// Put directory string together
		finalDirectory = home;
		if(directory.size() > 1)
			finalDirectory += directory.substr(1, directory.length() - 1);
	}

	// Unmodified directory
	else
		finalDirectory = directory;

	// Change working directory (while retaining current working directory for cdl command)
	std::string savedWorkingDirectory;
	GetWorkingDirectory(savedWorkingDirectory);
	bool error;
	errno = 0;
	error = chdir(finalDirectory.c_str());
	if(error)
	{
		std::string message{ SHELL_NAME + ": cd: \'" + finalDirectory + "\'" };
		perror(message.c_str());
	}
	else
	{
		// Only overwrite last directory if we successfully changed directories.
		lastWorkingDirectory = savedWorkingDirectory;
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
	std::string workingDirectoryString;
	GetWorkingDirectory(workingDirectoryString);
	std::cout << TEXT_STYLE_GREEN_ON_DEFAULT_BOLD << SHELL_NAME
		<< TEXT_STYLE_DEFAULT << ':'
		<< TEXT_STYLE_BLUE_ON_DEFAULT_BOLD << '~' << workingDirectoryString
		<< TEXT_STYLE_DEFAULT << "$ ";
}
void SeparateIntoCommands(const std::vector<std::string>& wordList, std::vector<Command>& commandListOut)
{
	// Make sure output is empty
	commandListOut.clear();

	// Don't process empty input
	if(wordList.empty())
		return;

	// Convert length to signed integer so that (currentWord - currentCommandStart + 1) works below
	int numWords = (int)wordList.size();

	// Go word by word, finding the start and end of each command, and making a list
	for(int currentWord = 0, currentCommandStart = 0; currentWord < numWords; ++currentWord)
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
				for(int i = currentCommandStart + 1; i < currentWord; ++i)
					command.arguments.push_back(wordList[i]);

				// Record command
				commandListOut.push_back(command);
			}

			// Next command needs to start on next word
			currentCommandStart = currentWord + 1;
		}

		// If we are at the last word
		if(currentWord == numWords - 1)
		{
			// If current command has any words, record the command
			if(currentWord - currentCommandStart + 1 > 0)
			{
				// Record command name
				Command command;
				command.name = wordList[currentCommandStart];

				// Record arguments, if any
				for(int i = currentCommandStart + 1; i <= currentWord; ++i)
					command.arguments.push_back(wordList[i]);

				// Record command
				commandListOut.push_back(command);
			}
		}
	}
}
/*void SeparateIntoCommand(const std::string& input, Command& commandOut)
{
	// Make sure output is empty
	commandOut.name.clear();
	commandOut.arguments.clear();

	// Don't process empty input
	if(input.empty())
		return;

	// Verify command is not too long
	if(input.length() > MAX_COMMAND_LENGTH)
	{
		std::cerr << SHELL_NAME << ": Exceeded maximum of " << MAX_COMMAND_LENGTH << " characters." << std::endl;
		return;
	}

	// Convert length to signed integer so that (currentChar - currentWordStart + 1) works below
	int numChars = (int)input.length();

	// Go char by char, finding the start and end of each word, and making a list
	bool commandProcessed{ false };
	for(int currentChar = 0, currentWordStart = 0; currentChar < numChars; ++currentChar)
	{
		// If current character is whitespace
		if(input[currentChar] == ' ' || input[currentChar] == '\t')
		{
			// If current word has any characters, record the word
			int wordLength{ currentChar - currentWordStart };
			if(wordLength > 0)
			{
				// Separate command from args
				if(!commandProcessed)
				{
					commandOut.name = input.substr(currentWordStart, wordLength);
					commandProcessed = true;
				}
				else
					commandOut.arguments.push_back(input.substr(currentWordStart, wordLength));
			}

			// Next word needs to start on next character, any time a whitespace character is encountered, no matter if a word was recorded or not
			currentWordStart = currentChar + 1;
		}

		// If we are at the last character
		if(currentChar == numChars - 1)
		{
			// If current word has any characters, record the word
			int wordLength{ currentChar - currentWordStart + 1};
			if(wordLength > 0)
			{
				// Separate command from args
				if(!commandProcessed)
				{
					commandOut.name = input.substr(currentWordStart, wordLength);
					commandProcessed = true;
				}
				else
					commandOut.arguments.push_back(input.substr(currentWordStart, wordLength));
			}
		}
	}
}*/
void SeparateIntoWords(const std::string& input, std::vector<std::string>& outputWordList)
{
	// Make sure output is empty
	outputWordList.clear();

	// Don't process empty input
	if(input.empty())
		return;

	// Verify command is not too long
	if(input.length() > MAX_COMMAND_LENGTH)
	{
		std::cerr << SHELL_NAME << ": Exceeded maximum of " << MAX_COMMAND_LENGTH << " characters." << std::endl;
		return;
	}

	// Convert length to signed integer so that (currentChar - currentWordStart + 1) works below
	int numChars = (int)input.length();

	// Go char by char, finding the start and end of each word, and making a list
	for(int currentChar = 0, currentWordStart = 0; currentChar < numChars; ++currentChar)
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
		if(currentChar == numChars - 1)
		{
			// If current word has any characters, record the word
			if(currentChar - currentWordStart + 1 > 0)
				outputWordList.push_back(input.substr(currentWordStart, currentChar - currentWordStart + 1));
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
void GetWorkingDirectory(std::string& pathOut)
{
	char cwd[256];
	char* result;
	result = getcwd(cwd, sizeof(cwd));
	if(!result)
	{
		perror("Failed to get current working directory");
		pathOut.clear();
	}
	else
		pathOut = cwd;
}


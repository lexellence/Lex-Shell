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

// Limit command inputLength to make sure SeparateIntoWords works correctly:
//		std::string::size_type is converted to int, which is potentially a narrowing conversion.
//		So we must put a limit on the value, but we could make this limit as high as INT_MAX if we wanted.
//		If a different splitting algorithm was used without this conversion, this limit could be removed.
const std::string::size_type MAX_COMMAND_LENGTH{ 1024 };

// Console text formatting
const std::string defaultStyle{ "\033[0m" };
const std::string greenOnDefault{ "\033[0;32;49m" };
const std::string blueOnDefault{ "\033[0;34;49m" };
const std::string greenOnDefaultBold{ "\033[1;32;49m" };
const std::string blueOnDefaultBold{ "\033[1;34;49m" };
const std::string shellName{ "lex" };

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
void SeparateIntoCommand(const std::string& input, Command& commandOut);
void PrintCommand(const Command& command);
void PrintWordList(const std::vector<std::string>& wordList);
void GetWorkingDirectory(std::string& pathOutput);

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
		std::cerr << shellName << ": Fatal Exception: " << e.what() << std::endl;
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

		// Get input from command-line and format it into a command and arguments
		Command currentCommand;
		{
			std::string inputLine;
			getline(std::cin, inputLine);

			//************************************************  
			//TODO: separate into list of commands separated by "&&" 
			//************************************************
			SeparateIntoCommand(inputLine, currentCommand);
		}
		if(currentCommand.name.empty())
			continue;

		ExecuteCommand(currentCommand);
	}
}

//+------------------------\----------------------------------
//|		   Commands		   |
//\------------------------/----------------------------------
void ExecuteCommand(const Command& command)
{
	if(command.name == "exit")
		return;
	else if(command.name == "cd")
	{
		if(command.arguments.size() > 1)
			std::cerr << shellName << ": cd: too many arguments" << std::endl;
		else if(command.arguments.empty())
			ChangeWorkingDirectory("");
		else
			ChangeWorkingDirectory(command.arguments[0]);
	}
	else if(command.name == "cdl")
	{
		if(!command.arguments.empty())
			std::cerr << shellName << ": cdl: too many arguments" << std::endl;
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
			std::cerr << shellName << ": cd: ~: Failed to find home directory" << std::endl;
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
		std::string message{ shellName + ": cd: \'" + finalDirectory + "\'" };
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
		std::string message{ shellName + ": " + command.name };
		perror(message.c_str());
	}

	if(child_pid == 0)
	{
		// This is the child process

		// Convert command to c-style string list 
		std::vector<char*> cStyleStringList;

		// argv[0] is always the app name		
		//************************************************  
		//TODO: separate path from app name (detecting last '/')  and only put name in argv[0] while sending the full path to execvp 
		//************************************************
		cStyleStringList.emplace_back(const_cast<char*>(command.name.c_str()));

		// Add arguments
		for(auto const& arg : command.arguments)
			cStyleStringList.emplace_back(const_cast<char*>(arg.c_str()));

		// exec expects null-terminated array
		cStyleStringList.push_back(nullptr);

		// execute program with arguments
		errno = 0;
		int result = execvp(cStyleStringList[0], cStyleStringList.data());

		// If execvp failed, print error and terminate child process
		if(result < 0)
		{
			std::string message{ shellName + ": " + command.name };
			perror(message.c_str());
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// This is the parent process, so wait for child to finish
		errno = 0;
		if(waitpid(child_pid, nullptr, WUNTRACED) < 0)
		{
			// Print error, if any
			std::string message{ shellName + ": " + command.name };
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
	std::cout << greenOnDefaultBold << shellName
		<< defaultStyle << ':'
		<< blueOnDefaultBold << '~' << workingDirectoryString
		<< defaultStyle << "$ ";
}
void SeparateIntoCommand(const std::string& input, Command& commandOut)
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
		std::cerr << shellName << ": Exceeded maximum of " << MAX_COMMAND_LENGTH << " characters." << std::endl;
		return;
	}

	// Convert length to signed integer so that (currentChar - currentWordStart + 1) works below
	int inputLength = (int)input.length();

	// Go char by char, finding the start and end of each word, and making a list
	bool commandProcessed{ false };
	for(int currentChar = 0, currentWordStart = 0; currentChar < inputLength; ++currentChar)
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
		if(currentChar == inputLength - 1)
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
void GetWorkingDirectory(std::string& pathOutput)
{
	char cwd[256];
	char* result;
	result = getcwd(cwd, sizeof(cwd));
	if(!result)
	{
		perror("Failed to get current working directory");
		pathOutput.clear();
	}
	else
		pathOutput = cwd;
}


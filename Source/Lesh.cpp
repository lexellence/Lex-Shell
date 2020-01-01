/**************************************************************************************\
** File: Lesh.cpp
** Project: lesh - Lexellence Linux Shell
** Author: David Leksen - Lexellence Games
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
#include "LexUtility.h"
#include "LexConsole.h"

namespace Lesh
{
	// Operators/Separators
	const std::vector<char> WHITESPACE_CHARS{ ' ', '\t' };
	const std::string COMMAND_SEPARATOR_1{ "&&" };
	const std::string COMMAND_SEPARATOR_2{ ";" };
	const std::string REDIRECT_OUTPUT_OPERATOR{ ">" };
	const std::string REDIRECT_OUTPUT_APPEND_OPERATOR{ ">>" };
	const std::string REDIRECT_INPUT_OPERATOR{ "<" };
	const std::string PIPING_OPERATOR{ "<" };

	// Commands
	const std::string SHELL_NAME{ "lesh" };
	const std::string QUIT_COMMAND_1{ "exit" };
	const std::string QUIT_COMMAND_2{ "quit" };
	const std::string CHANGE_DIRECTORY_COMMAND{ "cd" };
	const std::string CHANGE_TO_LAST_DIRECTORY_COMMAND{ "cdl" };
	const std::string DISPLAY_HISTORY_COMMAND{ "history" };
	const std::string EXECUTE_HISTORY_COMMAND{ "!" };

	// Prompt styles
	const std::string& SHELL_STYLE{ Lex::ConsoleFormatting::BLUE_ON_DEFAULT_BOLD };
	const std::string& USER_STYLE{ Lex::ConsoleFormatting::GREEN_ON_DEFAULT_BOLD };
	const std::string& DIRECTORY_STYLE{ Lex::ConsoleFormatting::BLUE_ON_DEFAULT_BOLD };
	const std::string& PUNCUATION_STYLE{ Lex::ConsoleFormatting::DEFAULT };

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
		void Print(std::ostream& os) const
		{
			os << name << ' ';
			Lex::WordLists::Print(os, arguments);
		}
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
	//|		   Execute		   |
	//\------------------------/----------------------------------
	void ExecuteCommand(const Command& command);
	bool StringToCommandListIndex(const std::string& stringIn, CommandListIndex& indexOut);
	void ExecuteExternalApp(const Command& command);
	
	//+------------------------\----------------------------------
	//|		    Parse		   |
	//\------------------------/----------------------------------
	void SeparateIntoCommands(const std::vector<std::string>& wordList, std::vector<Command>& commandListOut);

	//+------------------------\----------------------------------
	//|		   Directory	   |
	//\------------------------/----------------------------------
	void ChangeDirectory(const std::string& path);
	void ExpandDirectory(std::string& path);

	//+------------------------\----------------------------------
	//|		    Print		   |
	//\------------------------/----------------------------------
	void PrintPrompt();
	void PrintHistory(CommandListIndex numCommands);

	//+------------------------\----------------------------------
	//|			 Main		   |
	//\------------------------/----------------------------------
	int RunLesh()
	{
		try
		{
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
						Lex::WordLists::Separate(inputLine, WHITESPACE_CHARS, wordList);
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
					Lex::Lists::AddUniqueElementToFront(*sourceIter, commandHistory);
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
				if(StringToCommandListIndex(command.arguments[0], input) && input > 0)
					commandToExecutePtr = Lex::Lists::GetElementPtr(commandHistory, input - 1);
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
		Lex::Lists::AddUniqueElementToFront(*commandToExecutePtr, executedCommands);

		// Execute command
		if(commandToExecutePtr->name == CHANGE_DIRECTORY_COMMAND)
		{

			if(commandToExecutePtr->arguments.size() > 1)
				std::cerr << SHELL_NAME << ": " << CHANGE_DIRECTORY_COMMAND << ": too many parameters" << std::endl;
			else
			{
				std::string newPath;
				if(!commandToExecutePtr->arguments.empty())
					newPath = commandToExecutePtr->arguments[0];
				ExpandDirectory(newPath);
				ChangeDirectory(newPath);
			}
		}
		else if(commandToExecutePtr->name == CHANGE_TO_LAST_DIRECTORY_COMMAND)
		{
			if(!commandToExecutePtr->arguments.empty())
				std::cerr << SHELL_NAME << ": " << CHANGE_TO_LAST_DIRECTORY_COMMAND << ": too many parameters" << std::endl;
			else
				ChangeDirectory(lastWorkingDirectory);
		}
		else
			ExecuteExternalApp(*commandToExecutePtr);
	}
	bool StringToCommandListIndex(const std::string& stringIn, CommandListIndex& indexOut)
	{
		try {
			indexOut = (CommandListIndex)std::stoul(stringIn);
		}
		catch(const std::logic_error & e) {
			return false;
		}
		return true;
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
	//|		    Parse		   |
	//\------------------------/----------------------------------
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
	//|		   Directory	   |
	//\------------------------/----------------------------------
	void ChangeDirectory(const std::string& path)
	{
		// Save current
		if(!Lex::Posix::GetWorkingDirectory(lastWorkingDirectory))
			std::cerr << SHELL_NAME << ": Failed to save current working directory" << std::endl;

		// Change to new
		if(!Lex::Posix::ChangeWorkingDirectory(path))
			std::cerr << SHELL_NAME << ": Failed to change current working directory to \'" << path << '\'' << std::endl;
	}
	void ExpandDirectory(std::string& path)
	{
		// If no directory specified, change to root directory
		if(path.empty())
			path = "/";

		// Expand ~ to home directory
		if(path[0] == '~')
		{
			std::string home;
			if(!Lex::Posix::GetHomeDirectory(home))
			{
				std::cerr << SHELL_NAME << ": ~: Failed to find home directory" << std::endl;
				return;
			}
			path.replace(0, 1, home);
		}
	}

	//+------------------------\----------------------------------
	//|		    Print		   |
	//\------------------------/----------------------------------
	void PrintPrompt()
	{
		std::cout << SHELL_STYLE << SHELL_NAME;
		{
			std::string user;
			if(Lex::Posix::GetUser(user))
				std::cout << PUNCUATION_STYLE << '(' << USER_STYLE << user << PUNCUATION_STYLE << ')';
		}
		std::cout << PUNCUATION_STYLE << ':';
		{
			std::string workingDirectory;
			if(Lex::Posix::GetWorkingDirectory(workingDirectory))
				std::cout << DIRECTORY_STYLE << workingDirectory;
			std::cout << PUNCUATION_STYLE << "$ " << Lex::ConsoleFormatting::DEFAULT;
		}
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
			{
				std::cout << SHELL_NAME << ": ! " << i << ": ";
				cmdIter->Print(std::cout);
				std::cout << std::endl;
			}
		}
	}
}

int main()
{
	return Lesh::RunLesh();
}




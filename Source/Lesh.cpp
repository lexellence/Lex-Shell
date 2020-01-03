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
#include <limits>
#include <memory>
#include <unistd.h>
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

	//+------------------------\----------------------------------
	//|		    Parse		   |
	//\------------------------/----------------------------------
	void SeparateIntoCommands(const std::vector<std::string>& wordList, std::vector<Command>& commandListOut);
	bool StringToCommandListIndex(const std::string& str, CommandListIndex& out);

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
		if(commandToExecutePtr->name == EXECUTE_HISTORY_COMMAND)
		{
			if(commandToExecutePtr->arguments.size() > 1)
			{
				std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": too many parameters" << std::endl;
				return;
			}
			else if(commandToExecutePtr->arguments.empty())
			{
				std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND << ": missing parameter" << std::endl;
				return;
			}
			else
			{
				// Convert parameter string to CommandListIndex
				CommandListIndex input{ 0 };
				bool succeeded = (StringToCommandListIndex(commandToExecutePtr->arguments[0], input) && input > 0);

				// Get list element
				if(succeeded)
				{
					commandToExecutePtr = Lex::Lists::GetElementPtr(commandHistory, input - 1);
					if(!commandToExecutePtr)
						succeeded = false;
				}
				if(!succeeded)
				{
					std::cerr << SHELL_NAME << ": " << EXECUTE_HISTORY_COMMAND 
							  << ": invalid parameter (min=" << (commandHistory.empty() ? '0' : '1') 
							  << " max=" << commandHistory.size() << ')' << std::endl;
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
			Lex::Posix::ExecuteExternalAppAndWait(commandToExecutePtr->name, commandToExecutePtr->arguments,
										   SHELL_NAME + ": " + commandToExecutePtr->name);
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
	bool StringToCommandListIndex(const std::string& str, CommandListIndex& out)
	{
		static_assert(sizeof(CommandListIndex) >= sizeof(int));
		int numCommandsInt;

		if(Lex::Strings::ToInt(str, numCommandsInt) && numCommandsInt >= 0)
		{
			out = static_cast<CommandListIndex>(numCommandsInt);
			return true;
		}
		else
			return false;
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




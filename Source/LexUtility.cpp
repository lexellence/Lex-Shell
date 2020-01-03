/**************************************************************************************\
** File: LexUtility.cpp
** Project:
** Author: David Leksen - Lexellence Games
** Date:
**
** Source code file for reusable code
**
\**************************************************************************************/
#include "LexUtility.h"
#include <ostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>

namespace Lex
{
	namespace WordLists
	{
		void Separate(const std::string& input, const std::vector<char>& whitespaceChars, std::vector<std::string>& wordsOut)
		{
			// Go char by char, finding the start and end of each word, and making a list
			wordsOut.clear();
			for(std::string::size_type currentChar = 0, currentWordStart = 0; currentChar < input.size(); ++currentChar)
			{
				// If current character is whitespace
				if(std::find(whitespaceChars.begin(), whitespaceChars.end(), input[currentChar]) != whitespaceChars.end())
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
		void Print(std::ostream& os, const std::vector<std::string>& wordList)
		{
			for(std::vector<std::string>::size_type i = 0; i < wordList.size(); ++i)
			{
				os << wordList[i];
				if(i < wordList.size() - 1)
					os << ' ';
			}
		}
	}

	namespace Posix
	{
		bool GetUser(std::string& userOut)
		{
			char* user{ getenv("USER") };
			if(!user)
				return false;
			userOut = user;
			return true;
		}
		bool GetHomeDirectory(std::string& homeOut)
		{
			char* home{ getenv("HOME") };
			if(!home)
				return false;
			homeOut = home;
			return true;
		}
		bool GetWorkingDirectory(std::string& pathOut)
		{
			char cwd[512];
			char* result;
			result = getcwd(cwd, sizeof(cwd));
			if(!result)
				return false;
			pathOut = cwd;
			return true;
		}
		bool ChangeWorkingDirectory(const std::string& path)
		{
			return chdir(path.c_str());
		}
		/*void ExecuteExternalApp(const Command& command)
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
		}*/
		bool ExecuteExternalAppAndWait(const std::string& pathToApp, 
								const std::vector<std::string>& arguments, 
								const std::string& perrorMessage)
		{
			if(pathToApp.empty())
				return false;

			errno = 0;
			pid_t child_pid;
			child_pid = fork();

			// Fork failed
			if(child_pid < 0)
			{
				perror(perrorMessage.c_str());
				return false;
			}

			// Child of fork
			if(child_pid == 0)
			{
				// Convert command to c-style string list 
				std::vector<char*> cStyleStringList;
				{
					// Separate path from app name and only put name in argv[0] while sending the full path to execvp 
					{
						std::string appName;
						{
							std::size_t endOfPathIndex{ pathToApp.find_last_of('/') };
							if(endOfPathIndex == std::string::npos)
								appName = pathToApp;
							else
								appName = pathToApp.substr(endOfPathIndex + 1);
						}
						cStyleStringList.emplace_back(const_cast<char*>(appName.c_str()));
					}

					// Add arguments
					for(auto const& arg : arguments)
						cStyleStringList.emplace_back(const_cast<char*>(arg.c_str()));

					// exec expects null-terminated array
					cStyleStringList.push_back(nullptr);
				}

				// execute program
				errno = 0;
				int result = execvp(pathToApp.c_str(), cStyleStringList.data());

				// If execvp failed, print error and terminate child process
				if(result < 0)
				{
					perror(perrorMessage.c_str());
					exit(EXIT_FAILURE);
				}
			}
			// Parent of fork
			else
			{
				// Wait for child to finish
				errno = 0;
				if(waitpid(child_pid, nullptr, WUNTRACED) < 0)
				{
					perror(perrorMessage.c_str());
					return false;
				}
			}
			return true;
		}
	}

	namespace Strings
	{
		bool ToInt(const std::string& str, int& out)
		{
			try {
				std::string::size_type nextCharPos;
				out = std::stoi(str, &nextCharPos);
				if(nextCharPos == str.size())
					return true;
				else
					return false;
			}
			catch(const std::logic_error & e) {
				return false;
			}
		}
	}
}
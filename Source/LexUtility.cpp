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
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <ostream>

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
	}
}
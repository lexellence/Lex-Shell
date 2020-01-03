/**************************************************************************************\
** File: LexUtility.h
** Project: lesh - Lexellence Linux Shell
** Author: David Leksen
** Date:
**
** Header file for reusable code
**
\**************************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <ostream>
#include <list>
namespace Lex
{
	namespace WordLists
	{
		void Separate(const std::string& input, const std::vector<char>& whitespaceChars, std::vector<std::string>& wordsOut);
		void Print(std::ostream& os, const std::vector<std::string>& wordList);
	}

	namespace Posix
	{
		bool GetUser(std::string& userOut);
		bool GetHomeDirectory(std::string& homeOut);
		bool GetWorkingDirectory(std::string& pathOut);
		bool ChangeWorkingDirectory(const std::string& path);
		bool ExecuteExternalAppAndWait(const std::string& pathToApp,
								const std::vector<std::string>& arguments, 
								const std::string& perrorMessage);
	}

	namespace Lists
	{
		template <class ElementType, class IndexType>
		const ElementType* GetElementPtr(const std::list<ElementType>& sourceList, IndexType index)
		{
			if(sourceList.empty() || index < 0 || index >= sourceList.size())
				return nullptr;
			else if(index == sourceList.size() - 1)
				return &sourceList.back();
			else if(index == 0)
				return &sourceList.front();
			else
			{
				auto it{ sourceList.begin() };
				std::advance(it, index);
				return &*it;
			}
		}

		template <class ElementType>
		void AddUniqueElementToFront(const ElementType& sourceElement, std::list<ElementType>& destinationList)
		{
			auto foundExistingElementIter{ std::find(destinationList.begin(), destinationList.end(), sourceElement) };
			if(foundExistingElementIter != destinationList.end())
			{
				if(foundExistingElementIter != destinationList.begin())
					destinationList.splice(destinationList.begin(), destinationList, foundExistingElementIter);
			}
			else
				destinationList.push_front(sourceElement);
		}
	}

	namespace Strings
	{
		bool ToInt(const std::string& str, int& out);
	}
}

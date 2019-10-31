#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

// Limit command inputLength to make sure SeparateIntoWords works correctly:
//		std::string::size_type is converted to int
//		If a different splitting algorithm was used, this could be removed.
const std::string::size_type MAX_COMMAND_LENGTH{ 1024 };

void MainLoop();
void SeparateIntoWords(const std::string& inputString, std::vector<std::string>& outputWordList);
void PrintWordList(const std::vector<std::string>& wordList);
void ExecuteExternalApp(const std::vector<std::string>& wordList);

int main()
{
	try
	{
		MainLoop();
	}
	catch(const std::exception & e)
	{
		std::cerr << "Fatal Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void MainLoop()
{
	while(true)
	{
		// Display prompt
		std::cout << "lex$ ";

		// Get list of words from command-line
		std::string inputLineString;
		getline(std::cin, inputLineString);
		if(inputLineString.length() > MAX_COMMAND_LENGTH)
		{
			std::cerr << "Exceeded max " << MAX_COMMAND_LENGTH << " characters." << std::endl;
			continue;
		}
		std::vector<std::string> wordList;
		SeparateIntoWords(inputLineString, wordList);
		if(wordList.empty())
			continue;

		// Execute built-in command if one exists, otherwise try executing external application
		if(wordList[0] == "exit")
			return;
		else
			ExecuteExternalApp(wordList);
	}
}
void SeparateIntoWords(const std::string& inputString, std::vector<std::string>& outputWordList)
{
	// Make sure output is empty
	outputWordList.clear();

	// Don't process empty input
	if(inputString.empty())
		return;

	// Convert length to signed integer so that (currentChar - currentWordStart + 1) works below
	int inputLength;
	if(inputString.length() > MAX_COMMAND_LENGTH)
		inputLength = (int)MAX_COMMAND_LENGTH;
	else
		inputLength = (int)inputString.length();

	// Go char by char, finding the start and end of each word, and making a list
	for(int currentChar = 0, currentWordStart = 0; currentChar < inputLength; ++currentChar)
	{
		// If current character is whitespace
		if(inputString[currentChar] == ' ' || inputString[currentChar] == '\t')
		{
			// If current word has any characters, record the word
			if(currentChar > currentWordStart)
				outputWordList.push_back(inputString.substr(currentWordStart, currentChar - currentWordStart));

			// Next word needs to start on next character, any time a whitespace character is encountered, no matter if a word was recorded or not
			currentWordStart = currentChar + 1;
		}

		// If we are at the last character
		if(currentChar == inputLength - 1)
		{
			// If current word has any characters, record the word
			if(currentChar - currentWordStart + 1 > 0)
				outputWordList.push_back(inputString.substr(currentWordStart, currentChar - currentWordStart + 1));
		}
	}
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
void ExecuteExternalApp(const std::vector<std::string>& wordList)
{
	if(wordList.empty())
		return;

	pid_t child_pid;
	child_pid = fork();

	// Bail if fork failed
	if(child_pid < 0)
	{
		perror("Could not create child process");
		exit(EXIT_FAILURE);
	}

	// If this is the child process, execute external app
	if(child_pid == 0)
	{
		// Convert std::string list to c-style string list 
		std::vector <char const*> cStyleStringList;
		for(auto word : wordList)
			cStyleStringList.push_back(word.c_str());

		// exec expects null-terminated array
		cStyleStringList.push_back(nullptr);

		// execute program with arguments
		int result;
		result = execvp(cStyleStringList[0], const_cast <char* const*> (cStyleStringList.data()));

		// Bail if execvp failed
		if(result < 0)
		{
			perror(cStyleStringList[0]);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// This is the parent process, so wait for child to finish, or bail if error
		if(waitpid(child_pid, nullptr, WUNTRACED) < 0)
		{
			perror("Could not wait for child process");
			exit(EXIT_FAILURE);
		}
	}
}

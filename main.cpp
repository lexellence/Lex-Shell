#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

// Limit command inputLength to make sure SeparateIntoWords works correctly:
//		std::string::size_type is converted to int
//		If a different splitting algorithm was used, this could be removed.
const std::string::size_type MAX_COMMAND_LENGTH{ 1024 };

// Console text color formatting codes
const std::string defaultStyle("\033[0m");
const std::string greenOnDefault("\033[0;32;49m");
const std::string blueOnDefault("\033[0;34;49m");
const std::string greenOnDefaultBold("\033[1;32;49m");
const std::string blueOnDefaultBold("\033[1;34;49m");

void MainLoop();
void SeparateIntoWords(const std::string& inputString, std::vector<std::string>& outputWordList);
void PrintWordList(const std::vector<std::string>& wordList);
void ExecuteExternalApp(const std::vector<std::string>& pathAndArgs);
void GetWorkingDirectory(std::string& pathOutput);

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
	// Clear terminal
	ExecuteExternalApp({ "clear" });

	while(true)
	{
		// Display shell prompt including current directory
		std::string workingDirectoryString;
		GetWorkingDirectory(workingDirectoryString);
		std::cout << greenOnDefaultBold << "lex"
			<< defaultStyle << ':'
			<< blueOnDefaultBold << '~' << workingDirectoryString
			<< defaultStyle << "$ ";

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

		//SeparateIntoCommands(wordList, wordListVector);

		// Execute built-in command if one exists, otherwise try executing external application
		if(wordList[0] == "exit")
			return;
		else if(wordList[0] == "cd")
		{
			if(wordList.size() != 2)
			{
				std::cerr << "Invalid number of arguments. Usage: $ cd <dir>" << std::endl;
				continue;
			}
			else
			{
				// If we need to expand tilde to home directory
				if(wordList[1][0] == '~')
				{
					// Try to get home directory
					std::string directoryString;
					char* homeDirectory = getenv("HOME");

					// If home directory exists, start the directory path with it
					if(homeDirectory)
						directoryString += homeDirectory;

					// Add the rest of the directory path
					directoryString += wordList[1].substr(1, wordList[1].length() - 1);
				}
				else
				{
					// Attempt to change working directory without any tilde expansion
					chdir(wordList[1].c_str());
				}
			}
		}
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
void ExecuteExternalApp(const std::vector<std::string>& pathAndArgs)
{
	if(pathAndArgs.empty())
		return;

	pid_t child_pid;
	child_pid = fork();

	// If fork failed, print error
	if(child_pid < 0)
	{
		std::string message{ "Error creating child process to execute " + pathAndArgs[0] };
		perror(message.c_str());
	}

	// If this is the child process, execute external app
	if(child_pid == 0)
	{
		// Convert std::string list to c-style string list 
		std::vector<char*> cStyleStringList;
		for(auto const& word : pathAndArgs)
			cStyleStringList.emplace_back(const_cast<char*>(word.c_str()));

		// exec expects null-terminated array
		cStyleStringList.push_back(nullptr);

		// execute program with arguments
		int result = execvp(cStyleStringList[0], cStyleStringList.data());

		// If execvp failed, print error and terminate child process
		if(result < 0)
		{
			std::string message{ "Error executing " + pathAndArgs[0] };
			perror(message.c_str());
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// This is the parent process, so wait for child to finish
		if(waitpid(child_pid, nullptr, WUNTRACED) < 0)
		{
			// Print error, if any
			std::string message{ "Error waiting for and reaping child process " + pathAndArgs[0] };
			perror(message.c_str());
		}
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
/**************************************************************************************\
** File: LexConsole.h
** Project: lesh - Lexellence Linux Shell
** Author: David Leksen
** Date:
**
** Header file for console text formatting codes
**
\**************************************************************************************/
#pragma once
#include <string>
namespace Lex::ConsoleFormatting
{
	const std::string DEFAULT{ "\033[0m" };
	const std::string GREEN_ON_DEFAULT{ "\033[0;32;49m" };
	const std::string BLUE_ON_DEFAULT{ "\033[0;34;49m" };
	const std::string RED_ON_DEFAULT{ "\033[0;31;49m" };
	const std::string YELLOW_ON_DEFAULT{ "\033[0;33;49m" };
	const std::string GREEN_ON_DEFAULT_BOLD{ "\033[1;32;49m" };
	const std::string BLUE_ON_DEFAULT_BOLD{ "\033[1;34;49m" };
	const std::string RED_ON_DEFAULT_BOLD{ "\033[1;31;49m" };
	const std::string YELLOW_ON_DEFAULT_BOLD{ "\033[1;33;49m" };
	const std::string CYAN_ON_DEFAULT_BOLD{ "\033[1;36;49m" };
}

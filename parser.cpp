
/*
 * =====================================================================================
 *
 *       Filename:  parser.cpp
 *
 *    Description:  Source for Parser object.
 *
 *        Version:  1.0
 *        Created:  26/03/16 11:49:28
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Michael Tierney (MT), tiernemi@tcd.ie
 *
 * =====================================================================================
 */

#include "parser.hpp"
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include <regex>

/* 
 * ===  MEMBER FUNCTION CLASS : Parser  ===============================================
 *         Name:  parser
 *    Arguments:  std::string cmd - The input to be parsed.
 *      Returns:  A vector containing the individual groups to be launched, each group
 *                contains the individual commands that make up the group, each command 
 *                contains a vector of string arguments.
 *  Description:  Parses the simple command string based on groups ";" and then IO
 *                redirection "| > < >>" + background "&" and then finally by command
 *                arguments.
 * =====================================================================================
 */

std::vector<std::vector<std::vector<std::string>>> Parser::parse(const std::string & cmd) {
	std::vector<std::vector<std::string>> allCommands ;
	std::stringstream totalCommand(cmd) ;
	// Split based on reserved characters. //
	while (!totalCommand.eof()) {
		std::vector<std::string> singleCommand ;
		std::string component ;

		do {
			if(totalCommand >> component) {
				singleCommand.push_back(component) ;
			} else {
				singleCommand.push_back("") ;
			}
		} while (!isReserved(component) && !totalCommand.eof()) ;

		if (isReserved(component)) {
			singleCommand.pop_back() ;
			std::vector<std::string> resCommand ;
			resCommand.push_back(component) ;
			allCommands.push_back(singleCommand) ;	
			allCommands.push_back(resCommand) ;
		} else {
			allCommands.push_back(singleCommand) ;	
		}
	}

	// Need to permute IO redirects for stack unwinding. //
	for (unsigned int i = 0 ; i < allCommands.size() ; ++i) {
		if (isReservedIO(allCommands[i][0])) {
			std::iter_swap(allCommands.begin()+i, allCommands.begin()+i+1) ;
			++i ;
		}
	}

	// Handles stream redirection (Any stream number) EG 2> or 1>. //
	for (unsigned int i = 0 ; i < allCommands.size() ; ++i) {
		if (allCommands[i][0].find(">") != std::string::npos) {	
			std::regex rgx("^[[:digit:]]*>(>)?$");
			if (regex_match(allCommands[i][0], rgx)) {
				std::regex extractNum("(\\w+)>(>)?") ;
				std::regex extractOverOp("[[:digit:]]*>$") ;
				std::smatch matchNum ;
				// Deafult is std out. //
				std::string destStr = std::to_string(STDOUT_FILENO) ;
				const std::string searchPattern = allCommands[i][0] ;
				if(std::regex_search(searchPattern.begin(), searchPattern.end(), matchNum, extractNum)) {
					destStr = matchNum[1] ;
				}
				allCommands[i].push_back(destStr) ;
				if (regex_match(allCommands[i][0], extractOverOp)) {
					allCommands[i][0] = ">" ;
				} else {
					allCommands[i][0] = ">>" ;
				}
			}
		}
	}

	// Clean junk in commands. //
	for (unsigned int i = 0 ; i < allCommands.size() ; ++i) {
		for (unsigned int j = 0 ; j < allCommands[i].size() ; ++j) {
			allCommands[i][j].erase(std::remove(allCommands[i][j].begin(), 
						allCommands[i][j].end(), '"'), allCommands[i][j].end());
			allCommands[i][j].erase(std::remove(allCommands[i][j].begin(), 
						allCommands[i][j].end(), '\n'), allCommands[i][j].end());
			allCommands[i][j].erase(std::remove(allCommands[i][j].begin(), 
						allCommands[i][j].end(), '\t'), allCommands[i][j].end());
			allCommands[i][j].erase(std::remove(allCommands[i][j].begin(), 
						allCommands[i][j].end(), ' '), allCommands[i][j].end());
		}
	}
	// Handle "No arguments" //
	for (unsigned int i = 0 ; i < allCommands.size() ; ++i) {
		for (unsigned int j = 1 ; j < allCommands[i].size() ; ++j) {
			if (allCommands[i][j].compare("") == 0) {
				allCommands[i].erase(allCommands[i].begin()+j) ;
				--j ;
			}
		}
		if (allCommands[i].size() == 0) {
			allCommands.erase(allCommands.begin()+i) ;
			--i ;
		}
	}

	// Finally seperate commands based on ;. //
	std::vector<std::vector<std::vector<std::string>>> fullyParsedCommands ;

	std::vector<std::vector<std::string>> seperateGroup ;
	for (unsigned int i = 0 ; i < allCommands.size() ; ++i) {
		if (allCommands[i][0].compare(";") != 0) {
			seperateGroup.push_back(allCommands[i]) ;
		} else {
			fullyParsedCommands.push_back(seperateGroup) ;
			seperateGroup.clear() ;
		}
	}
	if (allCommands[allCommands.size()-1][0].compare(";") != 0) {
		fullyParsedCommands.push_back(seperateGroup) ;
	}

	return fullyParsedCommands ;
}		/* -----  end of member function parser  ----- */


/* 
 * ===  MEMBER FUNCTION CLASS : Parser  ===============================================
 *         Name:  convertCmdsToString
 *    Arguments:  std::vector<std::vector<std::string>> Vector of commands in group.
 *      Returns:  A string consisting of all commands.
 *  Description:  Converts parsed group of commands back into a string.
 * =====================================================================================
 */

std::string Parser::convertCmdsToString(const std::vector<std::vector<std::string>> & cmds) {
	std::string cmdString ;
	std::vector<std::vector<std::string>> cmdsCpy = cmds ;	
	// Need to unpermute IO redirects for stack unwinding to get true command. //
	for (unsigned int i = 0 ; i < cmdsCpy.size() ; ++i) {
		if (isReservedIO(cmdsCpy[i][0])) {
			std::iter_swap(cmdsCpy.begin()+i, cmdsCpy.begin()+i-1) ;
			++i ;
		}
	}

	// Convert vector to string. //
	for (unsigned int i = 0 ; i < cmdsCpy.size() ; ++i) {
		for (unsigned int j = 0 ; j < cmdsCpy[i].size() ; ++j) {
			cmdString += cmdsCpy[i][j] ;
			cmdString += " " ;
		}
		cmdString += " " ;
	}
	return  cmdString ;
}		/* -----  end of member function convertArgsToString  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Parser  ===============================================
 *         Name:  isReserved
 *    Arguments:  const std::string & str
 *      Returns:  True if string is on the reserved list. False otherwise.
 *  Description:  Checks if string is on the list of reserved characters.
 * =====================================================================================
 */

bool Parser::isReserved(const std::string & str) {
	static const std::vector<std::string> reserved = {"|", "<", "<<", ">", ">>", "&", ";"} ;
	for (unsigned int i = 0 ; i < reserved.size() ; ++i) {
		if (str.find(reserved[i]) != std::string::npos) {
			return true ;
		}
	}
	return false ;
}		/* -----  end of member function isReserved  ----- */

/* 
 * ===  MEMBER FUNCTION CLASS : Parser  ===============================================
 *         Name:  isReservedIO
 *    Arguments:  const std::string & str
 *      Returns:  True if string is on the reserved list of IO redirects. False otherwise.
 * =====================================================================================
 */

bool Parser::isReservedIO(const std::string & str) {
	static const std::vector<std::string> reserved = {"|", "<", "<<", ">", ">>"} ;
	for (unsigned int i = 0 ; i < reserved.size() ; ++i) {
		if (str.find(reserved[i]) != std::string::npos) {
			return true ;
		}
	}
	return false ;
}		/* -----  end of member function isReserved  ----- */



#ifndef PARSER_HPP_AMWVQYAN
#define PARSER_HPP_AMWVQYAN

/*
 * =====================================================================================
 *
 *       Filename:  parser.hpp
 *
 *    Description:  Parser for parsing commands.
 *
 *        Version:  1.0
 *        Created:  26/03/16 12:00:43
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Michael Tierney (MT), tiernemi@tcd.ie
 *
 * =====================================================================================
 */

#include <string>
#include <vector>

/* 
 * ===  CLASS  =========================================================================
 *         Name:  Parser
 *  Description:  Helper class for parsing command strings.
 * =====================================================================================
 */

class Parser {
 public:
	static std::vector<std::vector<std::vector<std::string>>> parse(const std::string &) ;
	static std::string convertCmdsToString(const std::vector<std::vector<std::string>> &) ;
 private:
	static bool isReserved(const std::string &) ;
	static bool isReservedIO(const std::string &) ;	
} ;		/* -----  end of class Parser  ----- */

#endif /* end of include guard: PARSER_HPP_AMWVQYAN */

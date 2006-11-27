/*
	$Id$
	
	Config parser for an mrs web service, basically a simplified Perl parser...
*/

#include <fstream>
#include <map>

#include "WConfig.h"

using namespace std;

config_exception::config_exception(const string& inError, int inLine)
{
	snprintf(msg, sizeof msg, "Error in reading config file at line %d: %s", inLine, inError.c_str());
}

class CfParser
{
  public:
					CfParser(const string& inPath);
	
	void			Parse(string& outDataDir, string& outFormatDir, vector<DbInfo>& outDbInfo);

  private:
	
	void			ParseStatement();
	void			ParseAssignment(const string& inVariable);
	void			ParseList();
	void			ParseHash();

	enum CfToken
	{
		cfEOF			= 0,
		cfUndefined		= 256,
		cfVariable,
		cfIdentifier,
		cfNumber,
		cfString,
		cfLeft,
	};
	
	char			GetCharacter();
	
	void			Retract();
	int				Restart(int inStart);

	int				GetToken();
	
	void			Match(int inToken);

	string			fPath;
	ifstream		fFile;
	int				fLine;
	streamoff		fPointer;
	string			fToken;
	int				fLookahead;
	map<int,string>	fTokenNames;

	vector<DbInfo>	fDbInfo;
	string			fDataDir;
	string			fFormatDir;
};

CfParser::CfParser(const string& inPath)
	: fFile(inPath.c_str())
	, fLine(1)
	, fPointer(0)
{
	fLookahead = GetToken();
	
	fTokenNames[cfEOF] =		"end of file";
	fTokenNames[cfUndefined] =	"undefined";
	fTokenNames[cfVariable] =	"variable";
	fTokenNames[cfIdentifier] =	"identifier";
	fTokenNames[cfNumber] =		"number";
	fTokenNames[cfString] =		"string";
	fTokenNames[cfLeft] =		"comma";
}

char CfParser::GetCharacter()
{
	char result = cfEOF;
	if (not fFile.eof())
	{
		result = fFile.get();
		fToken += result;
	}

	return result;
}

void CfParser::Retract()
{
	fFile.unget();
	if (fToken.length() > 0)
		fToken.erase(fToken.length() - 1, 1);
}

int CfParser::Restart(int inStart)
{
	int state = 0;
	
	switch (inStart)
	{
		case 1:		state = 10;	break;
		case 10:	state = 20; break;
		case 20:	state = 30; break;
		case 30:	state = 40; break;
		case 40:	state = 50; break;
		case 50:	state = 60; break;
	}
	
	fToken.clear();
	fFile.seekg(fPointer);
	
	return state;
}

int CfParser::GetToken()
{
	int state = 1, start = 1;
	int token = cfUndefined;
	
	fFile.seekg(fPointer);
	
	while (state != 0 and token == cfUndefined)
	{
		char ch = GetCharacter();
		
		switch (state)
		{
			case 1:
			{
				if (ch == ' ' or ch == '\t' or ch == '\n')
				{
					fToken.clear();
					fPointer = fFile.tellg();

					if (ch == '\n')
						++fLine;
				}
				else if (ch == '#')
					state = 2;
				else if (ch == 0)
					token = cfEOF;
				else
					state = start = Restart(start);
				break;
			}
			
			case 2:
			{
				if (ch == '\n')
				{
					++fLine;
					fToken.clear();
					fPointer = fFile.tellg();
					state = 1;
				}
				else if (ch == 0)
					token = cfEOF;
				break;
			}
			
			case 10:
			{
				if (ch == '$' or ch == '@')
					state = 11;
				else
					state = start = Restart(start);
				break;
			}
			
			case 11:
			{
				if (not (isalnum(ch) or ch == '_'))
				{
					Retract();
					token = cfVariable;
				}
				break;				
			}
			
			case 20:
			{
				if (ch == '\'')
				{
					fToken.clear();
					state = 21;
				}
				else
					state = start = Restart(start);
				break;
			}
			
			case 21:
			{
				if (ch == '\\')
				{
					fToken.erase(fToken.length() - 1, 1);
					state = 22;
				}
				else if (ch == '\'')
				{
					token = cfString;
					fToken.erase(fToken.length() - 1, 1);
				}
				else if (ch == 0)
					throw config_exception("unterminated string constant", fLine);
				break;
			}
			
			case 22:
				state = 21;
				break;
			
			case 30:
			{
				if (ch == '"')
				{
					fToken.clear();
					state = 31;
				}
				else
					state = start = Restart(start);
				break;
			}
			
			case 31:
			{
				if (ch == '\\')
				{
					fToken.erase(fToken.length() - 1, 1);
					state = 32;
				}
				else if (ch == '"')
				{
					token = cfString;
					fToken.erase(fToken.length() - 1, 1);
				}
				else if (ch == 0)
					throw config_exception("unterminated string constant", fLine);
				break;
			}
			
			case 32:
				state = 31;
				break;
			
			case 40:
				if (isdigit(ch))
					state = 41;
				else
					state = start = Restart(start);
				break;
			
			case 41:
				if (not isdigit(ch))
				{
					Retract();
					token = cfNumber;
				}
				break;
			
			case 50:
				if (isalpha(ch))
					state = 51;
				else
					state = start = Restart(start);
				break;
			
			case 51:
				if (not (isalnum(ch) or ch == '_'))
				{
					Retract();
					token = cfIdentifier;
				}
				break;
			
			case 60:
				switch (ch)
				{
					case '=':
						state = 61;
						break;
					
					default:
						token = ch;
				}
				break;
			
			case 61:
				if (ch == '>')
					token = cfLeft;
				else
				{
					Retract();
					token = '=';
				}
				break;
			
			default:
				throw config_exception("invalid state in tokenizer", fLine);
		}
	}
	
	fPointer = fFile.tellg();
	return token;
}

void CfParser::Match(int inToken)
{
	if (fLookahead == inToken)
		fLookahead = GetToken();
	else
	{
		string err = "expected ";
		
		if (inToken == cfEOF or inToken >= cfUndefined)
			err += fTokenNames[inToken];
		else
			err += char(inToken);
		
		err += " but found ";
		
		if (fLookahead == cfEOF or fLookahead >= cfUndefined)
			err += fTokenNames[fLookahead];
		else
			err += char(fLookahead);
		
		throw config_exception(err, fLine);
	}
}

void CfParser::Parse(string& outDataDir, string& outFormatDir, vector<DbInfo>& outDbInfo)
{
	while (fLookahead != cfEOF)
		ParseStatement();
	
	outDataDir = fDataDir;
	outFormatDir = fFormatDir;
	outDbInfo = fDbInfo;
}

void CfParser::ParseStatement()
{
	string variable = fToken;
	Match(cfVariable);
	ParseAssignment(variable);
	Match(';');
}

void CfParser::ParseAssignment(const string& inVariable)
{
	Match('=');
	if (fLookahead == '(')
	{
		if (inVariable != "@dbs")
			throw config_exception(string("unknown list variable ") + inVariable, fLine);
		ParseList();
	}
	else
	{
		string value = fToken;
		if (fLookahead == cfIdentifier)
		{
			Match(cfIdentifier);
			if (value != "undef")
				throw config_exception(string("unknown identifier ") + value, fLine);
		}
		else if (fLookahead == cfNumber)
		{
			Match(cfNumber);
		}
		else
		{
			Match(cfString);
			if (inVariable == "$mrs_data")
				fDataDir = value;
			else if (inVariable == "$mrs_format")
				fFormatDir = value;
		}
	}
}

void CfParser::ParseList()
{
	Match('(');
	while (fLookahead != ')' and fLookahead != cfEOF)
	{
		if (fLookahead == '{')
		{
			fDbInfo.push_back(DbInfo());
			ParseHash();
		}
		
		if (fLookahead == ',')
			Match(',');
		else
			break;
	}
	Match(')');
}

void CfParser::ParseHash()
{
	Match('{');
	
	while (fLookahead != '}' and fLookahead != cfEOF)
	{
		string key, value;
		
		key = fToken;
		
		if (fLookahead == cfString)
			Match(cfString);
		else
			Match(cfIdentifier);
		
		if (fLookahead == ',')
			Match(',');
		else
			Match(cfLeft);
		
		value = fToken;
		
		if (fLookahead == cfIdentifier or fLookahead == cfNumber)
			Match(fLookahead);
		else
			Match(cfString);
		
		if (key == "id")
			fDbInfo.back().id = value;
		else if (key == "name")
			fDbInfo.back().name = value;
		else if (key == "url")
			fDbInfo.back().url = value;
		else if (key == "script")
			fDbInfo.back().script = value;
		else if (key == "blast")
			fDbInfo.back().blast = atoi(value.c_str()) != 0;
		else if (key == "in_all")
			fDbInfo.back().in_all = atoi(value.c_str()) != 0;
		
		if (fLookahead == ',')
			Match(',');
		else
			break;
	}
	
	Match('}');
}

void ReadConfig(
	const string&		inPath,
	string&				outDataDir,
	string&				outFormatDir,
	vector<DbInfo>&		outDbInfo)
{
	CfParser p(inPath);
	p.Parse(outDataDir, outFormatDir, outDbInfo);
}

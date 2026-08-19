#include "ECSParser.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_set>
using namespace std;

bool ECSParser::parseFile(const string& filePath, vector<ECSStructInfo>& structList, string& error)
{
	ifstream stream(filePath, ios::binary);
	if (!stream.is_open())
	{
		error = filePath + "(1): EasyECS error: Cannot open file";
		return false;
	}
	stringstream buffer;
	buffer << stream.rdbuf();
	string content = removeComments(buffer.str());
	regex structRegex(R"(\bECS\s*\(\s*\)\s*struct\s+([A-Za-z_]\w*)\s*\{)");
	sregex_iterator iter(content.begin(), content.end(), structRegex);
	sregex_iterator end;
	unordered_set<string> qualifiedNameSet;
	vector<pair<size_t, size_t>> ecsStructRanges;
	for (; iter != end; ++iter)
	{
		ECSStructInfo info;
		info.mName = (*iter)[1].str();
		info.mSourcePath = filePath;
		info.mLine = getLineNumber(content, static_cast<size_t>(iter->position()));
		bool hasNonNamespaceScope = false;
		if (!findNamespaceAt(content, static_cast<size_t>(iter->position()), info.mNamespaceList, hasNonNamespaceScope, error))
		{
			error = formatError(filePath, info.mLine, info.mName, "", error);
			return false;
		}
		string qualifiedName = getQualifiedName(info);
		if (hasNonNamespaceScope)
		{
			error = formatError(filePath, info.mLine, qualifiedName, "", "Anonymous namespaces and ECS structs nested in non-namespace scopes are not supported");
			return false;
		}
		if (!qualifiedNameSet.insert(qualifiedName).second)
		{
			error = formatError(filePath, info.mLine, qualifiedName, "", "Duplicate ECS struct in the same file");
			return false;
		}
		size_t openBrace = static_cast<size_t>(iter->position() + iter->length() - 1);
		size_t closeBrace = findMatchingBrace(content, openBrace);
		if (closeBrace == string::npos)
		{
			error = formatError(filePath, info.mLine, qualifiedName, "", "ECS struct is missing the closing brace");
			return false;
		}
		ecsStructRanges.push_back({ static_cast<size_t>(iter->position()), closeBrace });
		size_t bodyStart = openBrace + 1;
		string body = content.substr(bodyStart, closeBrace - bodyStart);
		vector<ECSStatementInfo> statements;
		splitStatements(body, statements);
		unordered_set<string> fieldNameSet;
		for (const ECSStatementInfo& statementInfo : statements)
		{
			ECSFieldInfo field;
			bool isField = false;
			size_t firstNonSpace = 0;
			while (firstNonSpace < statementInfo.mText.size() && isspace(static_cast<unsigned char>(statementInfo.mText[firstNonSpace]))) ++firstNonSpace;
			int statementLine = getLineNumber(content, bodyStart + statementInfo.mOffset + firstNonSpace);
			if (!parseField(statementInfo.mText, field, isField, error))
			{
				error = formatError(filePath, statementLine, qualifiedName, field.mName, error);
				return false;
			}
			if (!isField) continue;
			field.mLine = statementLine;
			if (!fieldNameSet.insert(field.mName).second)
			{
				error = formatError(filePath, field.mLine, qualifiedName, field.mName, "Duplicate field");
				return false;
			}
			info.mFields.push_back(field);
		}
		if (info.mFields.empty())
		{
			error = formatError(filePath, info.mLine, qualifiedName, "", "ECS struct has no fields to generate");
			return false;
		}
		structList.push_back(info);
	}
	regex notECSRegex(R"(\bNOT_ECS\s*\(\s*\))");
	for (sregex_iterator notIter(content.begin(), content.end(), notECSRegex); notIter != end; ++notIter)
	{
		size_t position = static_cast<size_t>(notIter->position());
		bool insideECSStruct = false;
		for (const auto& range : ecsStructRanges)
		{
			if (position >= range.first && position <= range.second)
			{
				insideECSStruct = true;
				break;
			}
		}
		string easyECSHeader = "EasyECS.h";
		string fileName = "";
		if (filePath.size() >= easyECSHeader.length())
		{
			fileName = filePath.substr(filePath.size() - easyECSHeader.length(), easyECSHeader.length());
		}
		if (!insideECSStruct && fileName != "EasyECS.h")
		{
			error = formatError(filePath, getLineNumber(content, position), "", "", "NOT_ECS() can only be used inside an ECS() struct");
			return false;
		}
	}
	return true;
}
string ECSParser::removeComments(const string& content)
{
	string result;
	result.reserve(content.size());
	bool lineComment = false;
	bool blockComment = false;
	bool inString = false;
	bool inChar = false;
	bool escape = false;
	for (size_t i = 0; i < content.size(); ++i)
	{
		char c = content[i];
		char next = i + 1 < content.size() ? content[i + 1] : '\0';
		if (lineComment)
		{
			if (c == '\n')
			{
				lineComment = false;
				result.push_back(c);
			}
			else result.push_back(' ');
			continue;
		}
		if (blockComment)
		{
			if (c == '*' && next == '/')
			{
				blockComment = false;
				result.push_back(' ');
				result.push_back(' ');
				++i;
			}
			else result.push_back(c == '\n' ? '\n' : ' ');
			continue;
		}
		if (!inString && !inChar && c == '/' && next == '/')
		{
			lineComment = true;
			result.push_back(' ');
			result.push_back(' ');
			++i;
			continue;
		}
		if (!inString && !inChar && c == '/' && next == '*')
		{
			blockComment = true;
			result.push_back(' ');
			result.push_back(' ');
			++i;
			continue;
		}
		result.push_back(c);
		if (escape)
		{
			escape = false;
			continue;
		}
		if ((inString || inChar) && c == '\\')
		{
			escape = true;
			continue;
		}
		if (!inChar && c == '"') inString = !inString;
		else if (!inString && c == '\'') inChar = !inChar;
	}
	return result;
}
string ECSParser::trim(const string& value)
{
	size_t begin = 0;
	while (begin < value.size() && isspace(static_cast<unsigned char>(value[begin]))) ++begin;
	size_t end = value.size();
	while (end > begin && isspace(static_cast<unsigned char>(value[end - 1]))) --end;
	return value.substr(begin, end - begin);
}
size_t ECSParser::findMatchingBrace(const string& content, size_t openBrace)
{
	int depth = 0;
	bool inString = false;
	bool inChar = false;
	bool escape = false;
	for (size_t i = openBrace; i < content.size(); ++i)
	{
		char c = content[i];
		if (escape)
		{
			escape = false;
			continue;
		}
		if ((inString || inChar) && c == '\\')
		{
			escape = true;
			continue;
		}
		if (!inChar && c == '"')
		{
			inString = !inString;
			continue;
		}
		if (!inString && c == '\'')
		{
			inChar = !inChar;
			continue;
		}
		if (inString || inChar) continue;
		if (c == '{') ++depth;
		else if (c == '}')
		{
			--depth;
			if (depth == 0) return i;
		}
	}
	return string::npos;
}
void ECSParser::splitStatements(const string& body, vector<ECSStatementInfo>& statements)
{
	int angleDepth = 0;
	int parenthesisDepth = 0;
	int braceDepth = 0;
	int bracketDepth = 0;
	bool inString = false;
	bool inChar = false;
	bool escape = false;
	size_t start = 0;
	for (size_t i = 0; i < body.size(); ++i)
	{
		char c = body[i];
		if (escape)
		{
			escape = false;
			continue;
		}
		if ((inString || inChar) && c == '\\')
		{
			escape = true;
			continue;
		}
		if (!inChar && c == '"')
		{
			inString = !inString;
			continue;
		}
		if (!inString && c == '\'')
		{
			inChar = !inChar;
			continue;
		}
		if (inString || inChar) continue;
		if (c == '<') ++angleDepth;
		else if (c == '>' && angleDepth > 0) --angleDepth;
		else if (c == '(') ++parenthesisDepth;
		else if (c == ')' && parenthesisDepth > 0) --parenthesisDepth;
		else if (c == '{') ++braceDepth;
		else if (c == '}' && braceDepth > 0) --braceDepth;
		else if (c == '[') ++bracketDepth;
		else if (c == ']' && bracketDepth > 0) --bracketDepth;
		else if (c == ';' && angleDepth == 0 && parenthesisDepth == 0 && braceDepth == 0 && bracketDepth == 0)
		{
			statements.push_back({ body.substr(start, i - start), start });
			start = i + 1;
		}
	}
}
bool ECSParser::parseField(const string& statementValue, ECSFieldInfo& field, bool& isField, string& error)
{
	isField = false;
	string statement = trim(statementValue);
	if (statement.empty()) return true;
	if (statement == "public:" || statement == "private:" || statement == "protected:") return true;
	bool notECS = false;
	if (statement.find("NOT_ECS") != string::npos && !regex_search(statement, regex(R"(^\s*NOT_ECS\s*\(\s*\))")))
	{
		error = "NOT_ECS() must appear at the beginning of a field declaration";
		return false;
	}
	regex notECSRegex(R"(^\s*NOT_ECS\s*\(\s*\)\s*)");
	smatch notECSMatch;
	if (regex_search(statement, notECSMatch, notECSRegex))
	{
		notECS = true;
		statement = trim(statement.substr(static_cast<size_t>(notECSMatch.length())));
	}
	if (statement.find("NOT_ECS") != string::npos)
	{
		error = "NOT_ECS() may only be specified once on a field";
		return false;
	}
	if (statement.empty())
	{
		error = "NOT_ECS() must be followed by a field declaration";
		return false;
	}
	if (!stripLeadingStandardAttributes(statement, error)) return false;
	if (statement.rfind("using ", 0) == 0 || statement.rfind("typedef ", 0) == 0 || statement.rfind("static_assert", 0) == 0 ||
		statement.rfind("struct ", 0) == 0 || statement.rfind("class ", 0) == 0 || statement.rfind("enum ", 0) == 0)
	{
		if (notECS)
		{
			error = "NOT_ECS() can only be used on a non-static data field";
			return false;
		}
		return true;
	}
	int angleDepth = 0;
	int bracketDepth = 0;
	size_t initializerPos = string::npos;
	for (size_t i = 0; i < statement.size(); ++i)
	{
		char c = statement[i];
		if (c == '<') ++angleDepth;
		else if (c == '>' && angleDepth > 0) --angleDepth;
		else if (c == '[') ++bracketDepth;
		else if (c == ']' && bracketDepth > 0) --bracketDepth;
		else if ((c == '=' || c == '{') && angleDepth == 0 && bracketDepth == 0)
		{
			initializerPos = i;
			break;
		}
	}
	string declaration = trim(initializerPos == string::npos ? statement : statement.substr(0, initializerPos));
	if (declaration.find("(*") != string::npos || declaration.find("(&") != string::npos)
	{
		smatch pointerNameMatch;
		if (regex_search(declaration, pointerNameMatch, regex(R"([*&]\s*([A-Za-z_]\w*)\s*\))"))) field.mName = pointerNameMatch[1].str();
		error = "Function-pointer and function-reference fields are not supported";
		return false;
	}
	if (declaration.find('(') != string::npos || declaration.find(')') != string::npos)
	{
		if (notECS)
		{
			error = "NOT_ECS() cannot be used on a function";
			return false;
		}
		return true;
	}
	if (hasTopLevelComma(declaration))
	{
		error = "Multiple fields in one declaration are not supported";
		return false;
	}
	if (declaration.find('[') != string::npos || declaration.find(']') != string::npos)
	{
		smatch arrayNameMatch;
		if (regex_search(declaration, arrayNameMatch, regex(R"(([A-Za-z_]\w*)\s*\[)"))) field.mName = arrayNameMatch[1].str();
		error = "C-style array fields are not supported";
		return false;
	}
	if (hasBitFieldColon(declaration))
	{
		smatch bitFieldNameMatch;
		if (regex_search(declaration, bitFieldNameMatch, regex(R"(([A-Za-z_]\w*)\s*:)"))) field.mName = bitFieldNameMatch[1].str();
		error = "Bit-field members are not supported";
		return false;
	}
	regex nameRegex(R"(([A-Za-z_]\w*)\s*$)");
	smatch nameMatch;
	if (!regex_search(declaration, nameMatch, nameRegex))
	{
		if (notECS)
		{
			error = "NOT_ECS() must be followed by a valid field declaration";
			return false;
		}
		return true;
	}
	string name = nameMatch[1].str();
	field.mName = name;
	size_t namePos = static_cast<size_t>(nameMatch.position(1));
	string type = trim(declaration.substr(0, namePos));
	if (type.empty())
	{
		error = "Field must have an explicit type";
		return false;
	}
	if (type.rfind("static ", 0) == 0 || type.rfind("constexpr ", 0) == 0 || type.rfind("consteval ", 0) == 0 || type.rfind("inline ", 0) == 0)
	{
		if (notECS)
		{
			error = "NOT_ECS() cannot be used on a static/constexpr field";
			return false;
		}
		return true;
	}
	field.mType = type;
	field.mNotECS = notECS;
	if (!analyzeFieldType(type, field, error)) return false;
	isField = true;
	return true;
}
bool ECSParser::stripLeadingStandardAttributes(string& statement, string& error)
{
	statement = trim(statement);
	while (statement.rfind("[[", 0) == 0)
	{
		size_t close = statement.find("]]", 2);
		if (close == string::npos)
		{
			error = "C++ field attribute is missing ]]";
			return false;
		}
		statement = trim(statement.substr(close + 2));
	}
	if (regex_search(statement, regex(R"(^\s*alignas\s*\()")))
	{
		error = "alignas on ECS fields is not supported because per-element alignment semantics cannot be preserved in a SoA column";
		return false;
	}
	smatch macroMatch;
	if (regex_search(statement, macroMatch, regex(R"(^\s*([A-Za-z_]\w*)\s*\([^)]*\)\s+.+)")))
	{
		error = "Function-style field declaration macro/attribute is not supported by the lightweight parser:" + macroMatch[1].str();
		return false;
	}
	return true;
}
bool ECSParser::analyzeFieldType(const string& typeValue, ECSFieldInfo& field, string& error)
{
	string type = trim(typeValue);
	if (type.find('*') != string::npos)
	{
		error = "Pointer fields are not supported";
		return false;
	}
	if (type.find('&') != string::npos)
	{
		error = "Reference fields are not supported";
		return false;
	}
	if (containsWord(type, "volatile"))
	{
		error = "volatile fields are not supported";
		return false;
	}
	if (containsWord(type, "mutable"))
	{
		error = "mutable fields are not supported";
		return false;
	}
	if (containsWord(type, "auto") || containsWord(type, "decltype"))
	{
		error = "auto and decltype fields are not supported; use an explicit field type";
		return false;
	}
	bool isConst = false;
	string storageType = type;
	if (removeLeadingWord(storageType, "const")) isConst = true;
	if (removeTrailingWord(storageType, "const")) isConst = true;
	storageType = trim(storageType);
	if (containsWord(storageType, "const"))
	{
		error = "Nested const qualifiers are not supported; only top-level const fields are supported";
		return false;
	}
	if (storageType.empty())
	{
		error = "Field type is empty after qualifiers are removed";
		return false;
	}
	if (isKnownNonTrivialType(storageType))
	{
		error = "Known non-trivial/RAII field type is not supported:" + storageType;
		return false;
	}
	field.mConst = isConst;
	field.mStorageType = storageType;
	return true;
}
bool ECSParser::hasTopLevelComma(const string& text)
{
	int angleDepth = 0;
	int parenthesisDepth = 0;
	int braceDepth = 0;
	int bracketDepth = 0;
	for (char c : text)
	{
		if (c == '<') ++angleDepth;
		else if (c == '>' && angleDepth > 0) --angleDepth;
		else if (c == '(') ++parenthesisDepth;
		else if (c == ')' && parenthesisDepth > 0) --parenthesisDepth;
		else if (c == '{') ++braceDepth;
		else if (c == '}' && braceDepth > 0) --braceDepth;
		else if (c == '[') ++bracketDepth;
		else if (c == ']' && bracketDepth > 0) --bracketDepth;
		else if (c == ',' && angleDepth == 0 && parenthesisDepth == 0 && braceDepth == 0 && bracketDepth == 0) return true;
	}
	return false;
}
bool ECSParser::hasBitFieldColon(const string& text)
{
	int angleDepth = 0;
	int parenthesisDepth = 0;
	int braceDepth = 0;
	int bracketDepth = 0;
	for (size_t i = 0; i < text.size(); ++i)
	{
		char c = text[i];
		if (c == '<') ++angleDepth;
		else if (c == '>' && angleDepth > 0) --angleDepth;
		else if (c == '(') ++parenthesisDepth;
		else if (c == ')' && parenthesisDepth > 0) --parenthesisDepth;
		else if (c == '{') ++braceDepth;
		else if (c == '}' && braceDepth > 0) --braceDepth;
		else if (c == '[') ++bracketDepth;
		else if (c == ']' && bracketDepth > 0) --bracketDepth;
		else if (c == ':' && angleDepth == 0 && parenthesisDepth == 0 && braceDepth == 0 && bracketDepth == 0)
		{
			bool namespaceColon = (i > 0 && text[i - 1] == ':') || (i + 1 < text.size() && text[i + 1] == ':');
			if (!namespaceColon) return true;
		}
	}
	return false;
}
bool ECSParser::isKnownNonTrivialType(const string& type)
{
	string compact;
	compact.reserve(type.size());
	for (char c : type)
	{
		if (!isspace(static_cast<unsigned char>(c))) compact.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
	}
	static const char* patterns[] =
	{
		"std::string", "std::wstring", "std::u16string", "std::u32string", "std::basic_string<",
		"std::vector<", "std::map<", "std::multimap<", "std::unordered_map<", "std::unordered_multimap<",
		"std::list<", "std::forward_list<", "std::deque<", "std::set<", "std::multiset<", "std::unordered_set<", "std::unordered_multiset<",
		"std::shared_ptr<", "std::unique_ptr<", "std::weak_ptr<", "std::function<", "std::any", "std::variant<", "std::optional<",
		"std::mutex", "std::recursive_mutex", "std::condition_variable", "std::thread", "std::jthread"
	};
	for (const char* pattern : patterns)
	{
		if (compact.find(pattern) != string::npos) return true;
	}
	return false;
}
bool ECSParser::containsWord(const string& text, const string& word)
{
	size_t position = 0;
	while ((position = text.find(word, position)) != string::npos)
	{
		bool left = position == 0 || !isIdentifierChar(text[position - 1]);
		size_t rightPosition = position + word.size();
		bool right = rightPosition >= text.size() || !isIdentifierChar(text[rightPosition]);
		if (left && right) return true;
		position += word.size();
	}
	return false;
}
bool ECSParser::removeLeadingWord(string& text, const string& word)
{
	text = trim(text);
	if (text.size() < word.size() || text.compare(0, word.size(), word) != 0) return false;
	if (text.size() > word.size() && isIdentifierChar(text[word.size()])) return false;
	text = trim(text.substr(word.size()));
	return true;
}
bool ECSParser::removeTrailingWord(string& text, const string& word)
{
	text = trim(text);
	if (text.size() < word.size()) return false;
	size_t position = text.size() - word.size();
	if (text.compare(position, word.size(), word) != 0) return false;
	if (position > 0 && isIdentifierChar(text[position - 1])) return false;
	text = trim(text.substr(0, position));
	return true;
}
bool ECSParser::findNamespaceAt(const string& content, size_t position, vector<string>& namespaceList, bool& hasNonNamespaceScope, string& error)
{
	struct ScopeInfo
	{
		int mNamespaceCount = 0;
		bool mNamespaceScope = false;
	};
	vector<ScopeInfo> scopeList;
	namespaceList.clear();
	hasNonNamespaceScope = false;
	int nonNamespaceDepth = 0;
	string lastIdentifier;
	bool inString = false;
	bool inChar = false;
	bool escape = false;
	for (size_t i = 0; i < position;)
	{
		char c = content[i];
		if (escape)
		{
			escape = false;
			++i;
			continue;
		}
		if ((inString || inChar) && c == '\\')
		{
			escape = true;
			++i;
			continue;
		}
		if (!inChar && c == '"')
		{
			inString = !inString;
			++i;
			continue;
		}
		if (!inString && c == '\'')
		{
			inChar = !inChar;
			++i;
			continue;
		}
		if (inString || inChar)
		{
			++i;
			continue;
		}
		if (isIdentifierStart(c))
		{
			size_t begin = i++;
			while (i < position && isIdentifierChar(content[i])) ++i;
			string token = content.substr(begin, i - begin);
			if (token == "namespace" && lastIdentifier != "using")
			{
				size_t cursor = i;
				while (cursor < position && isspace(static_cast<unsigned char>(content[cursor]))) ++cursor;
				vector<string> parts;
				while (cursor < position && isIdentifierStart(content[cursor]))
				{
					size_t partBegin = cursor++;
					while (cursor < position && isIdentifierChar(content[cursor])) ++cursor;
					parts.push_back(content.substr(partBegin, cursor - partBegin));
					while (cursor < position && isspace(static_cast<unsigned char>(content[cursor]))) ++cursor;
					if (cursor + 1 < position && content[cursor] == ':' && content[cursor + 1] == ':')
					{
						cursor += 2;
						while (cursor < position && isspace(static_cast<unsigned char>(content[cursor]))) ++cursor;
						continue;
					}
					break;
				}
				while (cursor < position && isspace(static_cast<unsigned char>(content[cursor]))) ++cursor;
				if (cursor < position && content[cursor] == '{')
				{
					if (parts.empty())
					{
						scopeList.push_back({ 0, false });
						++nonNamespaceDepth;
					}
					else
					{
						for (const string& part : parts) namespaceList.push_back(part);
						scopeList.push_back({ static_cast<int>(parts.size()), true });
					}
					i = cursor + 1;
					lastIdentifier.clear();
					continue;
				}
				if (cursor < position && content[cursor] == '=')
				{
					while (cursor < position && content[cursor] != ';') ++cursor;
					i = cursor < position ? cursor + 1 : cursor;
					lastIdentifier.clear();
					continue;
				}
			}
			lastIdentifier = token;
			continue;
		}
		if (c == '{')
		{
			scopeList.push_back({ 0, false });
			++nonNamespaceDepth;
			lastIdentifier.clear();
			++i;
			continue;
		}
		if (c == '}')
		{
			if (scopeList.empty())
			{
				error = "Unmatched closing brace while parsing namespace";
				return false;
			}
			ScopeInfo scope = scopeList.back();
			scopeList.pop_back();
			if (scope.mNamespaceScope)
			{
				for (int count = 0; count < scope.mNamespaceCount && !namespaceList.empty(); ++count) namespaceList.pop_back();
			}
			else if (nonNamespaceDepth > 0) --nonNamespaceDepth;
			lastIdentifier.clear();
			++i;
			continue;
		}
		if (c == ';' || c == '=') lastIdentifier.clear();
		++i;
	}
	hasNonNamespaceScope = nonNamespaceDepth > 0;
	return true;
}
bool ECSParser::isIdentifierStart(char value)
{
	return value == '_' || isalpha(static_cast<unsigned char>(value)) != 0;
}
bool ECSParser::isIdentifierChar(char value)
{
	return value == '_' || isalnum(static_cast<unsigned char>(value)) != 0;
}
int ECSParser::getLineNumber(const string& content, size_t position)
{
	position = min(position, content.size());
	return 1 + static_cast<int>(count(content.begin(), content.begin() + static_cast<ptrdiff_t>(position), '\n'));
}
string ECSParser::formatError(const string& filePath, int line, const string& structName, const string& fieldName, const string& message)
{
	string location = filePath + "(" + to_string(line > 0 ? line : 1) + "): EasyECS error: ";
	if (!structName.empty())
	{
		location += structName;
		if (!fieldName.empty()) location += "::" + fieldName;
		location += ": ";
	}
	return location + message;
}
string ECSParser::getQualifiedName(const ECSStructInfo& info)
{
	string value;
	for (const string& item : info.mNamespaceList)
	{
		if (!value.empty()) value += "::";
		value += item;
	}
	if (!value.empty()) value += "::";
	value += info.mName;
	return value;
}

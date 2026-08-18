#pragma once
#include <cstddef>
#include <string>
#include <vector>

struct ECSFieldInfo
{
	std::string mType;
	std::string mStorageType;
	std::string mName;
	bool mNotECS = false;
	bool mConst = false;
	int mLine = 0;
};
struct ECSStructInfo
{
	std::string mName;
	std::string mSourcePath;
	std::vector<std::string> mNamespaceList;
	std::vector<ECSFieldInfo> mFields;
	int mLine = 0;
};
struct ECSStatementInfo
{
	std::string mText;
	size_t mOffset = 0;
};
class ECSParser
{
public:
	bool parseFile(const std::string& filePath, std::vector<ECSStructInfo>& structList, std::string& error);
private:
	std::string removeComments(const std::string& content);
	std::string trim(const std::string& value);
	size_t findMatchingBrace(const std::string& content, size_t openBrace);
	void splitStatements(const std::string& body, std::vector<ECSStatementInfo>& statements);
	bool parseField(const std::string& statement, ECSFieldInfo& field, bool& isField, std::string& error);
	bool analyzeFieldType(const std::string& type, ECSFieldInfo& field, std::string& error);
	bool stripLeadingStandardAttributes(std::string& statement, std::string& error);
	bool hasTopLevelComma(const std::string& text);
	bool hasBitFieldColon(const std::string& text);
	bool isKnownNonTrivialType(const std::string& type);
	bool containsWord(const std::string& text, const std::string& word);
	bool removeLeadingWord(std::string& text, const std::string& word);
	bool removeTrailingWord(std::string& text, const std::string& word);
	bool findNamespaceAt(const std::string& content, size_t position, std::vector<std::string>& namespaceList, bool& hasNonNamespaceScope, std::string& error);
	bool isIdentifierStart(char value);
	bool isIdentifierChar(char value);
	int getLineNumber(const std::string& content, size_t position);
	std::string formatError(const std::string& filePath, int line, const std::string& structName, const std::string& fieldName, const std::string& message);
	std::string getQualifiedName(const ECSStructInfo& info);
};

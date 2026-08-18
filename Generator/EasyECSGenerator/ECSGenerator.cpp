#include "ECSGenerator.h"
#include <cctype>
#include <fstream>
#include <sstream>
using namespace std;

namespace
{
string getRefType(const ECSFieldInfo& field, bool constRef)
{
	return (constRef || field.mConst ? "const " : "") + field.mStorageType + "&";
}
string getFieldReadExpression(const ECSFieldInfo& field)
{
	return field.mNotECS ? "mStorage.mAoS[index]." + field.mName : "mStorage." + field.mName + "[index]";
}
bool hasConstField(const ECSStructInfo& info)
{
	for (const ECSFieldInfo& field : info.mFields)
	{
		if (field.mConst) return true;
	}
	return false;
}
bool hasConstAoSField(const vector<ECSFieldInfo>& fields)
{
	for (const ECSFieldInfo& field : fields)
	{
		if (field.mConst) return true;
	}
	return false;
}
string makeStaticAssertName(const ECSStructInfo& info, const ECSFieldInfo& field)
{
	string value;
	for (const string& item : info.mNamespaceList)
	{
		if (!value.empty()) value += "::";
		value += item;
	}
	if (!value.empty()) value += "::";
	value += info.mName + "::" + field.mName + " line " + to_string(field.mLine);
	return value;
}
void appendStaticAssert(string& output, const string& condition, const string& message)
{
	output += "\tstatic_assert(\n";
	output += "\t\t" + condition + ",\n";
	output += "\t\t\"" + message + "\");\n";
}
}
bool ECSGenerator::generate(const vector<ECSStructInfo>& structList, const string& sourceHeaderName, const string& outputHeaderPath,
	const string& outputCppPath, string& error)
{
	string header;
	header += "#pragma once\n";
	header += "#include \"" + sourceHeaderName + "\"\n";
	header += "#include \"EasyECS.h\"\n#include \"EasyECSIndexMap.h\"\n";
	header += "#include <algorithm>\n"
		"#include <cassert>\n"
		"#include <cstddef>\n"
		"#include <cstdint>\n"
		"#include <functional>\n"
		"#include <new>\n"
		"#include <optional>\n"
		"#include <type_traits>\n"
		"#include <utility>\n"
		"#include <vector>\n"
		"\n";
	for (const ECSStructInfo& info : structList)
	{
		appendNamespaceBegin(header, info);
		appendStructHeader(header, info);
		appendNamespaceEnd(header, info);
		header += "\n";
	}
	string generatedHeaderName = fileNameOnly(outputHeaderPath);
	string cpp = "#include \"" + generatedHeaderName + "\"\n#include <cstdlib>\n#include <cstring>\n#include <new>\n\n";
	for (const ECSStructInfo& info : structList)
	{
		appendNamespaceBegin(cpp, info);
		appendStructCpp(cpp, info, generatedHeaderName);
		appendNamespaceEnd(cpp, info);
		cpp += "\n";
	}
	if (!writeFileIfChanged(outputHeaderPath, header, error)) return false;
	if (!writeFileIfChanged(outputCppPath, cpp, error)) return false;
	return true;
}
bool ECSGenerator::writeFileIfChanged(const string& filePath, const string& content, string& error)
{
	ifstream input(filePath, ios::binary);
	if (input.is_open())
	{
		stringstream buffer;
		buffer << input.rdbuf();
		if (buffer.str() == content) return true;
	}
	ofstream output(filePath, ios::binary | ios::trunc);
	if (!output.is_open())
	{
		error = "Cannot write generated file:" + filePath;
		return false;
	}
	output.write(content.data(), static_cast<streamsize>(content.size()));
	if (!output.good())
	{
		error = "Failed to write generated file:" + filePath;
		return false;
	}
	return true;
}
string ECSGenerator::makeColumnSuffix(const string& fieldName)
{
	if (fieldName.size() >= 2 && fieldName[0] == 'm' && isupper(static_cast<unsigned char>(fieldName[1]))) return fieldName.substr(1);
	string value = fieldName;
	if (!value.empty()) value[0] = static_cast<char>(toupper(static_cast<unsigned char>(value[0])));
	return value;
}
void ECSGenerator::appendStructHeader(string& o, const ECSStructInfo& info)
{
	const string& n = info.mName;
	vector<ECSFieldInfo> ecsFields;
	vector<ECSFieldInfo> aosFields;
	for (const ECSFieldInfo& field : info.mFields) (field.mNotECS ? aosFields : ecsFields).push_back(field);
	bool hasConst = hasConstField(info);
	bool hasConstAoS = hasConstAoSField(aosFields);
	o += "// Source ECS struct: " + getQualifiedName(info) + "\n";
	if (!aosFields.empty())
	{
		o += "struct " + n + "AoSBlock\n{\n";
		for (const ECSFieldInfo& field : aosFields) o += "\t" + field.mStorageType + " " + field.mName + ";\n";
		o += "};\n";
	}
	o += "struct " + n + "Storage\n{\n\tvoid* mRawMemory = nullptr;\n\tuint8_t* mAlignedMemory = nullptr;\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t" + field.mStorageType + "* " + field.mName + " = nullptr;\n";
	if (!aosFields.empty()) o += "\t" + n + "AoSBlock* mAoS = nullptr;\n";
	o += "};\n";
	o += "struct " + n + "Ref\n{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t" + getRefType(field, false) + " " + field.mName + ";\n";
	for (const ECSFieldInfo& field : aosFields) o += "\t" + getRefType(field, false) + " " + field.mName + ";\n";
	o += "};\n";
	o += "struct " + n + "ConstRef\n{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t" + getRefType(field, true) + " " + field.mName + ";\n";
	for (const ECSFieldInfo& field : aosFields) o += "\t" + getRefType(field, true) + " " + field.mName + ";\n";
	o += "};\n";
	o += "template<typename TKey, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>> class " + n + "ECSDictionary;\n";
	o += "class " + n + "ECSList\n{\npublic:\n\tusing SourceType = " + n + ";\n\texplicit " + n + "ECSList(int capacity = 4);\n\t~" + n + "ECSList();\n";
	o += "\t" + n + "ECSList(const " + n + "ECSList& other);\n\t" + n + "ECSList& operator=(const " + n + "ECSList& other);\n\t" + n + "ECSList(" + n
		+ "ECSList&& other) noexcept;\n\t" + n + "ECSList& operator=(" + n + "ECSList&& other) noexcept;\n";
	o += "\tEASY_ECS_FORCE_INLINE int size() const { return mCount; }\n"
		"\tEASY_ECS_FORCE_INLINE int capacity() const { return mCapacity; }\n"
		"\tEASY_ECS_FORCE_INLINE bool empty() const { return mCount == 0; }\n";
	o += "\tEASY_ECS_FORCE_INLINE " + n + "Ref operator[](int index)\n\t{\n\t\tassert(index >= 0 && index < mCount);\n\t\treturn\n\t\t{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\t\tmStorage." + field.mName + "[index],\n";
	for (size_t i = 0; i < aosFields.size(); ++i) o += "\t\t\tmStorage.mAoS[index]." + aosFields[i].mName + (i + 1 == aosFields.size() ? "\n" : ",\n");
	if (aosFields.empty() && !ecsFields.empty())
	{
		size_t position = o.rfind(",\n");
		if (position != string::npos) o.replace(position, 2, "\n");
	}
	o += "\t\t};\n\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE " + n + "ConstRef operator[](int index) const\n"
		"\t{\n"
		"\t\tassert(index >= 0 && index < mCount);\n"
		"\t\treturn\n"
		"\t\t{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\t\tmStorage." + field.mName + "[index],\n";
	for (size_t i = 0; i < aosFields.size(); ++i) o += "\t\t\tmStorage.mAoS[index]." + aosFields[i].mName + (i + 1 == aosFields.size() ? "\n" : ",\n");
	if (aosFields.empty() && !ecsFields.empty())
	{
		size_t position = o.rfind(",\n");
		if (position != string::npos) o.replace(position, 2, "\n");
	}
	o += "\t\t};\n\t}\n";
	for (const ECSFieldInfo& field : ecsFields)
	{
		string suffix = makeColumnSuffix(field.mName);
		if (field.mConst)
		{
			o += "\tEASY_ECS_FORCE_INLINE const " + field.mStorageType + "* get" + suffix + "Column() { return mStorage." + field.mName + "; }\n";
			o += "\tEASY_ECS_FORCE_INLINE const " + field.mStorageType + "* get" + suffix + "Column() const { return mStorage." + field.mName + "; }\n";
		}
		else
		{
			o += "\tEASY_ECS_FORCE_INLINE " + field.mStorageType + "* get" + suffix + "Column() { return mStorage." + field.mName + "; }\n";
			o += "\tEASY_ECS_FORCE_INLINE const " + field.mStorageType + "* get" + suffix + "Column() const { return mStorage." + field.mName + "; }\n";
		}
	}
	if (!aosFields.empty())
	{
		if (hasConstAoS) o += "\tEASY_ECS_FORCE_INLINE const " + n + "AoSBlock* getAoSColumn() { return mStorage.mAoS; }\n";
		else o += "\tEASY_ECS_FORCE_INLINE " + n + "AoSBlock* getAoSColumn() { return mStorage.mAoS; }\n";
		o += "\tEASY_ECS_FORCE_INLINE const " + n + "AoSBlock* getAoSColumn() const { return mStorage.mAoS; }\n";
	}
	o += "\tvoid clear();\n"
		"\tvoid clearKeepCapacity();\n"
		"\tvoid clearAndRelease();\n"
		"\tvoid reserve(int capacity);\n"
		"\tvoid shrinkToFit();\n"
		"\tvoid add(const " + n + "& value);\n\tvoid addRange(const " + n + "* values, int count);\n\t" + n + "Ref addDefault();\n";
	o += "\ttemplate<typename TPredicate> int removeAll(TPredicate&& predicate)\n"
		"\t{\n"
		"\t\tint oldSize = mCount;\n"
		"\t\tif (oldSize <= 0) return 0;\n"
		"\t\tint writeIndex = 0;\n"
		"\t\tint removeCount = 0;\n"
		"\t\tconst " + n + "ECSList& readOnly = *this;\n"
		"\t\tfor (int readIndex = 0; readIndex < oldSize; ++readIndex)\n"
		"\t\t{\n"
		"\t\t\tif (predicate(readOnly[readIndex]))\n"
		"\t\t\t{\n"
		"\t\t\t\t++removeCount;\n"
		"\t\t\t\tcontinue;\n"
		"\t\t\t}\n"
		"\t\t\tif (writeIndex != readIndex) copyValue(writeIndex, readIndex);\n"
		"\t\t\t++writeIndex;\n"
		"\t\t}\n"
		"\t\tmCount = writeIndex;\n"
		"\t\treturn removeCount;\n"
		"\t}\n";
	o += "\tvoid insert(int index, const " + n + "& value);\n\tvoid removeAt(int index);\n\tvoid removeAtSwapBack(int index);\n\tvoid popBack();\n\t"
		+ n + " get(int index) const;\n\tvoid set(int index, const " + n + "& value);\nprivate:\n";
	o += "\ttemplate<typename TKey, typename THash, typename TEqual> friend class " + n + "ECSDictionary;\n";
	o += "\tvoid ensureCapacity(int requiredCapacity);\n"
		"\tvoid resizeCapacity(int newCapacity);\n"
		"\tvoid allocateStorage(" + n + "Storage& storage, int capacity);\n\tvoid releaseStorage(" + n + "Storage& storage);\n\tvoid copyStorage(" + n
			+ "Storage& target, const " + n + "Storage& source, int count);\n\tvoid swap(" + n + "ECSList& other) noexcept;\n";
	o += "\tEASY_ECS_FORCE_INLINE void writeValue(int index, const " + n + "& value)\n\t{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\tmStorage." + field.mName + "[index] = value." + field.mName + ";\n";
	for (const ECSFieldInfo& field : aosFields) o += "\t\tmStorage.mAoS[index]." + field.mName + " = value." + field.mName + ";\n";
	o += "\t}\n\tEASY_ECS_FORCE_INLINE void copyValue(int targetIndex, int sourceIndex)\n\t{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\tmStorage." + field.mName + "[targetIndex] = mStorage." + field.mName + "[sourceIndex];\n";
	if (!aosFields.empty()) o += "\t\tmStorage.mAoS[targetIndex] = mStorage.mAoS[sourceIndex];\n";
	o += "\t}\n";
	o += "\tvoid compactRemoved(const uint8_t* removed, int oldSize)\n"
		"\t{\n"
		"\t\tassert(removed != nullptr && oldSize == mCount);\n"
		"\t\tint writeIndex = 0;\n"
		"\t\tfor (int readIndex = 0; readIndex < oldSize; ++readIndex)\n"
		"\t\t{\n"
		"\t\t\tif (removed[static_cast<size_t>(readIndex)] != 0) continue;\n"
		"\t\t\tif (writeIndex != readIndex) copyValue(writeIndex, readIndex);\n"
		"\t\t\t++writeIndex;\n"
		"\t\t}\n"
		"\t\tmCount = writeIndex;\n"
		"\t}\n";
	o += "\t" + n + "Storage mStorage;\n\tint mCount = 0;\n\tint mCapacity = 0;\n};\n";
	o += "template<typename TKey, typename THash, typename TEqual>\nclass " + n + "ECSDictionary\n{\npublic:\n\tusing SourceType = " + n
		+ ";\n\tusing IndexMap = EasyECSIndexMap<TKey, THash, TEqual>;\n\texplicit " + n
		+ "ECSDictionary(int capacity = 4) : mIndexMap(capacity), mValues(capacity)\n"
		"\t{\n"
		"\t\tif (capacity < 1) capacity = 4;\n"
		"\t\treserve(capacity);\n"
		"\t}\n";
	o += "\t" + n + "ECSDictionary(const " + n + "ECSDictionary& other) : mIndexMap(other.mIndexMap), mValues(other.mValues) {}\n";
	o += "\t" + n + "ECSDictionary& operator=(const " + n + "ECSDictionary& other)\n";
	o += "\t{\n";
	o += "\t\tif (this == &other) return *this;\n";
	o += "\t\t" + n + "ECSDictionary copy(other);\n";
	o += "\t\t*this = std::move(copy);\n";
	o += "\t\treturn *this;\n";
	o += "\t}\n";
	o += "\t" + n + "ECSDictionary(" + n + "ECSDictionary&& other) : mIndexMap(std::move(other.mIndexMap)), mValues(std::move(other.mValues)) {}\n";
	o += "\t" + n + "ECSDictionary& operator=(" + n + "ECSDictionary&& other)\n";
	o += "\t{\n";
	o += "\t\tif (this == &other) return *this;\n";
	o += "\t\tmIndexMap = std::move(other.mIndexMap);\n";
	o += "\t\tmValues = std::move(other.mValues);\n";
	o += "\t\treturn *this;\n";
	o += "\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE int size() const { return mIndexMap.size(); }\n"
		"\tEASY_ECS_FORCE_INLINE int capacity() const { return mValues.capacity(); }\n"
		"\tEASY_ECS_FORCE_INLINE int indexCapacity() const { return mIndexMap.capacity(); }\n"
		"\tEASY_ECS_FORCE_INLINE size_t indexMemoryUsageBytes() const { return mIndexMap.memoryUsageBytes(); }\n"
		"\tEASY_ECS_FORCE_INLINE bool empty() const { return mIndexMap.empty(); }\n"
		"\tEASY_ECS_FORCE_INLINE bool containsKey(const TKey& key) const { return mIndexMap.contains(key); }\n";
	o += "\tEASY_ECS_FORCE_INLINE int getIndex(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return *index; }\n";
	o += "\tEASY_ECS_FORCE_INLINE bool tryGetIndex(const TKey& key, int& index) const\n";
	o += "\t{\n";
	o += "\t\tconst int* foundIndex = mIndexMap.findIndex(key);\n";
	o += "\t\tif (foundIndex == nullptr) return false;\n";
	o += "\t\tindex = *foundIndex;\n";
	o += "\t\treturn true;\n";
	o += "\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE " + n + "Ref operator[](const TKey& key)\n";
	o += "\t{\n";
	o += "\t\tconst int* index = mIndexMap.findIndex(key);\n";
	o += "\t\tassert(index != nullptr);\n";
	o += "\t\treturn mValues[*index];\n";
	o += "\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE " + n + "ConstRef operator[](const TKey& key) const\n";
	o += "\t{\n";
	o += "\t\tconst int* index = mIndexMap.findIndex(key);\n";
	o += "\t\tassert(index != nullptr);\n";
	o += "\t\treturn mValues[*index];\n";
	o += "\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE " + n + "Ref getValueByIndex(int index) { return mValues[index]; }\n\tEASY_ECS_FORCE_INLINE " + n
		+ "ConstRef getValueByIndex(int index) const { return mValues[index]; }\n"
		"\tEASY_ECS_FORCE_INLINE const TKey& getKeyByIndex(int index) const { return mIndexMap.getKeyByIndex(index); }\n";
	o += "\tEASY_ECS_FORCE_INLINE " + n + "Ref valueAt(int index) { return mValues[index]; }\n\tEASY_ECS_FORCE_INLINE " + n
		+ "ConstRef valueAt(int index) const { return mValues[index]; }\n"
		"\tEASY_ECS_FORCE_INLINE const TKey& keyAt(int index) const { return mIndexMap.getKeyByIndex(index); }\n";
	o += "\ttemplate<typename TAction> EASY_ECS_FORCE_INLINE void forEach(TAction&& action)\n";
	o += "\t{\n";
	o += "\t\tint count = size();\n";
	o += "\t\tfor (int i = 0; i < count; ++i) action(keyAt(i), valueAt(i));\n";
	o += "\t}\n";
	o += "\ttemplate<typename TAction> EASY_ECS_FORCE_INLINE void forEach(TAction&& action) const\n";
	o += "\t{\n";
	o += "\t\tint count = size();\n";
	o += "\t\tfor (int i = 0; i < count; ++i) action(keyAt(i), valueAt(i));\n";
	o += "\t}\n";
	o += "\ttemplate<typename TPredicate> int removeAll(TPredicate&& predicate)\n"
		"\t{\n"
		"\t\tint oldSize = size();\n"
		"\t\tif (oldSize <= 0) return 0;\n"
		"\t\tstd::vector<uint8_t> removed(static_cast<size_t>(oldSize), 0);\n"
		"\t\tint removeCount = 0;\n"
		"\t\tconst " + n + "ECSDictionary& readOnly = *this;\n"
		"\t\tfor (int i = 0; i < oldSize; ++i)\n"
		"\t\t{\n"
		"\t\t\tif (!predicate(readOnly.keyAt(i), readOnly.valueAt(i))) continue;\n"
		"\t\t\tremoved[static_cast<size_t>(i)] = 1;\n"
		"\t\t\t++removeCount;\n"
		"\t\t}\n"
		"\t\tif (removeCount == 0) return 0;\n"
		"\t\tif (removeCount == oldSize)\n"
		"\t\t{\n"
		"\t\t\tclear();\n"
		"\t\t\treturn removeCount;\n"
		"\t\t}\n"
		"\t\tmValues.compactRemoved(removed.data(), oldSize);\n"
		"\t\tmIndexMap.compactRemove(removed.data(), oldSize);\n"
		"\t\treturn removeCount;\n"
		"\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE std::optional<" + n + "Ref> tryGetRef(const TKey& key)\n";
	o += "\t{\n";
	o += "\t\tconst int* index = mIndexMap.findIndex(key);\n";
	o += "\t\tif (index == nullptr) return std::nullopt;\n";
	o += "\t\treturn mValues[*index];\n";
	o += "\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE std::optional<" + n + "ConstRef> tryGetRef(const TKey& key) const\n";
	o += "\t{\n";
	o += "\t\tconst int* index = mIndexMap.findIndex(key);\n";
	o += "\t\tif (index == nullptr) return std::nullopt;\n";
	o += "\t\treturn mValues[*index];\n";
	o += "\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE " + n + "ECSList& getValues() { return mValues; }\n\tEASY_ECS_FORCE_INLINE const " + n
		+ "ECSList& getValues() const { return mValues; }\n";
	for (const ECSFieldInfo& field : ecsFields)
	{
		string suffix = makeColumnSuffix(field.mName);
		if (field.mConst)
		{
			o += "\tEASY_ECS_FORCE_INLINE const " + field.mStorageType + "* get" + suffix + "Column() { return mValues.get" + suffix + "Column(); }\n";
			o += "\tEASY_ECS_FORCE_INLINE const " + field.mStorageType + "* get" + suffix + "Column() const { return mValues.get" + suffix + "Column(); }\n";
		}
		else
		{
			o += "\tEASY_ECS_FORCE_INLINE " + field.mStorageType + "* get" + suffix + "Column() { return mValues.get" + suffix + "Column(); }\n";
			o += "\tEASY_ECS_FORCE_INLINE const " + field.mStorageType + "* get" + suffix + "Column() const { return mValues.get" + suffix + "Column(); }\n";
		}
	}
	if (!aosFields.empty())
	{
		if (hasConstAoS) o += "\tEASY_ECS_FORCE_INLINE const " + n + "AoSBlock* getAoSColumn() { return mValues.getAoSColumn(); }\n";
		else o += "\tEASY_ECS_FORCE_INLINE " + n + "AoSBlock* getAoSColumn() { return mValues.getAoSColumn(); }\n";
		o += "\tEASY_ECS_FORCE_INLINE const " + n + "AoSBlock* getAoSColumn() const { return mValues.getAoSColumn(); }\n";
	}
	o += "\tbool add(const TKey& key, const " + n + "& value)\n"
		"\t{\n"
		"\t\tint index = size();\n"
		"\t\tif (!mIndexMap.tryAdd(key, index)) return false;\n"
		"\t\ttry\n"
		"\t\t{\n"
		"\t\t\tmValues.add(value);\n"
		"\t\t}\n"
		"\t\tcatch (...)\n"
		"\t\t{\n"
		"\t\t\tmIndexMap.eraseByIndex(index);\n"
		"\t\t\tthrow;\n"
		"\t\t}\n"
		"\t\treturn true;\n"
		"\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE bool tryAdd(const TKey& key, const " + n + "& value) { return add(key, value); }\n";
	o += "\tint addRange(const TKey* keys, const " + n + "* values, int count)\n"
		"\t{\n"
		"\t\tif (keys == nullptr || values == nullptr || count <= 0) return 0;\n"
		"\t\treserve(size() + count);\n"
		"\t\tint addedCount = 0;\n"
		"\t\tfor (int i = 0; i < count; ++i)\n"
		"\t\t{\n"
		"\t\t\tint index = size();\n"
		"\t\t\tif (!mIndexMap.tryAdd(keys[i], index)) continue;\n"
		"\t\t\ttry\n"
		"\t\t\t{\n"
		"\t\t\t\tmValues.add(values[i]);\n"
		"\t\t\t}\n"
		"\t\t\tcatch (...)\n"
		"\t\t\t{\n"
		"\t\t\t\tmIndexMap.eraseByIndex(index);\n"
		"\t\t\t\tthrow;\n"
		"\t\t\t}\n"
		"\t\t\t++addedCount;\n"
		"\t\t}\n"
		"\t\treturn addedCount;\n"
		"\t}\n";
	o += "\tbool build(const TKey* keys, const " + n + "* values, int count)\n"
		"\t{\n"
		"\t\tif (count < 0 || (count > 0 && (keys == nullptr || values == nullptr))) return false;\n"
		"\t\tclearKeepCapacity();\n"
		"\t\tif (count == 0) return true;\n"
		"\t\tmValues.reserve(count);\n"
		"\t\tif (!mIndexMap.tryBuild(keys, count)) return false;\n"
		"\t\ttry\n"
		"\t\t{\n"
		"\t\t\tmValues.addRange(values, count);\n"
		"\t\t}\n"
		"\t\tcatch (...)\n"
		"\t\t{\n"
		"\t\t\tmIndexMap.clearKeepCapacity();\n"
		"\t\t\tmValues.clearKeepCapacity();\n"
		"\t\t\tthrow;\n"
		"\t\t}\n"
		"\t\treturn true;\n"
		"\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE std::pair<" + n + "Ref, bool> getOrAdd(const TKey& key, const " + n + "& defaultValue)\n"
		"\t{\n"
		"\t\tbool added = false;\n"
		"\t\tint index = mIndexMap.getOrAddIndex(key, size(), added);\n"
		"\t\tif (!added) return { mValues[index], false };\n"
		"\t\ttry\n"
		"\t\t{\n"
		"\t\t\tmValues.add(defaultValue);\n"
		"\t\t}\n"
		"\t\tcatch (...)\n"
		"\t\t{\n"
		"\t\t\tmIndexMap.eraseByIndex(index);\n"
		"\t\t\tthrow;\n"
		"\t\t}\n"
		"\t\treturn { mValues[index], true };\n"
		"\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE std::pair<" + n + "Ref, bool> getOrAdd(const TKey& key) { return getOrAdd(key, " + n + "{}); }\n";
	o += "\tvoid set(const TKey& key, const " + n + "& value)\n"
		"\t{\n"
		"\t\tint* existingIndex = mIndexMap.findIndex(key);\n"
		"\t\tif (existingIndex != nullptr)\n"
		"\t\t{\n"
		"\t\t\tmValues.set(*existingIndex, value);\n"
		"\t\t\treturn;\n"
		"\t\t}\n"
		"\t\tint index = size();\n"
		"\t\tif (!mIndexMap.tryAdd(key, index))\n"
		"\t\t{\n"
		"\t\t\tint* duplicateIndex = mIndexMap.findIndex(key);\n"
		"\t\t\tassert(duplicateIndex != nullptr);\n"
		"\t\t\tmValues.set(*duplicateIndex, value);\n"
		"\t\t\treturn;\n"
		"\t\t}\n"
		"\t\ttry\n"
		"\t\t{\n"
		"\t\t\tmValues.add(value);\n"
		"\t\t}\n"
		"\t\tcatch (...)\n"
		"\t\t{\n"
		"\t\t\tmIndexMap.eraseByIndex(index);\n"
		"\t\t\tthrow;\n"
		"\t\t}\n"
		"\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE bool remove(const TKey& key)\n"
		"\t{\n"
		"\t\tint removeIndex = -1;\n"
		"\t\tif (!mIndexMap.erase(key, removeIndex)) return false;\n"
		"\t\tmValues.removeAtSwapBack(removeIndex);\n"
		"\t\treturn true;\n"
		"\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE bool removeByIndex(int index)\n"
		"\t{\n"
		"\t\tif (!mIndexMap.eraseByIndex(index)) return false;\n"
		"\t\tmValues.removeAtSwapBack(index);\n"
		"\t\treturn true;\n"
		"\t}\n";
	o += "\tEASY_ECS_FORCE_INLINE int removeBatch(const std::vector<TKey>& keys) { return removeBatch(keys.data(), static_cast<int>(keys.size())); }\n";
	o += "\tEASY_ECS_FORCE_INLINE int removeBatch(const TKey* keys, int keyCount)\n"
		"\t{\n"
		"\t\tif (keys == nullptr || keyCount <= 0 || empty()) return 0;\n"
		"\t\tint currentSize = size();\n"
		"\t\tif (keyCount * 10 < currentSize * 7) return removeBatchSmall(keys, keyCount);\n"
		"\t\treturn removeBatchLarge(keys, keyCount, currentSize);\n"
		"\t}\n";
	o += "\tint removeByIndexBatch(const std::vector<int>& indices) { return removeByIndexBatch(indices.data(), static_cast<int>(indices.size())); }\n";
	o += "\tint removeByIndexBatch(const int* indices, int indexCount)\n"
		"\t{\n"
		"\t\tif (indices == nullptr || indexCount <= 0 || empty()) return 0;\n"
		"\t\tint currentSize = size();\n"
		"\t\tif (indexCount * 4 < currentSize)\n"
		"\t\t{\n"
		"\t\t\tstd::vector<int> validIndices;\n"
		"\t\t\tvalidIndices.reserve(static_cast<size_t>(indexCount));\n"
		"\t\t\tfor (int i = 0; i < indexCount; ++i) if (indices[i] >= 0 && indices[i] < currentSize) validIndices.push_back(indices[i]);\n"
		"\t\t\tstd::sort(validIndices.begin(), validIndices.end(), std::greater<int>());\n"
		"\t\t\tvalidIndices.erase(std::unique(validIndices.begin(), validIndices.end()), validIndices.end());\n"
		"\t\t\tfor (int index : validIndices) removeByIndex(index);\n"
		"\t\t\treturn static_cast<int>(validIndices.size());\n"
		"\t\t}\n"
		"\t\tstd::vector<uint8_t> removed(static_cast<size_t>(currentSize), 0);\n"
		"\t\tint removeCount = 0;\n"
		"\t\tfor (int i = 0; i < indexCount; ++i)\n"
		"\t\t{\n"
		"\t\t\tint index = indices[i];\n"
		"\t\t\tif (index < 0 || index >= currentSize || removed[static_cast<size_t>(index)] != 0) continue;\n"
		"\t\t\tremoved[static_cast<size_t>(index)] = 1;\n"
		"\t\t\t++removeCount;\n"
		"\t\t}\n"
		"\t\tif (removeCount == 0) return 0;\n"
		"\t\tif (removeCount == currentSize)\n"
		"\t\t{\n"
		"\t\t\tclear();\n"
		"\t\t\treturn removeCount;\n"
		"\t\t}\n"
		"\t\tif (removeCount * 10 < currentSize * 7)\n"
		"\t\t{\n"
		"\t\t\tfor (int index = currentSize - 1; index >= 0; --index) if (removed[static_cast<size_t>(index)] != 0) removeByIndex(index);\n"
		"\t\t\treturn removeCount;\n"
		"\t\t}\n"
		"\t\tmValues.compactRemoved(removed.data(), currentSize);\n"
		"\t\tmIndexMap.compactRemove(removed.data(), currentSize);\n"
		"\t\treturn removeCount;\n"
		"\t}\n";
	if (hasConst)
	{
		o += "\tbool tryGetValue(const TKey& key, " + n + "& value) const\n"
			"\t{\n"
			"\t\tconst int* index = mIndexMap.findIndex(key);\n"
			"\t\tif (index == nullptr) return false;\n"
			"\t\t" + n + " newValue = mValues.get(*index);\n\t\tvalue.~" + n + "();\n\t\tnew (&value) " + n + "(newValue);\n\t\treturn true;\n\t}\n";
	}
	else
	{
		o += "\tbool tryGetValue(const TKey& key, " + n + "& value) const\n";
		o += "\t{\n";
		o += "\t\tconst int* index = mIndexMap.findIndex(key);\n";
		o += "\t\tif (index == nullptr) return false;\n";
		o += "\t\tvalue = mValues.get(*index);\n";
		o += "\t\treturn true;\n";
		o += "\t}\n";
	}
	o += "\t" + n + " get(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return mValues.get(*index); }\n";
	o += "\tvoid clear() { clearKeepCapacity(); }\n"
		"\tvoid clearKeepCapacity() { mIndexMap.clearKeepCapacity(); mValues.clearKeepCapacity(); }\n"
		"\tvoid clearAndRelease() { mIndexMap.clearAndRelease(); mValues.clearAndRelease(); }\n"
		"\tvoid reserve(int capacity) { if (capacity <= 0) return; mIndexMap.reserve(capacity); mValues.reserve(capacity); }\n"
		"\tvoid shrinkToFit() { mIndexMap.shrinkToFit(); mValues.shrinkToFit(); }\n"
		"private:\n";
	o += "\tEASY_ECS_NO_INLINE int removeBatchSmall(const TKey* keys, int keyCount)\n"
		"\t{\n"
		"\t\tint removedCount = 0;\n"
		"\t\tconst TKey* current = keys;\n"
		"\t\tconst TKey* end = keys + keyCount;\n"
		"\t\tfor (; current != end; ++current) removedCount += remove(*current) ? 1 : 0;\n"
		"\t\treturn removedCount;\n"
		"\t}\n";
	o += "\tEASY_ECS_NO_INLINE int removeBatchLarge(const TKey* keys, int keyCount, int currentSize)\n"
		"\t{\n"
		"\t\tstd::vector<uint8_t> removed(static_cast<size_t>(currentSize), 0);\n"
		"\t\tint removeCount = 0;\n"
		"\t\tfor (int i = 0; i < keyCount; ++i)\n"
		"\t\t{\n"
		"\t\t\tconst int* index = mIndexMap.findIndex(keys[i]);\n"
		"\t\t\tif (index == nullptr || removed[static_cast<size_t>(*index)] != 0) continue;\n"
		"\t\t\tremoved[static_cast<size_t>(*index)] = 1;\n"
		"\t\t\t++removeCount;\n"
		"\t\t}\n"
		"\t\tif (removeCount == 0) return 0;\n"
		"\t\tif (removeCount == currentSize)\n"
		"\t\t{\n"
		"\t\t\tclear();\n"
		"\t\t\treturn removeCount;\n"
		"\t\t}\n"
		"\t\tif (removeCount * 10 < currentSize * 7)\n"
		"\t\t{\n"
		"\t\t\tfor (int index = currentSize - 1; index >= 0; --index) if (removed[static_cast<size_t>(index)] != 0) removeByIndex(index);\n"
		"\t\t\treturn removeCount;\n"
		"\t\t}\n"
		"\t\tmValues.compactRemoved(removed.data(), currentSize);\n"
		"\t\tmIndexMap.compactRemove(removed.data(), currentSize);\n"
		"\t\treturn removeCount;\n"
		"\t}\n";
	o += "\tIndexMap mIndexMap;\n\t" + n + "ECSList mValues;\n};\n";
}
void ECSGenerator::appendStructCpp(string& o, const ECSStructInfo& info, const string&)
{
	const string& n = info.mName;
	vector<ECSFieldInfo> ecsFields;
	vector<ECSFieldInfo> aosFields;
	for (const ECSFieldInfo& field : info.mFields) (field.mNotECS ? aosFields : ecsFields).push_back(field);
	bool hasConst = hasConstField(info);
	o += n + "ECSList::" + n + "ECSList(int capacity)\n{\n";
	for (const ECSFieldInfo& field : ecsFields)
	{
		string name = makeStaticAssertName(info, field);
		appendStaticAssert(o, "std::is_trivially_copyable<" + field.mStorageType + ">::value", "EasyECS field must be trivially copyable: " + name);
		appendStaticAssert(o, "std::is_default_constructible<" + field.mStorageType + ">::value", "EasyECS field must be default constructible: " + name);
		appendStaticAssert(o, "std::is_trivially_destructible<" + field.mStorageType + ">::value", "EasyECS field must be trivially destructible: " + name);
		appendStaticAssert(o, "std::is_trivially_copy_assignable<" + field.mStorageType + ">::value", "EasyECS field must be trivially copy assignable: " + name);
		appendStaticAssert(o, "alignof(" + field.mStorageType + ") <= EASY_ECS_MEMORY_ALIGNMENT",
			"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: " + name);
	}
	if (!aosFields.empty())
	{
		string qualifiedName = getQualifiedName(info);
		appendStaticAssert(o, "std::is_trivially_copyable<" + n + "AoSBlock>::value", "EasyECS NotECS block must be trivially copyable: " + qualifiedName);
		appendStaticAssert(o, "std::is_default_constructible<" + n + "AoSBlock>::value", "EasyECS NotECS block must be default constructible: " + qualifiedName);
		appendStaticAssert(o, "std::is_trivially_destructible<" + n + "AoSBlock>::value", "EasyECS NotECS block must be trivially destructible: " + qualifiedName);
		appendStaticAssert(o, "std::is_trivially_copy_assignable<" + n + "AoSBlock>::value", "EasyECS NotECS block must be trivially copy assignable: "
			+ qualifiedName);
		appendStaticAssert(o, "alignof(" + n + "AoSBlock) <= EASY_ECS_MEMORY_ALIGNMENT",
			"EasyECS NotECS block alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: " + qualifiedName);
	}
	if (hasConst)
	{
		string qualifiedName = getQualifiedName(info);
		appendStaticAssert(o, "std::is_aggregate<" + n + ">::value", "EasyECS source structs containing const fields must be aggregate types: " + qualifiedName);
		appendStaticAssert(o, "std::is_copy_constructible<" + n + ">::value", "EasyECS source structs containing const fields must be copy constructible: "
			+ qualifiedName);
	}
	o += "\tif (capacity < 1) capacity = 4;\n"
		"\tallocateStorage(mStorage, capacity);\n"
		"\tmCapacity = capacity;\n"
		"}\n";
	o += n + "ECSList::" + n + "ECSList(const " + n + "ECSList& other)\n"
		"{\n"
		"\tif (other.mCapacity <= 0) return;\n"
		"\tallocateStorage(mStorage, other.mCapacity);\n"
		"\tcopyStorage(mStorage, other.mStorage, other.mCount);\n"
		"\tmCount = other.mCount;\n"
		"\tmCapacity = other.mCapacity;\n"
		"}\n";
	o += n + "ECSList& " + n + "ECSList::operator=(const " + n + "ECSList& other)\n{\n\tif (this == &other) return *this;\n\t" + n
		+ "ECSList copy(other);\n\tswap(copy);\n\treturn *this;\n}\n";
	o += n + "ECSList::" + n + "ECSList(" + n + "ECSList&& other) noexcept : mStorage(other.mStorage), mCount(other.mCount), mCapacity(other.mCapacity)\n"
		"{\n"
		"\tother.mStorage = {};\n"
		"\tother.mCount = 0;\n"
		"\tother.mCapacity = 0;\n"
		"}\n";
	o += n + "ECSList& " + n + "ECSList::operator=(" + n + "ECSList&& other) noexcept\n"
		"{\n"
		"\tif (this == &other) return *this;\n"
		"\treleaseStorage(mStorage);\n"
		"\tmStorage = other.mStorage;\n"
		"\tmCount = other.mCount;\n"
		"\tmCapacity = other.mCapacity;\n"
		"\tother.mStorage = {};\n"
		"\tother.mCount = 0;\n"
		"\tother.mCapacity = 0;\n"
		"\treturn *this;\n"
		"}\n";
	o += n + "ECSList::~" + n + "ECSList()\n"
		"{\n"
		"\treleaseStorage(mStorage);\n"
		"}\n";
	o += "void " + n + "ECSList::clear()\n"
		"{\n"
		"\tclearKeepCapacity();\n"
		"}\n";
	o += "void " + n + "ECSList::clearKeepCapacity()\n"
		"{\n"
		"\tmCount = 0;\n"
		"}\n";
	o += "void " + n + "ECSList::clearAndRelease()\n"
		"{\n"
		"\treleaseStorage(mStorage);\n"
		"\tmCount = 0;\n"
		"\tmCapacity = 0;\n"
		"}\n";
	o += "void " + n + "ECSList::reserve(int capacity)\n"
		"{\n"
		"\tif (capacity > mCapacity) resizeCapacity(capacity);\n"
		"}\n";
	o += "void " + n + "ECSList::shrinkToFit()\n"
		"{\n"
		"\tint targetCapacity = mCount > 0 ? mCount : 4;\n"
		"\tif (targetCapacity < mCapacity) resizeCapacity(targetCapacity);\n"
		"}\n";
	o += "void " + n + "ECSList::ensureCapacity(int requiredCapacity)\n"
		"{\n"
		"\tif (requiredCapacity <= mCapacity) return;\n"
		"\tint newCapacity = mCapacity > 0 ? mCapacity : 4;\n"
		"\twhile (newCapacity < requiredCapacity) newCapacity *= 2;\n"
		"\tresizeCapacity(newCapacity);\n"
		"}\n";
	o += "void " + n + "ECSList::add(const " + n + "& value)\n"
		"{\n"
		"\tensureCapacity(mCount + 1);\n"
		"\twriteValue(mCount, value);\n"
		"\t++mCount;\n"
		"}\n";
	o += "void " + n + "ECSList::addRange(const " + n + "* values, int count)\n"
		"{\n"
		"\tif (values == nullptr || count <= 0) return;\n"
		"\tconst int startIndex = mCount;\n"
		"\tensureCapacity(startIndex + count);\n"
		"\tfor (int i = 0; i < count; ++i)\n"
		"\t{\n"
		"\t\tconst int targetIndex = startIndex + i;\n"
		"\t\tconst " + n + "& value = values[i];\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\tmStorage." + field.mName + "[targetIndex] = value." + field.mName + ";\n";
	for (const ECSFieldInfo& field : aosFields) o += "\t\tmStorage.mAoS[targetIndex]." + field.mName + " = value." + field.mName + ";\n";
	o += "\t}\n\tmCount += count;\n}\n";
	o += n + "Ref " + n + "ECSList::addDefault()\n{\n\tensureCapacity(mCount + 1);\n\tint index = mCount;\n\t" + n
		+ " value{};\n\twriteValue(index, value);\n\t++mCount;\n\treturn (*this)[index];\n}\n";
	o += "void " + n + "ECSList::insert(int index, const " + n + "& value)\n"
		"{\n"
		"\tassert(index >= 0 && index <= mCount);\n"
		"\tensureCapacity(mCount + 1);\n"
		"\tint moveCount = mCount - index;\n"
		"\tif (moveCount > 0)\n"
		"\t{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\tstd::memmove(mStorage." + field.mName + " + index + 1, mStorage." + field.mName
		+ " + index, sizeof(" + field.mStorageType + ") * moveCount);\n";
	if (!aosFields.empty()) o += "\t\tstd::memmove(mStorage.mAoS + index + 1, mStorage.mAoS + index, sizeof(" + n + "AoSBlock) * moveCount);\n";
	o += "\t}\n\twriteValue(index, value);\n\t++mCount;\n}\n";
	o += "void " + n + "ECSList::removeAt(int index)\n"
		"{\n"
		"\tassert(index >= 0 && index < mCount);\n"
		"\tint moveCount = mCount - index - 1;\n"
		"\tif (moveCount > 0)\n"
		"\t{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\tstd::memmove(mStorage." + field.mName + " + index, mStorage." + field.mName
		+ " + index + 1, sizeof(" + field.mStorageType + ") * moveCount);\n";
	if (!aosFields.empty()) o += "\t\tstd::memmove(mStorage.mAoS + index, mStorage.mAoS + index + 1, sizeof(" + n + "AoSBlock) * moveCount);\n";
	o += "\t}\n\t--mCount;\n}\n";
	o += "void " + n + "ECSList::removeAtSwapBack(int index)\n";
	o += "{\n";
	o += "\tassert(index >= 0 && index < mCount);\n";
	o += "\tint lastIndex = mCount - 1;\n";
	o += "\tif (index != lastIndex) copyValue(index, lastIndex);\n";
	o += "\t--mCount;\n";
	o += "}\n";
	o += "void " + n + "ECSList::popBack()\n"
		"{\n"
		"\tassert(mCount > 0);\n"
		"\t--mCount;\n"
		"}\n";
	if (hasConst)
	{
		o += n + " " + n + "ECSList::get(int index) const\n{\n\tassert(index >= 0 && index < mCount);\n\treturn " + n + "{\n";
		for (size_t i = 0; i < info.mFields.size(); ++i)
		{
			o += "\t\t" + getFieldReadExpression(info.mFields[i]) + (i + 1 == info.mFields.size() ? "\n" : ",\n");
		}
		o += "\t};\n}\n";
	}
	else
	{
		o += n + " " + n + "ECSList::get(int index) const\n{\n\tassert(index >= 0 && index < mCount);\n\t" + n + " value;\n";
		for (const ECSFieldInfo& field : ecsFields) o += "\tvalue." + field.mName + " = mStorage." + field.mName + "[index];\n";
		for (const ECSFieldInfo& field : aosFields) o += "\tvalue." + field.mName + " = mStorage.mAoS[index]." + field.mName + ";\n";
		o += "\treturn value;\n}\n";
	}
	o += "void " + n + "ECSList::set(int index, const " + n + "& value)\n"
		"{\n"
		"\tassert(index >= 0 && index < mCount);\n"
		"\twriteValue(index, value);\n"
		"}\n";
	o += "void " + n + "ECSList::resizeCapacity(int newCapacity)\n{\n\tassert(newCapacity >= mCount);\n\t" + n + "Storage newStorage;\n"
		"\tallocateStorage(newStorage, newCapacity);\n"
		"\tcopyStorage(newStorage, mStorage, mCount);\n"
		"\t" + n + "Storage oldStorage = mStorage;\n"
		"\tmStorage = newStorage;\n"
		"\treleaseStorage(oldStorage);\n"
		"\tmCapacity = newCapacity;\n"
		"}\n";
	o += "void " + n + "ECSList::allocateStorage(" + n + "Storage& storage, int capacity)\n{\n\tsize_t offset = 0;\n";
	for (const ECSFieldInfo& field : ecsFields)
	{
		o += "\toffset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);\n\tsize_t " + field.mName + "Offset = offset;\n\toffset += sizeof("
			+ field.mStorageType + ") * capacity;\n";
	}
	if (!aosFields.empty()) o += "\toffset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);\n"
		"\tsize_t aosOffset = offset;\n"
		"\toffset += sizeof(" + n + "AoSBlock) * capacity;\n";
	o += "\tsize_t allocateSize = offset + EASY_ECS_MEMORY_ALIGNMENT - 1;\n"
		"\tvoid* rawMemory = std::malloc(allocateSize);\n"
		"\tif (rawMemory == nullptr) throw std::bad_alloc();\n"
		"\tuint8_t* alignedMemory = EasyECSRuntime::alignPointer(rawMemory, EASY_ECS_MEMORY_ALIGNMENT);\n"
		"\tstorage.mRawMemory = rawMemory;\n"
		"\tstorage.mAlignedMemory = alignedMemory;\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\tstorage." + field.mName + " = reinterpret_cast<" + field.mStorageType + "*>(alignedMemory + "
		+ field.mName + "Offset);\n";
	if (!aosFields.empty()) o += "\tstorage.mAoS = reinterpret_cast<" + n + "AoSBlock*>(alignedMemory + aosOffset);\n";
	o += "\tfor (int i = 0; i < capacity; ++i)\n\t{\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\t\tnew (storage." + field.mName + " + i) " + field.mStorageType + ";\n";
	if (!aosFields.empty()) o += "\t\tnew (storage.mAoS + i) " + n + "AoSBlock;\n";
	o += "\t}\n}\n";
	o += "void " + n + "ECSList::releaseStorage(" + n + "Storage& storage)\n"
		"{\n"
		"\tif (storage.mRawMemory != nullptr) std::free(storage.mRawMemory);\n"
		"\tstorage = {};\n"
		"}\n";
	o += "void " + n + "ECSList::copyStorage(" + n + "Storage& target, const " + n + "Storage& source, int count)\n{\n\tif (count <= 0) return;\n";
	for (const ECSFieldInfo& field : ecsFields) o += "\tstd::memcpy(target." + field.mName + ", source." + field.mName + ", sizeof("
		+ field.mStorageType + ") * count);\n";
	if (!aosFields.empty()) o += "\tstd::memcpy(target.mAoS, source.mAoS, sizeof(" + n + "AoSBlock) * count);\n";
	o += "}\n";
	o += "void " + n + "ECSList::swap(" + n + "ECSList& other) noexcept\n"
		"{\n"
		"\tusing std::swap;\n"
		"\tswap(mStorage, other.mStorage);\n"
		"\tswap(mCount, other.mCount);\n"
		"\tswap(mCapacity, other.mCapacity);\n"
		"}\n";
}
void ECSGenerator::appendNamespaceBegin(string& output, const ECSStructInfo& info)
{
	string namespaceName = getNamespaceName(info);
	if (!namespaceName.empty()) output += "namespace " + namespaceName + "\n{\n";
}
void ECSGenerator::appendNamespaceEnd(string& output, const ECSStructInfo& info)
{
	if (!info.mNamespaceList.empty()) output += "}\n";
}
string ECSGenerator::getNamespaceName(const ECSStructInfo& info)
{
	string value;
	for (const string& item : info.mNamespaceList)
	{
		if (!value.empty()) value += "::";
		value += item;
	}
	return value;
}
string ECSGenerator::getQualifiedName(const ECSStructInfo& info)
{
	string value = getNamespaceName(info);
	if (!value.empty()) value += "::";
	value += info.mName;
	return value;
}
string ECSGenerator::fileNameOnly(const string& path)
{
	size_t position = path.find_last_of("/\\");
	return position == string::npos ? path : path.substr(position + 1);
}

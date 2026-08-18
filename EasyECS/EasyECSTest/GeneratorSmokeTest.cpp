#include "GeneratorSmokeTest.h"
#include "Data/EasyECS.generated.h"
#include <cstdio>
#include <type_traits>

int runGeneratorSmokeTest()
{
	RoleData data;
	data.mHP = 100;
	data.mSpeed = 2.0f;
	data.mPositionX = 10.0f;
	data.mPositionY = 20.0f;
	data.mID = 1001;
	data.mModelID = 2001;
	data.mCamp = 1;
	RoleDataECSList list;
	list.add(data);
	RoleDataRef role = list[0];
	role.mHP -= 10;
	role.mPositionX += role.mSpeed;
	if (list[0].mHP != 90 || list[0].mPositionX != 12.0f || list[0].mID != 1001)
	{
		std::printf("Generator Smoke Test:FAILED(List Ref)\n");
		return 1;
	}
	RoleDataECSList defaultList;
	RoleDataRef defaultRole = defaultList.addDefault();
	if (defaultList.size() != 1 || defaultRole.mHP != 0 || defaultRole.mSpeed != 0.0f || defaultRole.mPositionX != 0.0f || defaultRole.mPositionY != 0.0f ||
		defaultRole.mID != 0 || defaultRole.mModelID != 0 || defaultRole.mCamp != 0)
	{
		std::printf("Generator Smoke Test:FAILED(List AddDefault)\n");
		return 29;
	}
	defaultRole.mHP = 2468;
	defaultRole.mID = 1357;
	if (defaultList[0].mHP != 2468 || defaultList[0].mID != 1357)
	{
		std::printf("Generator Smoke Test:FAILED(List AddDefault Ref)\n");
		return 30;
	}
	RoleData rangeValues[3];
	for (int i = 0; i < 3; ++i)
	{
		rangeValues[i] = data;
		rangeValues[i].mHP = 200 + i;
		rangeValues[i].mSpeed = 3.0f + static_cast<float>(i);
		rangeValues[i].mID = 7000 + i;
		rangeValues[i].mModelID = 8000 + i;
		rangeValues[i].mCamp = i;
	}
	RoleDataECSList rangeList(1);
	rangeList.add(data);
	rangeList.addRange(rangeValues, 3);
	rangeList.addRange(nullptr, 3);
	rangeList.addRange(rangeValues, 0);
	if (rangeList.size() != 4 || rangeList[0].mID != 1001 || rangeList[1].mHP != 200 || rangeList[1].mID != 7000 || rangeList[2].mSpeed != 4.0f ||
		rangeList[2].mModelID != 8001 || rangeList[3].mHP != 202 || rangeList[3].mCamp != 2)
	{
		std::printf("Generator Smoke Test:FAILED(List AddRange)\n");
		return 32;
	}
	int rangeCapacity = rangeList.capacity();
	int rangeRemoved = rangeList.removeAll([](RoleDataConstRef value) { return (value.mID & 1) == 0; });
	if (rangeRemoved != 2 || rangeList.size() != 2 || rangeList.capacity() != rangeCapacity || rangeList[0].mID != 1001 || rangeList[1].mID != 7001 ||
		rangeList.removeAll([](RoleDataConstRef) { return false; }) != 0)
	{
		std::printf("Generator Smoke Test:FAILED(List RemoveAll)\n");
		return 39;
	}
	if (rangeList.removeAll([](RoleDataConstRef) { return true; }) != 2 || !rangeList.empty() || rangeList.capacity() != rangeCapacity)
	{
		std::printf("Generator Smoke Test:FAILED(List RemoveAll All)\n");
		return 40;
	}
	RoleDataECSDictionary<int> dictionary;
	dictionary.add(1001, data);
	dictionary[1001].mHP = 777;
	if (dictionary[1001].mHP != 777)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary)\n");
		return 2;
	}
	auto dictionaryRef = dictionary.tryGetRef(1001);
	if (!dictionaryRef.has_value() || dictionaryRef->mHP != 777 || dictionary.tryGetRef(9999).has_value())
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary TryGetRef)\n");
		return 12;
	}
	dictionaryRef->mPositionX += 5.0f;
	const RoleDataECSDictionary<int>& constDictionary = dictionary;
	auto constDictionaryRef = constDictionary.tryGetRef(1001);
	if (!constDictionaryRef.has_value() || constDictionaryRef->mPositionX != 15.0f)
	{
		std::printf("Generator Smoke Test:FAILED(Const Dictionary TryGetRef)\n");
		return 13;
	}
	int* dictionaryHP = dictionary.getHPColumn();
	dictionaryHP[0] = 888;
	RoleDataAoSBlock* dictionaryAoS = dictionary.getAoSColumn();
	if (dictionary[1001].mHP != 888 || dictionaryAoS[0].mID != 1001)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Direct Column)\n");
		return 14;
	}
	RoleData extraData = data;
	extraData.mHP = 321;
	if (!dictionary.tryAdd(2001, extraData) || dictionary.tryAdd(2001, data) || dictionary[2001].mHP != 321)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary TryAdd)\n");
		return 15;
	}
	RoleData batchValues[4];
	int batchKeys[4]{ 3001, 3002, 3002, 3003 };
	for (int i = 0; i < 4; ++i)
	{
		batchValues[i] = data;
		batchValues[i].mHP = 400 + i;
		batchValues[i].mID = batchKeys[i];
	}
	RoleDataECSDictionary<int> batchDictionary;
	batchDictionary.add(3000, data);
	int batchAdded = batchDictionary.addRange(batchKeys, batchValues, 4);
	if (batchAdded != 3 || batchDictionary.size() != 4 || batchDictionary[3001].mHP != 400 || batchDictionary[3002].mHP != 401 ||
		batchDictionary[3003].mHP != 403 || batchDictionary.addRange(nullptr, batchValues, 4) != 0 || batchDictionary.addRange(batchKeys, nullptr, 4) != 0)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary AddRange)\n");
		return 34;
	}
	int buildKeys[3]{ 4001, 4002, 4003 };
	RoleData buildValues[3];
	for (int i = 0; i < 3; ++i)
	{
		buildValues[i] = data;
		buildValues[i].mHP = 500 + i;
		buildValues[i].mID = buildKeys[i];
	}
	if (!batchDictionary.build(buildKeys, buildValues, 3) || batchDictionary.size() != 3 || batchDictionary.containsKey(3000) ||
		batchDictionary[4001].mHP != 500 || batchDictionary[4003].mID != 4003)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Build)\n");
		return 35;
	}
	int duplicateBuildKeys[3]{ 5001, 5001, 5002 };
	if (batchDictionary.build(duplicateBuildKeys, buildValues, 3) || !batchDictionary.empty())
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Build Duplicate)\n");
		return 36;
	}
	batchDictionary.add(6001, data);
	if (batchDictionary.build(nullptr, buildValues, 1) || batchDictionary.size() != 1 || !batchDictionary.containsKey(6001) ||
		!batchDictionary.build(nullptr, nullptr, 0) || !batchDictionary.empty())
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Build Invalid)\n");
		return 37;
	}
	int removeAllKeys[6]{ 6100, 6101, 6102, 6103, 6104, 6105 };
	RoleData removeAllValues[6];
	for (int i = 0; i < 6; ++i)
	{
		removeAllValues[i] = data;
		removeAllValues[i].mID = removeAllKeys[i];
		removeAllValues[i].mHP = 700 + i;
	}
	RoleDataECSDictionary<int> removeAllDictionary(32);
	if (!removeAllDictionary.build(removeAllKeys, removeAllValues, 6))
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary RemoveAll Setup)\n");
		return 41;
	}
	int removeAllValueCapacity = removeAllDictionary.capacity();
	int removeAllIndexCapacity = removeAllDictionary.indexCapacity();
	int dictionaryRemoved = removeAllDictionary.removeAll([](const int& key, RoleDataConstRef value) { return (key & 1) == 0 && value.mID == key; });
	bool dictionaryRemoveAllPass = dictionaryRemoved == 3 && removeAllDictionary.size() == 3 && removeAllDictionary.capacity() == removeAllValueCapacity &&
		removeAllDictionary.indexCapacity() == removeAllIndexCapacity;
	for (int i = 0; i < 3 && dictionaryRemoveAllPass; ++i)
	{
		int expectedKey = 6101 + i * 2;
		dictionaryRemoveAllPass = removeAllDictionary.keyAt(i) == expectedKey && removeAllDictionary.getIndex(expectedKey) == i &&
			removeAllDictionary.valueAt(i).mID == expectedKey;
	}
	dictionaryRemoveAllPass = dictionaryRemoveAllPass && removeAllDictionary.removeAll([](const int&, RoleDataConstRef) { return false; }) == 0;
	if (!dictionaryRemoveAllPass)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary RemoveAll)\n");
		return 42;
	}
	if (removeAllDictionary.removeAll([](const int&, RoleDataConstRef) { return true; }) != 3 || !removeAllDictionary.empty() ||
		removeAllDictionary.capacity() != removeAllValueCapacity || removeAllDictionary.indexCapacity() != removeAllIndexCapacity)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary RemoveAll All)\n");
		return 43;
	}
	auto existingResult = dictionary.getOrAdd(2001, data);
	if (existingResult.second || existingResult.first.mHP != 321)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary GetOrAdd Existing)\n");
		return 16;
	}
	auto newResult = dictionary.getOrAdd(2002, data);
	newResult.first.mHP = 654;
	if (!newResult.second || dictionary[2002].mHP != 654)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary GetOrAdd New)\n");
		return 17;
	}
	auto defaultResult = dictionary.getOrAdd(2003);
	if (!defaultResult.second || dictionary[2003].mHP != 0)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary GetOrAdd Default)\n");
		return 18;
	}
	int forEachCount = 0;
	dictionary.forEach([&](const int&, RoleDataRef value)
	{
		value.mHP += 1;
		++forEachCount;
	});
	int constForEachCount = 0;
	long long constForEachHPSum = 0;
	constDictionary.forEach([&](const int&, RoleDataConstRef value)
	{
		constForEachHPSum += value.mHP;
		++constForEachCount;
	});
	if (forEachCount != dictionary.size() || constForEachCount != dictionary.size() || dictionary[1001].mHP != 889 || dictionary[2001].mHP != 322 ||
		dictionary[2002].mHP != 655 || dictionary[2003].mHP != 1 || constForEachHPSum != 1867)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary ForEach)\n");
		return 19;
	}
	RoleDataECSList capacityList(64);
	for (int i = 0; i < 8; ++i) capacityList.add(data);
	capacityList.shrinkToFit();
	if (capacityList.capacity() != 8 || capacityList.size() != 8 || capacityList[0].mHP != 100)
	{
		std::printf("Generator Smoke Test:FAILED(List Capacity)\n");
		return 20;
	}
	RoleDataECSDictionary<int> capacityDictionary(64);
	for (int i = 0; i < 8; ++i) capacityDictionary.add(3000 + i, data);
	size_t indexMemoryBeforeShrink = capacityDictionary.indexMemoryUsageBytes();
	capacityDictionary.shrinkToFit();
	if (capacityDictionary.capacity() != 8 || capacityDictionary.indexCapacity() != 16 ||
		capacityDictionary.indexMemoryUsageBytes() >= indexMemoryBeforeShrink || capacityDictionary.keyAt(0) != capacityDictionary.getKeyByIndex(0) ||
		capacityDictionary.valueAt(0).mHP != capacityDictionary.getValueByIndex(0).mHP)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Capacity)\n");
		return 21;
	}
	capacityDictionary.reserve(100);
	if (capacityDictionary.capacity() < 100 || capacityDictionary.indexCapacity() < 134 || capacityDictionary.size() != 8)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Reserve)\n");
		return 22;
	}
	RoleDataECSList clearList(64);
	for (int i = 0; i < 8; ++i) clearList.add(data);
	int clearListCapacity = clearList.capacity();
	clearList.clearKeepCapacity();
	if (!clearList.empty() || clearList.capacity() != clearListCapacity)
	{
		std::printf("Generator Smoke Test:FAILED(List ClearKeepCapacity)\n");
		return 23;
	}
	clearList.add(data);
	clearList.clearAndRelease();
	if (!clearList.empty() || clearList.capacity() != 0)
	{
		std::printf("Generator Smoke Test:FAILED(List ClearAndRelease)\n");
		return 24;
	}
	clearList.add(data);
	if (clearList.size() != 1 || clearList.capacity() < 4 || clearList[0].mHP != 100)
	{
		std::printf("Generator Smoke Test:FAILED(List Reuse After Release)\n");
		return 25;
	}
	RoleDataECSDictionary<int> clearDictionary(64);
	for (int i = 0; i < 8; ++i) clearDictionary.add(4000 + i, data);
	int clearValueCapacity = clearDictionary.capacity();
	int clearIndexCapacity = clearDictionary.indexCapacity();
	size_t clearIndexMemory = clearDictionary.indexMemoryUsageBytes();
	clearDictionary.clear();
	if (!clearDictionary.empty() || clearDictionary.capacity() != clearValueCapacity || clearDictionary.indexCapacity() != clearIndexCapacity ||
		clearDictionary.indexMemoryUsageBytes() != clearIndexMemory)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary ClearKeepCapacity)\n");
		return 26;
	}
	clearDictionary.add(5000, data);
	clearDictionary.clearAndRelease();
	if (!clearDictionary.empty() || clearDictionary.capacity() != 0 || clearDictionary.indexCapacity() != 0 || clearDictionary.indexMemoryUsageBytes() != 0)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary ClearAndRelease)\n");
		return 27;
	}
	clearDictionary.add(6000, data);
	if (clearDictionary.size() != 1 || clearDictionary[6000].mHP != 100 || clearDictionary.capacity() < 4 || clearDictionary.indexCapacity() < 8)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Reuse After Release)\n");
		return 28;
	}
	int* hp = list.getHPColumn();
	hp[0] = 999;
	if (list[0].mHP != 999)
	{
		std::printf("Generator Smoke Test:FAILED(Direct)\n");
		return 3;
	}
	EasyECSDemo::MonsterData monster;
	monster.mHP = 500;
	monster.mMoveSpeed = 3.5f;
	monster.mID = 3001;
	EasyECSDemo::MonsterDataECSList monsterList;
	monsterList.add(monster);
	monsterList[0].mHP -= 50;
	if (monsterList[0].mHP != 450 || monsterList[0].mID != 3001)
	{
		std::printf("Generator Smoke Test:FAILED(Namespace)\n");
		return 4;
	}
	EasyECSDemo::NPCData npc;
	npc.mHP = 300;
	npc.mTalkDistance = 6.0f;
	npc.mID = 3501;
	EasyECSDemo::NPCDataECSList npcList;
	npcList.add(npc);
	npcList[0].mTalkDistance += 2.0f;
	if (npcList[0].mTalkDistance != 8.0f || npcList[0].mID != 3501)
	{
		std::printf("Generator Smoke Test:FAILED(Multi Struct Same Header)\n");
		return 5;
	}
	EasyECSDemo::ItemData item;
	item.mCount = 10;
	item.mWeight = 2.5f;
	item.mID = 4001;
	EasyECSDemo::ItemDataECSDictionary<int> itemDictionary;
	itemDictionary.add(4001, item);
	itemDictionary[4001].mCount += 5;
	if (itemDictionary[4001].mCount != 15)
	{
		std::printf("Generator Smoke Test:FAILED(Multi Struct)\n");
		return 6;
	}
	EasyECSDemo::Battle::BulletData bullet;
	bullet.mPositionX = 10.0f;
	bullet.mPositionY = 20.0f;
	bullet.mOwnerID = 5001;
	EasyECSDemo::Battle::BulletDataECSList bulletList;
	bulletList.add(bullet);
	float* positionX = bulletList.getPositionXColumn();
	positionX[0] += 5.0f;
	if (bulletList[0].mPositionX != 15.0f || bulletList[0].mOwnerID != 5001)
	{
		std::printf("Generator Smoke Test:FAILED(Nested Namespace)\n");
		return 7;
	}
	EasyECSDemo::TypeDataECSList defaultTypeList;
	auto defaultTypeRef = defaultTypeList.addDefault();
	if (defaultTypeRef.mUnsigned != 1 || defaultTypeRef.mLongLong != 2 || defaultTypeRef.mUInt32 != 3 || defaultTypeRef.mInt64 != -4 ||
		defaultTypeRef.mState != EasyECSDemo::TypeDataState::Idle || defaultTypeRef.mAliasUInt != 5 || defaultTypeRef.mAliasInt64 != -6 ||
		defaultTypeRef.mMoveState != EasyECSDemo::TypeSupport::MoveState::Idle || defaultTypeRef.mAttributeValue != 9 || defaultTypeRef.mImmutable != 777 ||
		defaultTypeRef.mTailConst != 888 || defaultTypeRef.mID != 0)
	{
		std::printf("Generator Smoke Test:FAILED(List AddDefault Source Defaults)\n");
		return 31;
	}
	EasyECSDemo::TypeData rangeTypeValues[2];
	rangeTypeValues[0].mUnsigned = 101;
	rangeTypeValues[0].mID = 9101;
	rangeTypeValues[1].mUnsigned = 202;
	rangeTypeValues[1].mID = 9102;
	EasyECSDemo::TypeDataECSList rangeTypeList(1);
	rangeTypeList.addRange(rangeTypeValues, 2);
	if (rangeTypeList.size() != 2 || rangeTypeList[0].mUnsigned != 101 || rangeTypeList[0].mImmutable != 777 || rangeTypeList[0].mTailConst != 888 ||
		rangeTypeList[0].mID != 9101 || rangeTypeList[1].mUnsigned != 202 || rangeTypeList[1].mImmutable != 777 || rangeTypeList[1].mID != 9102)
	{
		std::printf("Generator Smoke Test:FAILED(List AddRange Common Types)\n");
		return 33;
	}
	EasyECSDemo::TypeData removeTypeValues[3];
	for (int i = 0; i < 3; ++i)
	{
		removeTypeValues[i].mUnsigned = 300 + static_cast<unsigned>(i);
		removeTypeValues[i].mID = 9201 + static_cast<uint64_t>(i);
	}
	EasyECSDemo::TypeDataECSList removeTypeList;
	removeTypeList.addRange(removeTypeValues, 3);
	int typeRemoved = removeTypeList.removeAll([](EasyECSDemo::TypeDataConstRef value) { return value.mID == 9201; });
	if (typeRemoved != 1 || removeTypeList.size() != 2 || removeTypeList[0].mID != 9202 || removeTypeList[0].mUnsigned != 301 ||
		removeTypeList[0].mImmutable != 777 || removeTypeList[0].mTailConst != 888 || removeTypeList[1].mID != 9203 || removeTypeList[1].mUnsigned != 302)
	{
		std::printf("Generator Smoke Test:FAILED(List RemoveAll Common Types)\n");
		return 44;
	}
	EasyECSDemo::TypeData typeData;
	typeData.mUnsigned = 11;
	typeData.mLongLong = 22;
	typeData.mUInt32 = 33;
	typeData.mInt64 = -44;
	typeData.mState = EasyECSDemo::TypeDataState::Running;
	typeData.mPosition = { 5.0f, 6.0f };
	typeData.mAliasUInt = 55;
	typeData.mAliasInt64 = -66;
	typeData.mMoveState = EasyECSDemo::TypeSupport::MoveState::Moving;
	typeData.mPosition3 = { 7.0f, 8.0f, 9.0f };
	typeData.mFixedValues = { 10, 20, 30, 40 };
	typeData.mAttributeValue = 99;
	typeData.mID = 9001;
	EasyECSDemo::TypeDataECSList typeList;
	typeList.add(typeData);
	auto typeRef = typeList[0];
	typeRef.mUInt32 += 10;
	typeRef.mPosition.mX += 2.0f;
	if (typeRef.mUInt32 != 43 || typeRef.mLongLong != 22 || typeRef.mInt64 != -44 || typeRef.mState != EasyECSDemo::TypeDataState::Running ||
		typeRef.mPosition.mX != 7.0f || typeRef.mAliasUInt != 55 || typeRef.mAliasInt64 != -66 ||
		typeRef.mMoveState != EasyECSDemo::TypeSupport::MoveState::Moving || typeRef.mPosition3.mZ != 9.0f || typeRef.mFixedValues[2] != 30 ||
		typeRef.mAttributeValue != 99 || typeRef.mImmutable != 777 || typeRef.mTailConst != 888 || typeRef.mID != 9001)
	{
		std::printf("Generator Smoke Test:FAILED(Common Types)\n");
		return 8;
	}
	EasyECSDemo::TypeData copied = typeList.get(0);
	if (copied.mUInt32 != 43 || copied.mImmutable != 777 || copied.mTailConst != 888 || copied.mID != 9001)
	{
		std::printf("Generator Smoke Test:FAILED(Const Get)\n");
		return 9;
	}
	EasyECSDemo::TypeDataECSDictionary<int> typeDictionary;
	int typeDictionaryKey = 9001;
	if (!typeDictionary.build(&typeDictionaryKey, &typeData, 1))
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Build Common Types)\n");
		return 38;
	}
	EasyECSDemo::TypeData outputValue;
	if (!typeDictionary.tryGetValue(9001, outputValue) || outputValue.mID != 9001 || outputValue.mImmutable != 777 || outputValue.mTailConst != 888)
	{
		std::printf("Generator Smoke Test:FAILED(Const Dictionary TryGetValue)\n");
		return 10;
	}
	int typeIndex = -1;
	if (!typeDictionary.tryGetIndex(9001, typeIndex) || typeIndex != 0 || typeDictionary.getIndex(9001) != 0)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Index)\n");
		return 11;
	}
	EasyECSDemo::TypeDataECSList typeListCopy(typeList);
	bool listCopyMovePass = typeListCopy.size() == 1 && typeListCopy.capacity() == typeList.capacity() && typeListCopy[0].mUInt32 == 43 &&
		typeListCopy[0].mImmutable == 777 && typeListCopy[0].mTailConst == 888 && typeListCopy[0].mID == 9001;
	typeListCopy[0].mUInt32 = 1234;
	listCopyMovePass = listCopyMovePass && typeList[0].mUInt32 == 43 && typeListCopy[0].mUInt32 == 1234;
	EasyECSDemo::TypeDataECSList typeListAssigned;
	typeListAssigned = typeList;
	typeListAssigned = typeListAssigned;
	listCopyMovePass = listCopyMovePass && typeListAssigned.size() == 1 && typeListAssigned[0].mPosition3.mZ == 9.0f && typeListAssigned[0].mFixedValues[2] == 30;
	int movedListCapacity = typeListAssigned.capacity();
	EasyECSDemo::TypeDataECSList typeListMoved(std::move(typeListAssigned));
	listCopyMovePass = listCopyMovePass && typeListMoved.size() == 1 && typeListMoved.capacity() == movedListCapacity && typeListAssigned.empty() &&
		typeListAssigned.capacity() == 0;
	typeListAssigned.add(typeData);
	listCopyMovePass = listCopyMovePass && typeListAssigned.size() == 1 && typeListAssigned[0].mID == 9001;
	EasyECSDemo::TypeDataECSList typeListMoveAssigned;
	typeListMoveAssigned.add(typeData);
	typeListMoveAssigned = std::move(typeListMoved);
	typeListMoveAssigned = std::move(typeListMoveAssigned);
	listCopyMovePass = listCopyMovePass && typeListMoveAssigned.size() == 1 && typeListMoveAssigned[0].mUInt32 == 43 && typeListMoved.empty() &&
		typeListMoved.capacity() == 0;
	typeListMoved.add(typeData);
	listCopyMovePass = listCopyMovePass && typeListMoved.size() == 1;
	if (!listCopyMovePass)
	{
		std::printf("Generator Smoke Test:FAILED(List Copy Move)\n");
		return 45;
	}
	EasyECSDemo::TypeDataECSDictionary<int> typeDictionaryCopy(typeDictionary);
	bool dictionaryCopyMovePass = typeDictionaryCopy.size() == 1 && typeDictionaryCopy.capacity() == typeDictionary.capacity() &&
		typeDictionaryCopy.indexCapacity() == typeDictionary.indexCapacity() && typeDictionaryCopy[9001].mImmutable == 777 &&
		typeDictionaryCopy[9001].mID == 9001;
	typeDictionaryCopy[9001].mUInt32 = 2222;
	dictionaryCopyMovePass = dictionaryCopyMovePass && typeDictionary[9001].mUInt32 == 33 && typeDictionaryCopy[9001].mUInt32 == 2222;
	EasyECSDemo::TypeDataECSDictionary<int> typeDictionaryAssigned;
	typeDictionaryAssigned = typeDictionary;
	typeDictionaryAssigned = typeDictionaryAssigned;
	dictionaryCopyMovePass = dictionaryCopyMovePass && typeDictionaryAssigned.size() == 1 && typeDictionaryAssigned[9001].mTailConst == 888;
	int movedDictionaryValueCapacity = typeDictionaryAssigned.capacity();
	int movedDictionaryIndexCapacity = typeDictionaryAssigned.indexCapacity();
	EasyECSDemo::TypeDataECSDictionary<int> typeDictionaryMoved(std::move(typeDictionaryAssigned));
	dictionaryCopyMovePass = dictionaryCopyMovePass && typeDictionaryMoved.size() == 1 && typeDictionaryMoved.capacity() == movedDictionaryValueCapacity &&
		typeDictionaryMoved.indexCapacity() == movedDictionaryIndexCapacity && typeDictionaryAssigned.empty() && typeDictionaryAssigned.capacity() == 0 &&
		typeDictionaryAssigned.indexCapacity() == 0;
	typeDictionaryAssigned.add(9002, typeData);
	dictionaryCopyMovePass = dictionaryCopyMovePass && typeDictionaryAssigned.size() == 1 && typeDictionaryAssigned[9002].mID == 9001;
	EasyECSDemo::TypeDataECSDictionary<int> typeDictionaryMoveAssigned;
	typeDictionaryMoveAssigned.add(1, typeData);
	typeDictionaryMoveAssigned = std::move(typeDictionaryMoved);
	typeDictionaryMoveAssigned = std::move(typeDictionaryMoveAssigned);
	dictionaryCopyMovePass = dictionaryCopyMovePass && typeDictionaryMoveAssigned.size() == 1 && typeDictionaryMoveAssigned[9001].mUInt32 == 33 &&
		typeDictionaryMoved.empty() && typeDictionaryMoved.capacity() == 0 && typeDictionaryMoved.indexCapacity() == 0;
	typeDictionaryMoved.add(9003, typeData);
	dictionaryCopyMovePass = dictionaryCopyMovePass && typeDictionaryMoved.size() == 1 && typeDictionaryMoved.containsKey(9003);
	if (!dictionaryCopyMovePass)
	{
		std::printf("Generator Smoke Test:FAILED(Dictionary Copy Move)\n");
		return 46;
	}
	static_assert(std::is_same<EasyECSDemo::MonsterDataECSList::SourceType, EasyECSDemo::MonsterData>::value, "SourceType error");
	static_assert(std::is_same<EasyECSDemo::Battle::BulletDataECSList::SourceType, EasyECSDemo::Battle::BulletData>::value, "SourceType error");
	static_assert(std::is_same<decltype(typeRef.mImmutable), const uint32_t&>::value, "Const ECS field must remain const through Ref");
	static_assert(std::is_copy_constructible<RoleDataECSList>::value && std::is_copy_assignable<RoleDataECSList>::value &&
		std::is_move_constructible<RoleDataECSList>::value && std::is_move_assignable<RoleDataECSList>::value, "EasyECS List copy/move support error");
	static_assert(std::is_copy_constructible<RoleDataECSDictionary<int>>::value && std::is_copy_assignable<RoleDataECSDictionary<int>>::value &&
		std::is_move_constructible<RoleDataECSDictionary<int>>::value &&
		std::is_move_assignable<RoleDataECSDictionary<int>>::value, "EasyECS Dictionary copy/move support error");
	std::printf("Generator Smoke Test:PASS\n");
	std::printf("Namespace Test:PASS\n");
	std::printf("Multi Struct Test:PASS\n");
	std::printf("Multi Header Scan Test:PASS\n");
	std::printf("Common Type Test:PASS\n");
	std::printf("Const Field Test:PASS\n");
	std::printf("Dictionary Index Test:PASS\n");
	std::printf("Dictionary TryGetRef Test:PASS\n");
	std::printf("Dictionary Direct Column Test:PASS\n");
	std::printf("Dictionary TryAdd Test:PASS\n");
	std::printf("Dictionary AddRange Test:PASS\n");
	std::printf("Dictionary Build Test:PASS\n");
	std::printf("Dictionary GetOrAdd Test:PASS\n");
	std::printf("Dictionary ForEach Test:PASS\n");
	std::printf("Dictionary RemoveAll Test:PASS\n");
	std::printf("List Copy Move Test:PASS\n");
	std::printf("Dictionary Copy Move Test:PASS\n");
	std::printf("List AddDefault Test:PASS\n");
	std::printf("List AddRange Test:PASS\n");
	std::printf("List RemoveAll Test:PASS\n");
	std::printf("List Capacity Test:PASS\n");
	std::printf("Dictionary Capacity Test:PASS\n");
	std::printf("List Clear API Test:PASS\n");
	std::printf("Dictionary Clear API Test:PASS\n");
	std::printf("Alias Type Test:PASS\n");
	std::printf("Namespace Type Test:PASS\n");
	std::printf("std::array Type Test:PASS\n");
	std::printf("Standard Attribute Test:PASS\n");
	return 0;
}

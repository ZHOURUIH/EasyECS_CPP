#include "CharacterData.easyecs.generated.h"
#include <cstdlib>
#include <cstring>
#include <new>

namespace EasyECSDemo
{
MonsterDataECSList::MonsterDataECSList(int capacity)
{
	static_assert(
		std::is_trivially_copyable<int>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::MonsterData::mHP line 11");
	static_assert(
		std::is_default_constructible<int>::value,
		"EasyECS field must be default constructible: EasyECSDemo::MonsterData::mHP line 11");
	static_assert(
		std::is_trivially_destructible<int>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::MonsterData::mHP line 11");
	static_assert(
		std::is_trivially_copy_assignable<int>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::MonsterData::mHP line 11");
	static_assert(
		alignof(int) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::MonsterData::mHP line 11");
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::MonsterData::mMoveSpeed line 12");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: EasyECSDemo::MonsterData::mMoveSpeed line 12");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::MonsterData::mMoveSpeed line 12");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::MonsterData::mMoveSpeed line 12");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::MonsterData::mMoveSpeed line 12");
	static_assert(
		std::is_trivially_copyable<MonsterDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copyable: EasyECSDemo::MonsterData");
	static_assert(
		std::is_default_constructible<MonsterDataAoSBlock>::value,
		"EasyECS NotECS block must be default constructible: EasyECSDemo::MonsterData");
	static_assert(
		std::is_trivially_destructible<MonsterDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially destructible: EasyECSDemo::MonsterData");
	static_assert(
		std::is_trivially_copy_assignable<MonsterDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copy assignable: EasyECSDemo::MonsterData");
	static_assert(
		alignof(MonsterDataAoSBlock) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS NotECS block alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::MonsterData");
	if (capacity < 1) capacity = 4;
	allocateStorage(mStorage, capacity);
	mCapacity = capacity;
}
MonsterDataECSList::MonsterDataECSList(const MonsterDataECSList& other)
{
	if (other.mCapacity <= 0) return;
	allocateStorage(mStorage, other.mCapacity);
	copyStorage(mStorage, other.mStorage, other.mCount);
	mCount = other.mCount;
	mCapacity = other.mCapacity;
}
MonsterDataECSList& MonsterDataECSList::operator=(const MonsterDataECSList& other)
{
	if (this == &other) return *this;
	MonsterDataECSList copy(other);
	swap(copy);
	return *this;
}
MonsterDataECSList::MonsterDataECSList(MonsterDataECSList&& other) noexcept : mStorage(other.mStorage), mCount(other.mCount), mCapacity(other.mCapacity)
{
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
}
MonsterDataECSList& MonsterDataECSList::operator=(MonsterDataECSList&& other) noexcept
{
	if (this == &other) return *this;
	releaseStorage(mStorage);
	mStorage = other.mStorage;
	mCount = other.mCount;
	mCapacity = other.mCapacity;
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
	return *this;
}
MonsterDataECSList::~MonsterDataECSList()
{
	releaseStorage(mStorage);
}
void MonsterDataECSList::clear()
{
	clearKeepCapacity();
}
void MonsterDataECSList::clearKeepCapacity()
{
	mCount = 0;
}
void MonsterDataECSList::clearAndRelease()
{
	releaseStorage(mStorage);
	mCount = 0;
	mCapacity = 0;
}
void MonsterDataECSList::reserve(int capacity)
{
	if (capacity > mCapacity) resizeCapacity(capacity);
}
void MonsterDataECSList::shrinkToFit()
{
	int targetCapacity = mCount > 0 ? mCount : 4;
	if (targetCapacity < mCapacity) resizeCapacity(targetCapacity);
}
void MonsterDataECSList::ensureCapacity(int requiredCapacity)
{
	if (requiredCapacity <= mCapacity) return;
	int newCapacity = mCapacity > 0 ? mCapacity : 4;
	while (newCapacity < requiredCapacity) newCapacity *= 2;
	resizeCapacity(newCapacity);
}
void MonsterDataECSList::add(const MonsterData& value)
{
	ensureCapacity(mCount + 1);
	writeValue(mCount, value);
	++mCount;
}
void MonsterDataECSList::addRange(const MonsterData* values, int count)
{
	if (values == nullptr || count <= 0) return;
	const int startIndex = mCount;
	ensureCapacity(startIndex + count);
	for (int i = 0; i < count; ++i)
	{
		const int targetIndex = startIndex + i;
		const MonsterData& value = values[i];
		mStorage.mHP[targetIndex] = value.mHP;
		mStorage.mMoveSpeed[targetIndex] = value.mMoveSpeed;
		mStorage.mAoS[targetIndex].mID = value.mID;
	}
	mCount += count;
}
MonsterDataRef MonsterDataECSList::addDefault()
{
	ensureCapacity(mCount + 1);
	int index = mCount;
	MonsterData value{};
	writeValue(index, value);
	++mCount;
	return (*this)[index];
}
void MonsterDataECSList::insert(int index, const MonsterData& value)
{
	assert(index >= 0 && index <= mCount);
	ensureCapacity(mCount + 1);
	int moveCount = mCount - index;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mHP + index + 1, mStorage.mHP + index, sizeof(int) * moveCount);
		std::memmove(mStorage.mMoveSpeed + index + 1, mStorage.mMoveSpeed + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index + 1, mStorage.mAoS + index, sizeof(MonsterDataAoSBlock) * moveCount);
	}
	writeValue(index, value);
	++mCount;
}
void MonsterDataECSList::removeAt(int index)
{
	assert(index >= 0 && index < mCount);
	int moveCount = mCount - index - 1;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mHP + index, mStorage.mHP + index + 1, sizeof(int) * moveCount);
		std::memmove(mStorage.mMoveSpeed + index, mStorage.mMoveSpeed + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index, mStorage.mAoS + index + 1, sizeof(MonsterDataAoSBlock) * moveCount);
	}
	--mCount;
}
void MonsterDataECSList::removeAtSwapBack(int index)
{
	assert(index >= 0 && index < mCount);
	int lastIndex = mCount - 1;
	if (index != lastIndex) copyValue(index, lastIndex);
	--mCount;
}
void MonsterDataECSList::popBack()
{
	assert(mCount > 0);
	--mCount;
}
MonsterData MonsterDataECSList::get(int index) const
{
	assert(index >= 0 && index < mCount);
	MonsterData value;
	value.mHP = mStorage.mHP[index];
	value.mMoveSpeed = mStorage.mMoveSpeed[index];
	value.mID = mStorage.mAoS[index].mID;
	return value;
}
void MonsterDataECSList::set(int index, const MonsterData& value)
{
	assert(index >= 0 && index < mCount);
	writeValue(index, value);
}
void MonsterDataECSList::resizeCapacity(int newCapacity)
{
	assert(newCapacity >= mCount);
	MonsterDataStorage newStorage;
	allocateStorage(newStorage, newCapacity);
	copyStorage(newStorage, mStorage, mCount);
	MonsterDataStorage oldStorage = mStorage;
	mStorage = newStorage;
	releaseStorage(oldStorage);
	mCapacity = newCapacity;
}
void MonsterDataECSList::allocateStorage(MonsterDataStorage& storage, int capacity)
{
	size_t offset = 0;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mHPOffset = offset;
	offset += sizeof(int) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mMoveSpeedOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t aosOffset = offset;
	offset += sizeof(MonsterDataAoSBlock) * capacity;
	size_t allocateSize = offset + EASY_ECS_MEMORY_ALIGNMENT - 1;
	void* rawMemory = std::malloc(allocateSize);
	if (rawMemory == nullptr) throw std::bad_alloc();
	uint8_t* alignedMemory = EasyECSRuntime::alignPointer(rawMemory, EASY_ECS_MEMORY_ALIGNMENT);
	storage.mRawMemory = rawMemory;
	storage.mAlignedMemory = alignedMemory;
	storage.mHP = reinterpret_cast<int*>(alignedMemory + mHPOffset);
	storage.mMoveSpeed = reinterpret_cast<float*>(alignedMemory + mMoveSpeedOffset);
	storage.mAoS = reinterpret_cast<MonsterDataAoSBlock*>(alignedMemory + aosOffset);
	for (int i = 0; i < capacity; ++i)
	{
		new (storage.mHP + i) int;
		new (storage.mMoveSpeed + i) float;
		new (storage.mAoS + i) MonsterDataAoSBlock;
	}
}
void MonsterDataECSList::releaseStorage(MonsterDataStorage& storage)
{
	if (storage.mRawMemory != nullptr) std::free(storage.mRawMemory);
	storage = {};
}
void MonsterDataECSList::copyStorage(MonsterDataStorage& target, const MonsterDataStorage& source, int count)
{
	if (count <= 0) return;
	std::memcpy(target.mHP, source.mHP, sizeof(int) * count);
	std::memcpy(target.mMoveSpeed, source.mMoveSpeed, sizeof(float) * count);
	std::memcpy(target.mAoS, source.mAoS, sizeof(MonsterDataAoSBlock) * count);
}
void MonsterDataECSList::swap(MonsterDataECSList& other) noexcept
{
	using std::swap;
	swap(mStorage, other.mStorage);
	swap(mCount, other.mCount);
	swap(mCapacity, other.mCapacity);
}
}

namespace EasyECSDemo
{
NPCDataECSList::NPCDataECSList(int capacity)
{
	static_assert(
		std::is_trivially_copyable<int>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::NPCData::mHP line 18");
	static_assert(
		std::is_default_constructible<int>::value,
		"EasyECS field must be default constructible: EasyECSDemo::NPCData::mHP line 18");
	static_assert(
		std::is_trivially_destructible<int>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::NPCData::mHP line 18");
	static_assert(
		std::is_trivially_copy_assignable<int>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::NPCData::mHP line 18");
	static_assert(
		alignof(int) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::NPCData::mHP line 18");
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::NPCData::mTalkDistance line 19");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: EasyECSDemo::NPCData::mTalkDistance line 19");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::NPCData::mTalkDistance line 19");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::NPCData::mTalkDistance line 19");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::NPCData::mTalkDistance line 19");
	static_assert(
		std::is_trivially_copyable<NPCDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copyable: EasyECSDemo::NPCData");
	static_assert(
		std::is_default_constructible<NPCDataAoSBlock>::value,
		"EasyECS NotECS block must be default constructible: EasyECSDemo::NPCData");
	static_assert(
		std::is_trivially_destructible<NPCDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially destructible: EasyECSDemo::NPCData");
	static_assert(
		std::is_trivially_copy_assignable<NPCDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copy assignable: EasyECSDemo::NPCData");
	static_assert(
		alignof(NPCDataAoSBlock) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS NotECS block alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::NPCData");
	if (capacity < 1) capacity = 4;
	allocateStorage(mStorage, capacity);
	mCapacity = capacity;
}
NPCDataECSList::NPCDataECSList(const NPCDataECSList& other)
{
	if (other.mCapacity <= 0) return;
	allocateStorage(mStorage, other.mCapacity);
	copyStorage(mStorage, other.mStorage, other.mCount);
	mCount = other.mCount;
	mCapacity = other.mCapacity;
}
NPCDataECSList& NPCDataECSList::operator=(const NPCDataECSList& other)
{
	if (this == &other) return *this;
	NPCDataECSList copy(other);
	swap(copy);
	return *this;
}
NPCDataECSList::NPCDataECSList(NPCDataECSList&& other) noexcept : mStorage(other.mStorage), mCount(other.mCount), mCapacity(other.mCapacity)
{
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
}
NPCDataECSList& NPCDataECSList::operator=(NPCDataECSList&& other) noexcept
{
	if (this == &other) return *this;
	releaseStorage(mStorage);
	mStorage = other.mStorage;
	mCount = other.mCount;
	mCapacity = other.mCapacity;
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
	return *this;
}
NPCDataECSList::~NPCDataECSList()
{
	releaseStorage(mStorage);
}
void NPCDataECSList::clear()
{
	clearKeepCapacity();
}
void NPCDataECSList::clearKeepCapacity()
{
	mCount = 0;
}
void NPCDataECSList::clearAndRelease()
{
	releaseStorage(mStorage);
	mCount = 0;
	mCapacity = 0;
}
void NPCDataECSList::reserve(int capacity)
{
	if (capacity > mCapacity) resizeCapacity(capacity);
}
void NPCDataECSList::shrinkToFit()
{
	int targetCapacity = mCount > 0 ? mCount : 4;
	if (targetCapacity < mCapacity) resizeCapacity(targetCapacity);
}
void NPCDataECSList::ensureCapacity(int requiredCapacity)
{
	if (requiredCapacity <= mCapacity) return;
	int newCapacity = mCapacity > 0 ? mCapacity : 4;
	while (newCapacity < requiredCapacity) newCapacity *= 2;
	resizeCapacity(newCapacity);
}
void NPCDataECSList::add(const NPCData& value)
{
	ensureCapacity(mCount + 1);
	writeValue(mCount, value);
	++mCount;
}
void NPCDataECSList::addRange(const NPCData* values, int count)
{
	if (values == nullptr || count <= 0) return;
	const int startIndex = mCount;
	ensureCapacity(startIndex + count);
	for (int i = 0; i < count; ++i)
	{
		const int targetIndex = startIndex + i;
		const NPCData& value = values[i];
		mStorage.mHP[targetIndex] = value.mHP;
		mStorage.mTalkDistance[targetIndex] = value.mTalkDistance;
		mStorage.mAoS[targetIndex].mID = value.mID;
	}
	mCount += count;
}
NPCDataRef NPCDataECSList::addDefault()
{
	ensureCapacity(mCount + 1);
	int index = mCount;
	NPCData value{};
	writeValue(index, value);
	++mCount;
	return (*this)[index];
}
void NPCDataECSList::insert(int index, const NPCData& value)
{
	assert(index >= 0 && index <= mCount);
	ensureCapacity(mCount + 1);
	int moveCount = mCount - index;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mHP + index + 1, mStorage.mHP + index, sizeof(int) * moveCount);
		std::memmove(mStorage.mTalkDistance + index + 1, mStorage.mTalkDistance + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index + 1, mStorage.mAoS + index, sizeof(NPCDataAoSBlock) * moveCount);
	}
	writeValue(index, value);
	++mCount;
}
void NPCDataECSList::removeAt(int index)
{
	assert(index >= 0 && index < mCount);
	int moveCount = mCount - index - 1;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mHP + index, mStorage.mHP + index + 1, sizeof(int) * moveCount);
		std::memmove(mStorage.mTalkDistance + index, mStorage.mTalkDistance + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index, mStorage.mAoS + index + 1, sizeof(NPCDataAoSBlock) * moveCount);
	}
	--mCount;
}
void NPCDataECSList::removeAtSwapBack(int index)
{
	assert(index >= 0 && index < mCount);
	int lastIndex = mCount - 1;
	if (index != lastIndex) copyValue(index, lastIndex);
	--mCount;
}
void NPCDataECSList::popBack()
{
	assert(mCount > 0);
	--mCount;
}
NPCData NPCDataECSList::get(int index) const
{
	assert(index >= 0 && index < mCount);
	NPCData value;
	value.mHP = mStorage.mHP[index];
	value.mTalkDistance = mStorage.mTalkDistance[index];
	value.mID = mStorage.mAoS[index].mID;
	return value;
}
void NPCDataECSList::set(int index, const NPCData& value)
{
	assert(index >= 0 && index < mCount);
	writeValue(index, value);
}
void NPCDataECSList::resizeCapacity(int newCapacity)
{
	assert(newCapacity >= mCount);
	NPCDataStorage newStorage;
	allocateStorage(newStorage, newCapacity);
	copyStorage(newStorage, mStorage, mCount);
	NPCDataStorage oldStorage = mStorage;
	mStorage = newStorage;
	releaseStorage(oldStorage);
	mCapacity = newCapacity;
}
void NPCDataECSList::allocateStorage(NPCDataStorage& storage, int capacity)
{
	size_t offset = 0;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mHPOffset = offset;
	offset += sizeof(int) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mTalkDistanceOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t aosOffset = offset;
	offset += sizeof(NPCDataAoSBlock) * capacity;
	size_t allocateSize = offset + EASY_ECS_MEMORY_ALIGNMENT - 1;
	void* rawMemory = std::malloc(allocateSize);
	if (rawMemory == nullptr) throw std::bad_alloc();
	uint8_t* alignedMemory = EasyECSRuntime::alignPointer(rawMemory, EASY_ECS_MEMORY_ALIGNMENT);
	storage.mRawMemory = rawMemory;
	storage.mAlignedMemory = alignedMemory;
	storage.mHP = reinterpret_cast<int*>(alignedMemory + mHPOffset);
	storage.mTalkDistance = reinterpret_cast<float*>(alignedMemory + mTalkDistanceOffset);
	storage.mAoS = reinterpret_cast<NPCDataAoSBlock*>(alignedMemory + aosOffset);
	for (int i = 0; i < capacity; ++i)
	{
		new (storage.mHP + i) int;
		new (storage.mTalkDistance + i) float;
		new (storage.mAoS + i) NPCDataAoSBlock;
	}
}
void NPCDataECSList::releaseStorage(NPCDataStorage& storage)
{
	if (storage.mRawMemory != nullptr) std::free(storage.mRawMemory);
	storage = {};
}
void NPCDataECSList::copyStorage(NPCDataStorage& target, const NPCDataStorage& source, int count)
{
	if (count <= 0) return;
	std::memcpy(target.mHP, source.mHP, sizeof(int) * count);
	std::memcpy(target.mTalkDistance, source.mTalkDistance, sizeof(float) * count);
	std::memcpy(target.mAoS, source.mAoS, sizeof(NPCDataAoSBlock) * count);
}
void NPCDataECSList::swap(NPCDataECSList& other) noexcept
{
	using std::swap;
	swap(mStorage, other.mStorage);
	swap(mCount, other.mCount);
	swap(mCapacity, other.mCapacity);
}
}

namespace EasyECSDemo
{
TypeDataECSList::TypeDataECSList(int capacity)
{
	static_assert(
		std::is_trivially_copyable<unsigned>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mUnsigned line 52");
	static_assert(
		std::is_default_constructible<unsigned>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mUnsigned line 52");
	static_assert(
		std::is_trivially_destructible<unsigned>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mUnsigned line 52");
	static_assert(
		std::is_trivially_copy_assignable<unsigned>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mUnsigned line 52");
	static_assert(
		alignof(unsigned) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mUnsigned line 52");
	static_assert(
		std::is_trivially_copyable<long long>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mLongLong line 53");
	static_assert(
		std::is_default_constructible<long long>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mLongLong line 53");
	static_assert(
		std::is_trivially_destructible<long long>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mLongLong line 53");
	static_assert(
		std::is_trivially_copy_assignable<long long>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mLongLong line 53");
	static_assert(
		alignof(long long) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mLongLong line 53");
	static_assert(
		std::is_trivially_copyable<uint32_t>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mUInt32 line 54");
	static_assert(
		std::is_default_constructible<uint32_t>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mUInt32 line 54");
	static_assert(
		std::is_trivially_destructible<uint32_t>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mUInt32 line 54");
	static_assert(
		std::is_trivially_copy_assignable<uint32_t>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mUInt32 line 54");
	static_assert(
		alignof(uint32_t) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mUInt32 line 54");
	static_assert(
		std::is_trivially_copyable<int64_t>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mInt64 line 55");
	static_assert(
		std::is_default_constructible<int64_t>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mInt64 line 55");
	static_assert(
		std::is_trivially_destructible<int64_t>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mInt64 line 55");
	static_assert(
		std::is_trivially_copy_assignable<int64_t>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mInt64 line 55");
	static_assert(
		alignof(int64_t) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mInt64 line 55");
	static_assert(
		std::is_trivially_copyable<TypeDataState>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mState line 56");
	static_assert(
		std::is_default_constructible<TypeDataState>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mState line 56");
	static_assert(
		std::is_trivially_destructible<TypeDataState>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mState line 56");
	static_assert(
		std::is_trivially_copy_assignable<TypeDataState>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mState line 56");
	static_assert(
		alignof(TypeDataState) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mState line 56");
	static_assert(
		std::is_trivially_copyable<TypeDataVector2>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mPosition line 57");
	static_assert(
		std::is_default_constructible<TypeDataVector2>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mPosition line 57");
	static_assert(
		std::is_trivially_destructible<TypeDataVector2>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mPosition line 57");
	static_assert(
		std::is_trivially_copy_assignable<TypeDataVector2>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mPosition line 57");
	static_assert(
		alignof(TypeDataVector2) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mPosition line 57");
	static_assert(
		std::is_trivially_copyable<TypeDataAliasUInt>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mAliasUInt line 58");
	static_assert(
		std::is_default_constructible<TypeDataAliasUInt>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mAliasUInt line 58");
	static_assert(
		std::is_trivially_destructible<TypeDataAliasUInt>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mAliasUInt line 58");
	static_assert(
		std::is_trivially_copy_assignable<TypeDataAliasUInt>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mAliasUInt line 58");
	static_assert(
		alignof(TypeDataAliasUInt) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mAliasUInt line 58");
	static_assert(
		std::is_trivially_copyable<TypeDataAliasInt64>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mAliasInt64 line 59");
	static_assert(
		std::is_default_constructible<TypeDataAliasInt64>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mAliasInt64 line 59");
	static_assert(
		std::is_trivially_destructible<TypeDataAliasInt64>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mAliasInt64 line 59");
	static_assert(
		std::is_trivially_copy_assignable<TypeDataAliasInt64>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mAliasInt64 line 59");
	static_assert(
		alignof(TypeDataAliasInt64) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mAliasInt64 line 59");
	static_assert(
		std::is_trivially_copyable<TypeSupport::MoveState>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mMoveState line 60");
	static_assert(
		std::is_default_constructible<TypeSupport::MoveState>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mMoveState line 60");
	static_assert(
		std::is_trivially_destructible<TypeSupport::MoveState>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mMoveState line 60");
	static_assert(
		std::is_trivially_copy_assignable<TypeSupport::MoveState>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mMoveState line 60");
	static_assert(
		alignof(TypeSupport::MoveState) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mMoveState line 60");
	static_assert(
		std::is_trivially_copyable<TypeSupport::Position>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mPosition3 line 61");
	static_assert(
		std::is_default_constructible<TypeSupport::Position>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mPosition3 line 61");
	static_assert(
		std::is_trivially_destructible<TypeSupport::Position>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mPosition3 line 61");
	static_assert(
		std::is_trivially_copy_assignable<TypeSupport::Position>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mPosition3 line 61");
	static_assert(
		alignof(TypeSupport::Position) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mPosition3 line 61");
	static_assert(
		std::is_trivially_copyable<std::array<uint16_t, 4>>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mFixedValues line 62");
	static_assert(
		std::is_default_constructible<std::array<uint16_t, 4>>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mFixedValues line 62");
	static_assert(
		std::is_trivially_destructible<std::array<uint16_t, 4>>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mFixedValues line 62");
	static_assert(
		std::is_trivially_copy_assignable<std::array<uint16_t, 4>>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mFixedValues line 62");
	static_assert(
		alignof(std::array<uint16_t, 4>) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mFixedValues line 62");
	static_assert(
		std::is_trivially_copyable<uint32_t>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mAttributeValue line 63");
	static_assert(
		std::is_default_constructible<uint32_t>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mAttributeValue line 63");
	static_assert(
		std::is_trivially_destructible<uint32_t>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mAttributeValue line 63");
	static_assert(
		std::is_trivially_copy_assignable<uint32_t>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mAttributeValue line 63");
	static_assert(
		alignof(uint32_t) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mAttributeValue line 63");
	static_assert(
		std::is_trivially_copyable<uint32_t>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mImmutable line 64");
	static_assert(
		std::is_default_constructible<uint32_t>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mImmutable line 64");
	static_assert(
		std::is_trivially_destructible<uint32_t>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mImmutable line 64");
	static_assert(
		std::is_trivially_copy_assignable<uint32_t>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mImmutable line 64");
	static_assert(
		alignof(uint32_t) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mImmutable line 64");
	static_assert(
		std::is_trivially_copyable<unsigned long long>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::TypeData::mTailConst line 65");
	static_assert(
		std::is_default_constructible<unsigned long long>::value,
		"EasyECS field must be default constructible: EasyECSDemo::TypeData::mTailConst line 65");
	static_assert(
		std::is_trivially_destructible<unsigned long long>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::TypeData::mTailConst line 65");
	static_assert(
		std::is_trivially_copy_assignable<unsigned long long>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::TypeData::mTailConst line 65");
	static_assert(
		alignof(unsigned long long) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData::mTailConst line 65");
	static_assert(
		std::is_trivially_copyable<TypeDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copyable: EasyECSDemo::TypeData");
	static_assert(
		std::is_default_constructible<TypeDataAoSBlock>::value,
		"EasyECS NotECS block must be default constructible: EasyECSDemo::TypeData");
	static_assert(
		std::is_trivially_destructible<TypeDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially destructible: EasyECSDemo::TypeData");
	static_assert(
		std::is_trivially_copy_assignable<TypeDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copy assignable: EasyECSDemo::TypeData");
	static_assert(
		alignof(TypeDataAoSBlock) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS NotECS block alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::TypeData");
	static_assert(
		std::is_aggregate<TypeData>::value,
		"EasyECS source structs containing const fields must be aggregate types: EasyECSDemo::TypeData");
	static_assert(
		std::is_copy_constructible<TypeData>::value,
		"EasyECS source structs containing const fields must be copy constructible: EasyECSDemo::TypeData");
	if (capacity < 1) capacity = 4;
	allocateStorage(mStorage, capacity);
	mCapacity = capacity;
}
TypeDataECSList::TypeDataECSList(const TypeDataECSList& other)
{
	if (other.mCapacity <= 0) return;
	allocateStorage(mStorage, other.mCapacity);
	copyStorage(mStorage, other.mStorage, other.mCount);
	mCount = other.mCount;
	mCapacity = other.mCapacity;
}
TypeDataECSList& TypeDataECSList::operator=(const TypeDataECSList& other)
{
	if (this == &other) return *this;
	TypeDataECSList copy(other);
	swap(copy);
	return *this;
}
TypeDataECSList::TypeDataECSList(TypeDataECSList&& other) noexcept : mStorage(other.mStorage), mCount(other.mCount), mCapacity(other.mCapacity)
{
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
}
TypeDataECSList& TypeDataECSList::operator=(TypeDataECSList&& other) noexcept
{
	if (this == &other) return *this;
	releaseStorage(mStorage);
	mStorage = other.mStorage;
	mCount = other.mCount;
	mCapacity = other.mCapacity;
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
	return *this;
}
TypeDataECSList::~TypeDataECSList()
{
	releaseStorage(mStorage);
}
void TypeDataECSList::clear()
{
	clearKeepCapacity();
}
void TypeDataECSList::clearKeepCapacity()
{
	mCount = 0;
}
void TypeDataECSList::clearAndRelease()
{
	releaseStorage(mStorage);
	mCount = 0;
	mCapacity = 0;
}
void TypeDataECSList::reserve(int capacity)
{
	if (capacity > mCapacity) resizeCapacity(capacity);
}
void TypeDataECSList::shrinkToFit()
{
	int targetCapacity = mCount > 0 ? mCount : 4;
	if (targetCapacity < mCapacity) resizeCapacity(targetCapacity);
}
void TypeDataECSList::ensureCapacity(int requiredCapacity)
{
	if (requiredCapacity <= mCapacity) return;
	int newCapacity = mCapacity > 0 ? mCapacity : 4;
	while (newCapacity < requiredCapacity) newCapacity *= 2;
	resizeCapacity(newCapacity);
}
void TypeDataECSList::add(const TypeData& value)
{
	ensureCapacity(mCount + 1);
	writeValue(mCount, value);
	++mCount;
}
void TypeDataECSList::addRange(const TypeData* values, int count)
{
	if (values == nullptr || count <= 0) return;
	const int startIndex = mCount;
	ensureCapacity(startIndex + count);
	for (int i = 0; i < count; ++i)
	{
		const int targetIndex = startIndex + i;
		const TypeData& value = values[i];
		mStorage.mUnsigned[targetIndex] = value.mUnsigned;
		mStorage.mLongLong[targetIndex] = value.mLongLong;
		mStorage.mUInt32[targetIndex] = value.mUInt32;
		mStorage.mInt64[targetIndex] = value.mInt64;
		mStorage.mState[targetIndex] = value.mState;
		mStorage.mPosition[targetIndex] = value.mPosition;
		mStorage.mAliasUInt[targetIndex] = value.mAliasUInt;
		mStorage.mAliasInt64[targetIndex] = value.mAliasInt64;
		mStorage.mMoveState[targetIndex] = value.mMoveState;
		mStorage.mPosition3[targetIndex] = value.mPosition3;
		mStorage.mFixedValues[targetIndex] = value.mFixedValues;
		mStorage.mAttributeValue[targetIndex] = value.mAttributeValue;
		mStorage.mImmutable[targetIndex] = value.mImmutable;
		mStorage.mTailConst[targetIndex] = value.mTailConst;
		mStorage.mAoS[targetIndex].mID = value.mID;
	}
	mCount += count;
}
TypeDataRef TypeDataECSList::addDefault()
{
	ensureCapacity(mCount + 1);
	int index = mCount;
	TypeData value{};
	writeValue(index, value);
	++mCount;
	return (*this)[index];
}
void TypeDataECSList::insert(int index, const TypeData& value)
{
	assert(index >= 0 && index <= mCount);
	ensureCapacity(mCount + 1);
	int moveCount = mCount - index;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mUnsigned + index + 1, mStorage.mUnsigned + index, sizeof(unsigned) * moveCount);
		std::memmove(mStorage.mLongLong + index + 1, mStorage.mLongLong + index, sizeof(long long) * moveCount);
		std::memmove(mStorage.mUInt32 + index + 1, mStorage.mUInt32 + index, sizeof(uint32_t) * moveCount);
		std::memmove(mStorage.mInt64 + index + 1, mStorage.mInt64 + index, sizeof(int64_t) * moveCount);
		std::memmove(mStorage.mState + index + 1, mStorage.mState + index, sizeof(TypeDataState) * moveCount);
		std::memmove(mStorage.mPosition + index + 1, mStorage.mPosition + index, sizeof(TypeDataVector2) * moveCount);
		std::memmove(mStorage.mAliasUInt + index + 1, mStorage.mAliasUInt + index, sizeof(TypeDataAliasUInt) * moveCount);
		std::memmove(mStorage.mAliasInt64 + index + 1, mStorage.mAliasInt64 + index, sizeof(TypeDataAliasInt64) * moveCount);
		std::memmove(mStorage.mMoveState + index + 1, mStorage.mMoveState + index, sizeof(TypeSupport::MoveState) * moveCount);
		std::memmove(mStorage.mPosition3 + index + 1, mStorage.mPosition3 + index, sizeof(TypeSupport::Position) * moveCount);
		std::memmove(mStorage.mFixedValues + index + 1, mStorage.mFixedValues + index, sizeof(std::array<uint16_t, 4>) * moveCount);
		std::memmove(mStorage.mAttributeValue + index + 1, mStorage.mAttributeValue + index, sizeof(uint32_t) * moveCount);
		std::memmove(mStorage.mImmutable + index + 1, mStorage.mImmutable + index, sizeof(uint32_t) * moveCount);
		std::memmove(mStorage.mTailConst + index + 1, mStorage.mTailConst + index, sizeof(unsigned long long) * moveCount);
		std::memmove(mStorage.mAoS + index + 1, mStorage.mAoS + index, sizeof(TypeDataAoSBlock) * moveCount);
	}
	writeValue(index, value);
	++mCount;
}
void TypeDataECSList::removeAt(int index)
{
	assert(index >= 0 && index < mCount);
	int moveCount = mCount - index - 1;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mUnsigned + index, mStorage.mUnsigned + index + 1, sizeof(unsigned) * moveCount);
		std::memmove(mStorage.mLongLong + index, mStorage.mLongLong + index + 1, sizeof(long long) * moveCount);
		std::memmove(mStorage.mUInt32 + index, mStorage.mUInt32 + index + 1, sizeof(uint32_t) * moveCount);
		std::memmove(mStorage.mInt64 + index, mStorage.mInt64 + index + 1, sizeof(int64_t) * moveCount);
		std::memmove(mStorage.mState + index, mStorage.mState + index + 1, sizeof(TypeDataState) * moveCount);
		std::memmove(mStorage.mPosition + index, mStorage.mPosition + index + 1, sizeof(TypeDataVector2) * moveCount);
		std::memmove(mStorage.mAliasUInt + index, mStorage.mAliasUInt + index + 1, sizeof(TypeDataAliasUInt) * moveCount);
		std::memmove(mStorage.mAliasInt64 + index, mStorage.mAliasInt64 + index + 1, sizeof(TypeDataAliasInt64) * moveCount);
		std::memmove(mStorage.mMoveState + index, mStorage.mMoveState + index + 1, sizeof(TypeSupport::MoveState) * moveCount);
		std::memmove(mStorage.mPosition3 + index, mStorage.mPosition3 + index + 1, sizeof(TypeSupport::Position) * moveCount);
		std::memmove(mStorage.mFixedValues + index, mStorage.mFixedValues + index + 1, sizeof(std::array<uint16_t, 4>) * moveCount);
		std::memmove(mStorage.mAttributeValue + index, mStorage.mAttributeValue + index + 1, sizeof(uint32_t) * moveCount);
		std::memmove(mStorage.mImmutable + index, mStorage.mImmutable + index + 1, sizeof(uint32_t) * moveCount);
		std::memmove(mStorage.mTailConst + index, mStorage.mTailConst + index + 1, sizeof(unsigned long long) * moveCount);
		std::memmove(mStorage.mAoS + index, mStorage.mAoS + index + 1, sizeof(TypeDataAoSBlock) * moveCount);
	}
	--mCount;
}
void TypeDataECSList::removeAtSwapBack(int index)
{
	assert(index >= 0 && index < mCount);
	int lastIndex = mCount - 1;
	if (index != lastIndex) copyValue(index, lastIndex);
	--mCount;
}
void TypeDataECSList::popBack()
{
	assert(mCount > 0);
	--mCount;
}
TypeData TypeDataECSList::get(int index) const
{
	assert(index >= 0 && index < mCount);
	return TypeData{
		mStorage.mUnsigned[index],
		mStorage.mLongLong[index],
		mStorage.mUInt32[index],
		mStorage.mInt64[index],
		mStorage.mState[index],
		mStorage.mPosition[index],
		mStorage.mAliasUInt[index],
		mStorage.mAliasInt64[index],
		mStorage.mMoveState[index],
		mStorage.mPosition3[index],
		mStorage.mFixedValues[index],
		mStorage.mAttributeValue[index],
		mStorage.mImmutable[index],
		mStorage.mTailConst[index],
		mStorage.mAoS[index].mID
	};
}
void TypeDataECSList::set(int index, const TypeData& value)
{
	assert(index >= 0 && index < mCount);
	writeValue(index, value);
}
void TypeDataECSList::resizeCapacity(int newCapacity)
{
	assert(newCapacity >= mCount);
	TypeDataStorage newStorage;
	allocateStorage(newStorage, newCapacity);
	copyStorage(newStorage, mStorage, mCount);
	TypeDataStorage oldStorage = mStorage;
	mStorage = newStorage;
	releaseStorage(oldStorage);
	mCapacity = newCapacity;
}
void TypeDataECSList::allocateStorage(TypeDataStorage& storage, int capacity)
{
	size_t offset = 0;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mUnsignedOffset = offset;
	offset += sizeof(unsigned) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mLongLongOffset = offset;
	offset += sizeof(long long) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mUInt32Offset = offset;
	offset += sizeof(uint32_t) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mInt64Offset = offset;
	offset += sizeof(int64_t) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mStateOffset = offset;
	offset += sizeof(TypeDataState) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mPositionOffset = offset;
	offset += sizeof(TypeDataVector2) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mAliasUIntOffset = offset;
	offset += sizeof(TypeDataAliasUInt) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mAliasInt64Offset = offset;
	offset += sizeof(TypeDataAliasInt64) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mMoveStateOffset = offset;
	offset += sizeof(TypeSupport::MoveState) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mPosition3Offset = offset;
	offset += sizeof(TypeSupport::Position) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mFixedValuesOffset = offset;
	offset += sizeof(std::array<uint16_t, 4>) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mAttributeValueOffset = offset;
	offset += sizeof(uint32_t) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mImmutableOffset = offset;
	offset += sizeof(uint32_t) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mTailConstOffset = offset;
	offset += sizeof(unsigned long long) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t aosOffset = offset;
	offset += sizeof(TypeDataAoSBlock) * capacity;
	size_t allocateSize = offset + EASY_ECS_MEMORY_ALIGNMENT - 1;
	void* rawMemory = std::malloc(allocateSize);
	if (rawMemory == nullptr) throw std::bad_alloc();
	uint8_t* alignedMemory = EasyECSRuntime::alignPointer(rawMemory, EASY_ECS_MEMORY_ALIGNMENT);
	storage.mRawMemory = rawMemory;
	storage.mAlignedMemory = alignedMemory;
	storage.mUnsigned = reinterpret_cast<unsigned*>(alignedMemory + mUnsignedOffset);
	storage.mLongLong = reinterpret_cast<long long*>(alignedMemory + mLongLongOffset);
	storage.mUInt32 = reinterpret_cast<uint32_t*>(alignedMemory + mUInt32Offset);
	storage.mInt64 = reinterpret_cast<int64_t*>(alignedMemory + mInt64Offset);
	storage.mState = reinterpret_cast<TypeDataState*>(alignedMemory + mStateOffset);
	storage.mPosition = reinterpret_cast<TypeDataVector2*>(alignedMemory + mPositionOffset);
	storage.mAliasUInt = reinterpret_cast<TypeDataAliasUInt*>(alignedMemory + mAliasUIntOffset);
	storage.mAliasInt64 = reinterpret_cast<TypeDataAliasInt64*>(alignedMemory + mAliasInt64Offset);
	storage.mMoveState = reinterpret_cast<TypeSupport::MoveState*>(alignedMemory + mMoveStateOffset);
	storage.mPosition3 = reinterpret_cast<TypeSupport::Position*>(alignedMemory + mPosition3Offset);
	storage.mFixedValues = reinterpret_cast<std::array<uint16_t, 4>*>(alignedMemory + mFixedValuesOffset);
	storage.mAttributeValue = reinterpret_cast<uint32_t*>(alignedMemory + mAttributeValueOffset);
	storage.mImmutable = reinterpret_cast<uint32_t*>(alignedMemory + mImmutableOffset);
	storage.mTailConst = reinterpret_cast<unsigned long long*>(alignedMemory + mTailConstOffset);
	storage.mAoS = reinterpret_cast<TypeDataAoSBlock*>(alignedMemory + aosOffset);
	for (int i = 0; i < capacity; ++i)
	{
		new (storage.mUnsigned + i) unsigned;
		new (storage.mLongLong + i) long long;
		new (storage.mUInt32 + i) uint32_t;
		new (storage.mInt64 + i) int64_t;
		new (storage.mState + i) TypeDataState;
		new (storage.mPosition + i) TypeDataVector2;
		new (storage.mAliasUInt + i) TypeDataAliasUInt;
		new (storage.mAliasInt64 + i) TypeDataAliasInt64;
		new (storage.mMoveState + i) TypeSupport::MoveState;
		new (storage.mPosition3 + i) TypeSupport::Position;
		new (storage.mFixedValues + i) std::array<uint16_t, 4>;
		new (storage.mAttributeValue + i) uint32_t;
		new (storage.mImmutable + i) uint32_t;
		new (storage.mTailConst + i) unsigned long long;
		new (storage.mAoS + i) TypeDataAoSBlock;
	}
}
void TypeDataECSList::releaseStorage(TypeDataStorage& storage)
{
	if (storage.mRawMemory != nullptr) std::free(storage.mRawMemory);
	storage = {};
}
void TypeDataECSList::copyStorage(TypeDataStorage& target, const TypeDataStorage& source, int count)
{
	if (count <= 0) return;
	std::memcpy(target.mUnsigned, source.mUnsigned, sizeof(unsigned) * count);
	std::memcpy(target.mLongLong, source.mLongLong, sizeof(long long) * count);
	std::memcpy(target.mUInt32, source.mUInt32, sizeof(uint32_t) * count);
	std::memcpy(target.mInt64, source.mInt64, sizeof(int64_t) * count);
	std::memcpy(target.mState, source.mState, sizeof(TypeDataState) * count);
	std::memcpy(target.mPosition, source.mPosition, sizeof(TypeDataVector2) * count);
	std::memcpy(target.mAliasUInt, source.mAliasUInt, sizeof(TypeDataAliasUInt) * count);
	std::memcpy(target.mAliasInt64, source.mAliasInt64, sizeof(TypeDataAliasInt64) * count);
	std::memcpy(target.mMoveState, source.mMoveState, sizeof(TypeSupport::MoveState) * count);
	std::memcpy(target.mPosition3, source.mPosition3, sizeof(TypeSupport::Position) * count);
	std::memcpy(target.mFixedValues, source.mFixedValues, sizeof(std::array<uint16_t, 4>) * count);
	std::memcpy(target.mAttributeValue, source.mAttributeValue, sizeof(uint32_t) * count);
	std::memcpy(target.mImmutable, source.mImmutable, sizeof(uint32_t) * count);
	std::memcpy(target.mTailConst, source.mTailConst, sizeof(unsigned long long) * count);
	std::memcpy(target.mAoS, source.mAoS, sizeof(TypeDataAoSBlock) * count);
}
void TypeDataECSList::swap(TypeDataECSList& other) noexcept
{
	using std::swap;
	swap(mStorage, other.mStorage);
	swap(mCount, other.mCount);
	swap(mCapacity, other.mCapacity);
}
}


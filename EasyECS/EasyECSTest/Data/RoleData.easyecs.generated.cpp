#include "RoleData.easyecs.generated.h"
#include <cstdlib>
#include <cstring>
#include <new>

RoleDataECSList::RoleDataECSList(int capacity)
{
	static_assert(
		std::is_trivially_copyable<int>::value,
		"EasyECS field must be trivially copyable: RoleData::mHP line 7");
	static_assert(
		std::is_default_constructible<int>::value,
		"EasyECS field must be default constructible: RoleData::mHP line 7");
	static_assert(
		std::is_trivially_destructible<int>::value,
		"EasyECS field must be trivially destructible: RoleData::mHP line 7");
	static_assert(
		std::is_trivially_copy_assignable<int>::value,
		"EasyECS field must be trivially copy assignable: RoleData::mHP line 7");
	static_assert(
		alignof(int) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: RoleData::mHP line 7");
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: RoleData::mSpeed line 8");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: RoleData::mSpeed line 8");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: RoleData::mSpeed line 8");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: RoleData::mSpeed line 8");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: RoleData::mSpeed line 8");
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: RoleData::mPositionX line 9");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: RoleData::mPositionX line 9");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: RoleData::mPositionX line 9");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: RoleData::mPositionX line 9");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: RoleData::mPositionX line 9");
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: RoleData::mPositionY line 10");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: RoleData::mPositionY line 10");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: RoleData::mPositionY line 10");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: RoleData::mPositionY line 10");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: RoleData::mPositionY line 10");
	static_assert(
		std::is_trivially_copyable<RoleDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copyable: RoleData");
	static_assert(
		std::is_default_constructible<RoleDataAoSBlock>::value,
		"EasyECS NotECS block must be default constructible: RoleData");
	static_assert(
		std::is_trivially_destructible<RoleDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially destructible: RoleData");
	static_assert(
		std::is_trivially_copy_assignable<RoleDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copy assignable: RoleData");
	static_assert(
		alignof(RoleDataAoSBlock) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS NotECS block alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: RoleData");
	if (capacity < 1) capacity = 4;
	allocateStorage(mStorage, capacity);
	mCapacity = capacity;
}
RoleDataECSList::RoleDataECSList(const RoleDataECSList& other)
{
	if (other.mCapacity <= 0) return;
	allocateStorage(mStorage, other.mCapacity);
	copyStorage(mStorage, other.mStorage, other.mCount);
	mCount = other.mCount;
	mCapacity = other.mCapacity;
}
RoleDataECSList& RoleDataECSList::operator=(const RoleDataECSList& other)
{
	if (this == &other) return *this;
	RoleDataECSList copy(other);
	swap(copy);
	return *this;
}
RoleDataECSList::RoleDataECSList(RoleDataECSList&& other) noexcept : mStorage(other.mStorage), mCount(other.mCount), mCapacity(other.mCapacity)
{
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
}
RoleDataECSList& RoleDataECSList::operator=(RoleDataECSList&& other) noexcept
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
RoleDataECSList::~RoleDataECSList()
{
	releaseStorage(mStorage);
}
void RoleDataECSList::clear()
{
	clearKeepCapacity();
}
void RoleDataECSList::clearKeepCapacity()
{
	mCount = 0;
}
void RoleDataECSList::clearAndRelease()
{
	releaseStorage(mStorage);
	mCount = 0;
	mCapacity = 0;
}
void RoleDataECSList::reserve(int capacity)
{
	if (capacity > mCapacity) resizeCapacity(capacity);
}
void RoleDataECSList::shrinkToFit()
{
	int targetCapacity = mCount > 0 ? mCount : 4;
	if (targetCapacity < mCapacity) resizeCapacity(targetCapacity);
}
void RoleDataECSList::ensureCapacity(int requiredCapacity)
{
	if (requiredCapacity <= mCapacity) return;
	int newCapacity = mCapacity > 0 ? mCapacity : 4;
	while (newCapacity < requiredCapacity) newCapacity *= 2;
	resizeCapacity(newCapacity);
}
void RoleDataECSList::add(const RoleData& value)
{
	ensureCapacity(mCount + 1);
	writeValue(mCount, value);
	++mCount;
}
void RoleDataECSList::addRange(const RoleData* values, int count)
{
	if (values == nullptr || count <= 0) return;
	const int startIndex = mCount;
	ensureCapacity(startIndex + count);
	for (int i = 0; i < count; ++i)
	{
		const int targetIndex = startIndex + i;
		const RoleData& value = values[i];
		mStorage.mHP[targetIndex] = value.mHP;
		mStorage.mSpeed[targetIndex] = value.mSpeed;
		mStorage.mPositionX[targetIndex] = value.mPositionX;
		mStorage.mPositionY[targetIndex] = value.mPositionY;
		mStorage.mAoS[targetIndex].mID = value.mID;
		mStorage.mAoS[targetIndex].mModelID = value.mModelID;
		mStorage.mAoS[targetIndex].mCamp = value.mCamp;
	}
	mCount += count;
}
RoleDataRef RoleDataECSList::addDefault()
{
	ensureCapacity(mCount + 1);
	int index = mCount;
	RoleData value{};
	writeValue(index, value);
	++mCount;
	return (*this)[index];
}
void RoleDataECSList::insert(int index, const RoleData& value)
{
	assert(index >= 0 && index <= mCount);
	ensureCapacity(mCount + 1);
	int moveCount = mCount - index;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mHP + index + 1, mStorage.mHP + index, sizeof(int) * moveCount);
		std::memmove(mStorage.mSpeed + index + 1, mStorage.mSpeed + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mPositionX + index + 1, mStorage.mPositionX + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mPositionY + index + 1, mStorage.mPositionY + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index + 1, mStorage.mAoS + index, sizeof(RoleDataAoSBlock) * moveCount);
	}
	writeValue(index, value);
	++mCount;
}
void RoleDataECSList::removeAt(int index)
{
	assert(index >= 0 && index < mCount);
	int moveCount = mCount - index - 1;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mHP + index, mStorage.mHP + index + 1, sizeof(int) * moveCount);
		std::memmove(mStorage.mSpeed + index, mStorage.mSpeed + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mPositionX + index, mStorage.mPositionX + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mPositionY + index, mStorage.mPositionY + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index, mStorage.mAoS + index + 1, sizeof(RoleDataAoSBlock) * moveCount);
	}
	--mCount;
}
void RoleDataECSList::removeAtSwapBack(int index)
{
	assert(index >= 0 && index < mCount);
	int lastIndex = mCount - 1;
	if (index != lastIndex) copyValue(index, lastIndex);
	--mCount;
}
void RoleDataECSList::popBack()
{
	assert(mCount > 0);
	--mCount;
}
RoleData RoleDataECSList::get(int index) const
{
	assert(index >= 0 && index < mCount);
	RoleData value;
	value.mHP = mStorage.mHP[index];
	value.mSpeed = mStorage.mSpeed[index];
	value.mPositionX = mStorage.mPositionX[index];
	value.mPositionY = mStorage.mPositionY[index];
	value.mID = mStorage.mAoS[index].mID;
	value.mModelID = mStorage.mAoS[index].mModelID;
	value.mCamp = mStorage.mAoS[index].mCamp;
	return value;
}
void RoleDataECSList::set(int index, const RoleData& value)
{
	assert(index >= 0 && index < mCount);
	writeValue(index, value);
}
void RoleDataECSList::resizeCapacity(int newCapacity)
{
	assert(newCapacity >= mCount);
	RoleDataStorage newStorage;
	allocateStorage(newStorage, newCapacity);
	copyStorage(newStorage, mStorage, mCount);
	RoleDataStorage oldStorage = mStorage;
	mStorage = newStorage;
	releaseStorage(oldStorage);
	mCapacity = newCapacity;
}
void RoleDataECSList::allocateStorage(RoleDataStorage& storage, int capacity)
{
	size_t offset = 0;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mHPOffset = offset;
	offset += sizeof(int) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mSpeedOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mPositionXOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mPositionYOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t aosOffset = offset;
	offset += sizeof(RoleDataAoSBlock) * capacity;
	size_t allocateSize = offset + EASY_ECS_MEMORY_ALIGNMENT - 1;
	void* rawMemory = std::malloc(allocateSize);
	if (rawMemory == nullptr) throw std::bad_alloc();
	uint8_t* alignedMemory = EasyECSRuntime::alignPointer(rawMemory, EASY_ECS_MEMORY_ALIGNMENT);
	storage.mRawMemory = rawMemory;
	storage.mAlignedMemory = alignedMemory;
	storage.mHP = reinterpret_cast<int*>(alignedMemory + mHPOffset);
	storage.mSpeed = reinterpret_cast<float*>(alignedMemory + mSpeedOffset);
	storage.mPositionX = reinterpret_cast<float*>(alignedMemory + mPositionXOffset);
	storage.mPositionY = reinterpret_cast<float*>(alignedMemory + mPositionYOffset);
	storage.mAoS = reinterpret_cast<RoleDataAoSBlock*>(alignedMemory + aosOffset);
	for (int i = 0; i < capacity; ++i)
	{
		new (storage.mHP + i) int;
		new (storage.mSpeed + i) float;
		new (storage.mPositionX + i) float;
		new (storage.mPositionY + i) float;
		new (storage.mAoS + i) RoleDataAoSBlock;
	}
}
void RoleDataECSList::releaseStorage(RoleDataStorage& storage)
{
	if (storage.mRawMemory != nullptr) std::free(storage.mRawMemory);
	storage = {};
}
void RoleDataECSList::copyStorage(RoleDataStorage& target, const RoleDataStorage& source, int count)
{
	if (count <= 0) return;
	std::memcpy(target.mHP, source.mHP, sizeof(int) * count);
	std::memcpy(target.mSpeed, source.mSpeed, sizeof(float) * count);
	std::memcpy(target.mPositionX, source.mPositionX, sizeof(float) * count);
	std::memcpy(target.mPositionY, source.mPositionY, sizeof(float) * count);
	std::memcpy(target.mAoS, source.mAoS, sizeof(RoleDataAoSBlock) * count);
}
void RoleDataECSList::swap(RoleDataECSList& other) noexcept
{
	using std::swap;
	swap(mStorage, other.mStorage);
	swap(mCount, other.mCount);
	swap(mCapacity, other.mCapacity);
}


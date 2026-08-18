#include "BulletData.easyecs.generated.h"
#include <cstdlib>
#include <cstring>
#include <new>

namespace EasyECSDemo::Battle
{
BulletDataECSList::BulletDataECSList(int capacity)
{
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::Battle::BulletData::mPositionX line 9");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: EasyECSDemo::Battle::BulletData::mPositionX line 9");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::Battle::BulletData::mPositionX line 9");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::Battle::BulletData::mPositionX line 9");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::Battle::BulletData::mPositionX line 9");
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::Battle::BulletData::mPositionY line 10");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: EasyECSDemo::Battle::BulletData::mPositionY line 10");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::Battle::BulletData::mPositionY line 10");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::Battle::BulletData::mPositionY line 10");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::Battle::BulletData::mPositionY line 10");
	static_assert(
		std::is_trivially_copyable<BulletDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copyable: EasyECSDemo::Battle::BulletData");
	static_assert(
		std::is_default_constructible<BulletDataAoSBlock>::value,
		"EasyECS NotECS block must be default constructible: EasyECSDemo::Battle::BulletData");
	static_assert(
		std::is_trivially_destructible<BulletDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially destructible: EasyECSDemo::Battle::BulletData");
	static_assert(
		std::is_trivially_copy_assignable<BulletDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copy assignable: EasyECSDemo::Battle::BulletData");
	static_assert(
		alignof(BulletDataAoSBlock) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS NotECS block alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::Battle::BulletData");
	if (capacity < 1) capacity = 4;
	allocateStorage(mStorage, capacity);
	mCapacity = capacity;
}
BulletDataECSList::BulletDataECSList(const BulletDataECSList& other)
{
	if (other.mCapacity <= 0) return;
	allocateStorage(mStorage, other.mCapacity);
	copyStorage(mStorage, other.mStorage, other.mCount);
	mCount = other.mCount;
	mCapacity = other.mCapacity;
}
BulletDataECSList& BulletDataECSList::operator=(const BulletDataECSList& other)
{
	if (this == &other) return *this;
	BulletDataECSList copy(other);
	swap(copy);
	return *this;
}
BulletDataECSList::BulletDataECSList(BulletDataECSList&& other) noexcept : mStorage(other.mStorage), mCount(other.mCount), mCapacity(other.mCapacity)
{
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
}
BulletDataECSList& BulletDataECSList::operator=(BulletDataECSList&& other) noexcept
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
BulletDataECSList::~BulletDataECSList()
{
	releaseStorage(mStorage);
}
void BulletDataECSList::clear()
{
	clearKeepCapacity();
}
void BulletDataECSList::clearKeepCapacity()
{
	mCount = 0;
}
void BulletDataECSList::clearAndRelease()
{
	releaseStorage(mStorage);
	mCount = 0;
	mCapacity = 0;
}
void BulletDataECSList::reserve(int capacity)
{
	if (capacity > mCapacity) resizeCapacity(capacity);
}
void BulletDataECSList::shrinkToFit()
{
	int targetCapacity = mCount > 0 ? mCount : 4;
	if (targetCapacity < mCapacity) resizeCapacity(targetCapacity);
}
void BulletDataECSList::ensureCapacity(int requiredCapacity)
{
	if (requiredCapacity <= mCapacity) return;
	int newCapacity = mCapacity > 0 ? mCapacity : 4;
	while (newCapacity < requiredCapacity) newCapacity *= 2;
	resizeCapacity(newCapacity);
}
void BulletDataECSList::add(const BulletData& value)
{
	ensureCapacity(mCount + 1);
	writeValue(mCount, value);
	++mCount;
}
void BulletDataECSList::addRange(const BulletData* values, int count)
{
	if (values == nullptr || count <= 0) return;
	const int startIndex = mCount;
	ensureCapacity(startIndex + count);
	for (int i = 0; i < count; ++i)
	{
		const int targetIndex = startIndex + i;
		const BulletData& value = values[i];
		mStorage.mPositionX[targetIndex] = value.mPositionX;
		mStorage.mPositionY[targetIndex] = value.mPositionY;
		mStorage.mAoS[targetIndex].mOwnerID = value.mOwnerID;
	}
	mCount += count;
}
BulletDataRef BulletDataECSList::addDefault()
{
	ensureCapacity(mCount + 1);
	int index = mCount;
	BulletData value{};
	writeValue(index, value);
	++mCount;
	return (*this)[index];
}
void BulletDataECSList::insert(int index, const BulletData& value)
{
	assert(index >= 0 && index <= mCount);
	ensureCapacity(mCount + 1);
	int moveCount = mCount - index;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mPositionX + index + 1, mStorage.mPositionX + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mPositionY + index + 1, mStorage.mPositionY + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index + 1, mStorage.mAoS + index, sizeof(BulletDataAoSBlock) * moveCount);
	}
	writeValue(index, value);
	++mCount;
}
void BulletDataECSList::removeAt(int index)
{
	assert(index >= 0 && index < mCount);
	int moveCount = mCount - index - 1;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mPositionX + index, mStorage.mPositionX + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mPositionY + index, mStorage.mPositionY + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index, mStorage.mAoS + index + 1, sizeof(BulletDataAoSBlock) * moveCount);
	}
	--mCount;
}
void BulletDataECSList::removeAtSwapBack(int index)
{
	assert(index >= 0 && index < mCount);
	int lastIndex = mCount - 1;
	if (index != lastIndex) copyValue(index, lastIndex);
	--mCount;
}
void BulletDataECSList::popBack()
{
	assert(mCount > 0);
	--mCount;
}
BulletData BulletDataECSList::get(int index) const
{
	assert(index >= 0 && index < mCount);
	BulletData value;
	value.mPositionX = mStorage.mPositionX[index];
	value.mPositionY = mStorage.mPositionY[index];
	value.mOwnerID = mStorage.mAoS[index].mOwnerID;
	return value;
}
void BulletDataECSList::set(int index, const BulletData& value)
{
	assert(index >= 0 && index < mCount);
	writeValue(index, value);
}
void BulletDataECSList::resizeCapacity(int newCapacity)
{
	assert(newCapacity >= mCount);
	BulletDataStorage newStorage;
	allocateStorage(newStorage, newCapacity);
	copyStorage(newStorage, mStorage, mCount);
	BulletDataStorage oldStorage = mStorage;
	mStorage = newStorage;
	releaseStorage(oldStorage);
	mCapacity = newCapacity;
}
void BulletDataECSList::allocateStorage(BulletDataStorage& storage, int capacity)
{
	size_t offset = 0;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mPositionXOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mPositionYOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t aosOffset = offset;
	offset += sizeof(BulletDataAoSBlock) * capacity;
	size_t allocateSize = offset + EASY_ECS_MEMORY_ALIGNMENT - 1;
	void* rawMemory = std::malloc(allocateSize);
	if (rawMemory == nullptr) throw std::bad_alloc();
	uint8_t* alignedMemory = EasyECSRuntime::alignPointer(rawMemory, EASY_ECS_MEMORY_ALIGNMENT);
	storage.mRawMemory = rawMemory;
	storage.mAlignedMemory = alignedMemory;
	storage.mPositionX = reinterpret_cast<float*>(alignedMemory + mPositionXOffset);
	storage.mPositionY = reinterpret_cast<float*>(alignedMemory + mPositionYOffset);
	storage.mAoS = reinterpret_cast<BulletDataAoSBlock*>(alignedMemory + aosOffset);
	for (int i = 0; i < capacity; ++i)
	{
		new (storage.mPositionX + i) float;
		new (storage.mPositionY + i) float;
		new (storage.mAoS + i) BulletDataAoSBlock;
	}
}
void BulletDataECSList::releaseStorage(BulletDataStorage& storage)
{
	if (storage.mRawMemory != nullptr) std::free(storage.mRawMemory);
	storage = {};
}
void BulletDataECSList::copyStorage(BulletDataStorage& target, const BulletDataStorage& source, int count)
{
	if (count <= 0) return;
	std::memcpy(target.mPositionX, source.mPositionX, sizeof(float) * count);
	std::memcpy(target.mPositionY, source.mPositionY, sizeof(float) * count);
	std::memcpy(target.mAoS, source.mAoS, sizeof(BulletDataAoSBlock) * count);
}
void BulletDataECSList::swap(BulletDataECSList& other) noexcept
{
	using std::swap;
	swap(mStorage, other.mStorage);
	swap(mCount, other.mCount);
	swap(mCapacity, other.mCapacity);
}
}


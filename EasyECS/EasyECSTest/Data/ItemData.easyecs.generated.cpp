#include "ItemData.easyecs.generated.h"
#include <cstdlib>
#include <cstring>
#include <new>

namespace EasyECSDemo
{
ItemDataECSList::ItemDataECSList(int capacity)
{
	static_assert(
		std::is_trivially_copyable<int>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::ItemData::mCount line 9");
	static_assert(
		std::is_default_constructible<int>::value,
		"EasyECS field must be default constructible: EasyECSDemo::ItemData::mCount line 9");
	static_assert(
		std::is_trivially_destructible<int>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::ItemData::mCount line 9");
	static_assert(
		std::is_trivially_copy_assignable<int>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::ItemData::mCount line 9");
	static_assert(
		alignof(int) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::ItemData::mCount line 9");
	static_assert(
		std::is_trivially_copyable<float>::value,
		"EasyECS field must be trivially copyable: EasyECSDemo::ItemData::mWeight line 10");
	static_assert(
		std::is_default_constructible<float>::value,
		"EasyECS field must be default constructible: EasyECSDemo::ItemData::mWeight line 10");
	static_assert(
		std::is_trivially_destructible<float>::value,
		"EasyECS field must be trivially destructible: EasyECSDemo::ItemData::mWeight line 10");
	static_assert(
		std::is_trivially_copy_assignable<float>::value,
		"EasyECS field must be trivially copy assignable: EasyECSDemo::ItemData::mWeight line 10");
	static_assert(
		alignof(float) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS field alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::ItemData::mWeight line 10");
	static_assert(
		std::is_trivially_copyable<ItemDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copyable: EasyECSDemo::ItemData");
	static_assert(
		std::is_default_constructible<ItemDataAoSBlock>::value,
		"EasyECS NotECS block must be default constructible: EasyECSDemo::ItemData");
	static_assert(
		std::is_trivially_destructible<ItemDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially destructible: EasyECSDemo::ItemData");
	static_assert(
		std::is_trivially_copy_assignable<ItemDataAoSBlock>::value,
		"EasyECS NotECS block must be trivially copy assignable: EasyECSDemo::ItemData");
	static_assert(
		alignof(ItemDataAoSBlock) <= EASY_ECS_MEMORY_ALIGNMENT,
		"EasyECS NotECS block alignment cannot exceed EASY_ECS_MEMORY_ALIGNMENT: EasyECSDemo::ItemData");
	if (capacity < 1) capacity = 4;
	allocateStorage(mStorage, capacity);
	mCapacity = capacity;
}
ItemDataECSList::ItemDataECSList(const ItemDataECSList& other)
{
	if (other.mCapacity <= 0) return;
	allocateStorage(mStorage, other.mCapacity);
	copyStorage(mStorage, other.mStorage, other.mCount);
	mCount = other.mCount;
	mCapacity = other.mCapacity;
}
ItemDataECSList& ItemDataECSList::operator=(const ItemDataECSList& other)
{
	if (this == &other) return *this;
	ItemDataECSList copy(other);
	swap(copy);
	return *this;
}
ItemDataECSList::ItemDataECSList(ItemDataECSList&& other) noexcept : mStorage(other.mStorage), mCount(other.mCount), mCapacity(other.mCapacity)
{
	other.mStorage = {};
	other.mCount = 0;
	other.mCapacity = 0;
}
ItemDataECSList& ItemDataECSList::operator=(ItemDataECSList&& other) noexcept
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
ItemDataECSList::~ItemDataECSList()
{
	releaseStorage(mStorage);
}
void ItemDataECSList::clear()
{
	clearKeepCapacity();
}
void ItemDataECSList::clearKeepCapacity()
{
	mCount = 0;
}
void ItemDataECSList::clearAndRelease()
{
	releaseStorage(mStorage);
	mCount = 0;
	mCapacity = 0;
}
void ItemDataECSList::reserve(int capacity)
{
	if (capacity > mCapacity) resizeCapacity(capacity);
}
void ItemDataECSList::shrinkToFit()
{
	int targetCapacity = mCount > 0 ? mCount : 4;
	if (targetCapacity < mCapacity) resizeCapacity(targetCapacity);
}
void ItemDataECSList::ensureCapacity(int requiredCapacity)
{
	if (requiredCapacity <= mCapacity) return;
	int newCapacity = mCapacity > 0 ? mCapacity : 4;
	while (newCapacity < requiredCapacity) newCapacity *= 2;
	resizeCapacity(newCapacity);
}
void ItemDataECSList::add(const ItemData& value)
{
	ensureCapacity(mCount + 1);
	writeValue(mCount, value);
	++mCount;
}
void ItemDataECSList::addRange(const ItemData* values, int count)
{
	if (values == nullptr || count <= 0) return;
	const int startIndex = mCount;
	ensureCapacity(startIndex + count);
	for (int i = 0; i < count; ++i)
	{
		const int targetIndex = startIndex + i;
		const ItemData& value = values[i];
		mStorage.mCount[targetIndex] = value.mCount;
		mStorage.mWeight[targetIndex] = value.mWeight;
		mStorage.mAoS[targetIndex].mID = value.mID;
	}
	mCount += count;
}
ItemDataRef ItemDataECSList::addDefault()
{
	ensureCapacity(mCount + 1);
	int index = mCount;
	ItemData value{};
	writeValue(index, value);
	++mCount;
	return (*this)[index];
}
void ItemDataECSList::insert(int index, const ItemData& value)
{
	assert(index >= 0 && index <= mCount);
	ensureCapacity(mCount + 1);
	int moveCount = mCount - index;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mCount + index + 1, mStorage.mCount + index, sizeof(int) * moveCount);
		std::memmove(mStorage.mWeight + index + 1, mStorage.mWeight + index, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index + 1, mStorage.mAoS + index, sizeof(ItemDataAoSBlock) * moveCount);
	}
	writeValue(index, value);
	++mCount;
}
void ItemDataECSList::removeAt(int index)
{
	assert(index >= 0 && index < mCount);
	int moveCount = mCount - index - 1;
	if (moveCount > 0)
	{
		std::memmove(mStorage.mCount + index, mStorage.mCount + index + 1, sizeof(int) * moveCount);
		std::memmove(mStorage.mWeight + index, mStorage.mWeight + index + 1, sizeof(float) * moveCount);
		std::memmove(mStorage.mAoS + index, mStorage.mAoS + index + 1, sizeof(ItemDataAoSBlock) * moveCount);
	}
	--mCount;
}
void ItemDataECSList::removeAtSwapBack(int index)
{
	assert(index >= 0 && index < mCount);
	int lastIndex = mCount - 1;
	if (index != lastIndex) copyValue(index, lastIndex);
	--mCount;
}
void ItemDataECSList::popBack()
{
	assert(mCount > 0);
	--mCount;
}
ItemData ItemDataECSList::get(int index) const
{
	assert(index >= 0 && index < mCount);
	ItemData value;
	value.mCount = mStorage.mCount[index];
	value.mWeight = mStorage.mWeight[index];
	value.mID = mStorage.mAoS[index].mID;
	return value;
}
void ItemDataECSList::set(int index, const ItemData& value)
{
	assert(index >= 0 && index < mCount);
	writeValue(index, value);
}
void ItemDataECSList::resizeCapacity(int newCapacity)
{
	assert(newCapacity >= mCount);
	ItemDataStorage newStorage;
	allocateStorage(newStorage, newCapacity);
	copyStorage(newStorage, mStorage, mCount);
	ItemDataStorage oldStorage = mStorage;
	mStorage = newStorage;
	releaseStorage(oldStorage);
	mCapacity = newCapacity;
}
void ItemDataECSList::allocateStorage(ItemDataStorage& storage, int capacity)
{
	size_t offset = 0;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mCountOffset = offset;
	offset += sizeof(int) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t mWeightOffset = offset;
	offset += sizeof(float) * capacity;
	offset = EasyECSRuntime::alignUp(offset, EASY_ECS_MEMORY_ALIGNMENT);
	size_t aosOffset = offset;
	offset += sizeof(ItemDataAoSBlock) * capacity;
	size_t allocateSize = offset + EASY_ECS_MEMORY_ALIGNMENT - 1;
	void* rawMemory = std::malloc(allocateSize);
	if (rawMemory == nullptr) throw std::bad_alloc();
	uint8_t* alignedMemory = EasyECSRuntime::alignPointer(rawMemory, EASY_ECS_MEMORY_ALIGNMENT);
	storage.mRawMemory = rawMemory;
	storage.mAlignedMemory = alignedMemory;
	storage.mCount = reinterpret_cast<int*>(alignedMemory + mCountOffset);
	storage.mWeight = reinterpret_cast<float*>(alignedMemory + mWeightOffset);
	storage.mAoS = reinterpret_cast<ItemDataAoSBlock*>(alignedMemory + aosOffset);
	for (int i = 0; i < capacity; ++i)
	{
		new (storage.mCount + i) int;
		new (storage.mWeight + i) float;
		new (storage.mAoS + i) ItemDataAoSBlock;
	}
}
void ItemDataECSList::releaseStorage(ItemDataStorage& storage)
{
	if (storage.mRawMemory != nullptr) std::free(storage.mRawMemory);
	storage = {};
}
void ItemDataECSList::copyStorage(ItemDataStorage& target, const ItemDataStorage& source, int count)
{
	if (count <= 0) return;
	std::memcpy(target.mCount, source.mCount, sizeof(int) * count);
	std::memcpy(target.mWeight, source.mWeight, sizeof(float) * count);
	std::memcpy(target.mAoS, source.mAoS, sizeof(ItemDataAoSBlock) * count);
}
void ItemDataECSList::swap(ItemDataECSList& other) noexcept
{
	using std::swap;
	swap(mStorage, other.mStorage);
	swap(mCount, other.mCount);
	swap(mCapacity, other.mCapacity);
}
}


#pragma once
#include "ItemData.hpp"
#include "EasyECS.h"
#include "EasyECSIndexMap.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace EasyECSDemo
{
// Source ECS struct: EasyECSDemo::ItemData
struct ItemDataAoSBlock
{
	int mID;
};
struct ItemDataStorage
{
	void* mRawMemory = nullptr;
	uint8_t* mAlignedMemory = nullptr;
	int* mCount = nullptr;
	float* mWeight = nullptr;
	ItemDataAoSBlock* mAoS = nullptr;
};
struct ItemDataRef
{
	int& mCount;
	float& mWeight;
	int& mID;
};
struct ItemDataConstRef
{
	const int& mCount;
	const float& mWeight;
	const int& mID;
};
template<typename TKey, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>> class ItemDataECSDictionary;
class ItemDataECSList
{
public:
	using SourceType = ItemData;
	explicit ItemDataECSList(int capacity = 4);
	~ItemDataECSList();
	ItemDataECSList(const ItemDataECSList& other);
	ItemDataECSList& operator=(const ItemDataECSList& other);
	ItemDataECSList(ItemDataECSList&& other) noexcept;
	ItemDataECSList& operator=(ItemDataECSList&& other) noexcept;
	EASY_ECS_FORCE_INLINE int size() const { return mCount; }
	EASY_ECS_FORCE_INLINE int capacity() const { return mCapacity; }
	EASY_ECS_FORCE_INLINE bool empty() const { return mCount == 0; }
	EASY_ECS_FORCE_INLINE ItemDataRef operator[](int index)
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mCount[index],
			mStorage.mWeight[index],
			mStorage.mAoS[index].mID
		};
	}
	EASY_ECS_FORCE_INLINE ItemDataConstRef operator[](int index) const
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mCount[index],
			mStorage.mWeight[index],
			mStorage.mAoS[index].mID
		};
	}
	EASY_ECS_FORCE_INLINE int* getCountColumn() { return mStorage.mCount; }
	EASY_ECS_FORCE_INLINE const int* getCountColumn() const { return mStorage.mCount; }
	EASY_ECS_FORCE_INLINE float* getWeightColumn() { return mStorage.mWeight; }
	EASY_ECS_FORCE_INLINE const float* getWeightColumn() const { return mStorage.mWeight; }
	EASY_ECS_FORCE_INLINE ItemDataAoSBlock* getAoSColumn() { return mStorage.mAoS; }
	EASY_ECS_FORCE_INLINE const ItemDataAoSBlock* getAoSColumn() const { return mStorage.mAoS; }
	void clear();
	void clearKeepCapacity();
	void clearAndRelease();
	void reserve(int capacity);
	void shrinkToFit();
	void add(const ItemData& value);
	void addRange(const ItemData* values, int count);
	ItemDataRef addDefault();
	template<typename TPredicate> int removeAll(TPredicate&& predicate)
	{
		int oldSize = mCount;
		if (oldSize <= 0) return 0;
		int writeIndex = 0;
		int removeCount = 0;
		const ItemDataECSList& readOnly = *this;
		for (int readIndex = 0; readIndex < oldSize; ++readIndex)
		{
			if (predicate(readOnly[readIndex]))
			{
				++removeCount;
				continue;
			}
			if (writeIndex != readIndex) copyValue(writeIndex, readIndex);
			++writeIndex;
		}
		mCount = writeIndex;
		return removeCount;
	}
	void insert(int index, const ItemData& value);
	void removeAt(int index);
	void removeAtSwapBack(int index);
	void popBack();
	ItemData get(int index) const;
	void set(int index, const ItemData& value);
private:
	template<typename TKey, typename THash, typename TEqual> friend class ItemDataECSDictionary;
	void ensureCapacity(int requiredCapacity);
	void resizeCapacity(int newCapacity);
	void allocateStorage(ItemDataStorage& storage, int capacity);
	void releaseStorage(ItemDataStorage& storage);
	void copyStorage(ItemDataStorage& target, const ItemDataStorage& source, int count);
	void swap(ItemDataECSList& other) noexcept;
	EASY_ECS_FORCE_INLINE void writeValue(int index, const ItemData& value)
	{
		mStorage.mCount[index] = value.mCount;
		mStorage.mWeight[index] = value.mWeight;
		mStorage.mAoS[index].mID = value.mID;
	}
	EASY_ECS_FORCE_INLINE void copyValue(int targetIndex, int sourceIndex)
	{
		mStorage.mCount[targetIndex] = mStorage.mCount[sourceIndex];
		mStorage.mWeight[targetIndex] = mStorage.mWeight[sourceIndex];
		mStorage.mAoS[targetIndex] = mStorage.mAoS[sourceIndex];
	}
	void compactRemoved(const uint8_t* removed, int oldSize)
	{
		assert(removed != nullptr && oldSize == mCount);
		int writeIndex = 0;
		for (int readIndex = 0; readIndex < oldSize; ++readIndex)
		{
			if (removed[static_cast<size_t>(readIndex)] != 0) continue;
			if (writeIndex != readIndex) copyValue(writeIndex, readIndex);
			++writeIndex;
		}
		mCount = writeIndex;
	}
	ItemDataStorage mStorage;
	int mCount = 0;
	int mCapacity = 0;
};
template<typename TKey, typename THash, typename TEqual>
class ItemDataECSDictionary
{
public:
	using SourceType = ItemData;
	using IndexMap = EasyECSIndexMap<TKey, THash, TEqual>;
	explicit ItemDataECSDictionary(int capacity = 4) : mIndexMap(capacity), mValues(capacity)
	{
		if (capacity < 1) capacity = 4;
		reserve(capacity);
	}
	ItemDataECSDictionary(const ItemDataECSDictionary& other) : mIndexMap(other.mIndexMap), mValues(other.mValues) {}
	ItemDataECSDictionary& operator=(const ItemDataECSDictionary& other)
	{
		if (this == &other) return *this;
		ItemDataECSDictionary copy(other);
		*this = std::move(copy);
		return *this;
	}
	ItemDataECSDictionary(ItemDataECSDictionary&& other) : mIndexMap(std::move(other.mIndexMap)), mValues(std::move(other.mValues)) {}
	ItemDataECSDictionary& operator=(ItemDataECSDictionary&& other)
	{
		if (this == &other) return *this;
		mIndexMap = std::move(other.mIndexMap);
		mValues = std::move(other.mValues);
		return *this;
	}
	EASY_ECS_FORCE_INLINE int size() const { return mIndexMap.size(); }
	EASY_ECS_FORCE_INLINE int capacity() const { return mValues.capacity(); }
	EASY_ECS_FORCE_INLINE int indexCapacity() const { return mIndexMap.capacity(); }
	EASY_ECS_FORCE_INLINE size_t indexMemoryUsageBytes() const { return mIndexMap.memoryUsageBytes(); }
	EASY_ECS_FORCE_INLINE bool empty() const { return mIndexMap.empty(); }
	EASY_ECS_FORCE_INLINE bool containsKey(const TKey& key) const { return mIndexMap.contains(key); }
	EASY_ECS_FORCE_INLINE int getIndex(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return *index; }
	EASY_ECS_FORCE_INLINE bool tryGetIndex(const TKey& key, int& index) const
	{
		const int* foundIndex = mIndexMap.findIndex(key);
		if (foundIndex == nullptr) return false;
		index = *foundIndex;
		return true;
	}
	EASY_ECS_FORCE_INLINE ItemDataRef operator[](const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE ItemDataConstRef operator[](const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE ItemDataRef getValueByIndex(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE ItemDataConstRef getValueByIndex(int index) const { return mValues[index]; }
	EASY_ECS_FORCE_INLINE const TKey& getKeyByIndex(int index) const { return mIndexMap.getKeyByIndex(index); }
	EASY_ECS_FORCE_INLINE ItemDataRef valueAt(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE ItemDataConstRef valueAt(int index) const { return mValues[index]; }
	EASY_ECS_FORCE_INLINE const TKey& keyAt(int index) const { return mIndexMap.getKeyByIndex(index); }
	template<typename TAction> EASY_ECS_FORCE_INLINE void forEach(TAction&& action)
	{
		int count = size();
		for (int i = 0; i < count; ++i) action(keyAt(i), valueAt(i));
	}
	template<typename TAction> EASY_ECS_FORCE_INLINE void forEach(TAction&& action) const
	{
		int count = size();
		for (int i = 0; i < count; ++i) action(keyAt(i), valueAt(i));
	}
	template<typename TPredicate> int removeAll(TPredicate&& predicate)
	{
		int oldSize = size();
		if (oldSize <= 0) return 0;
		std::vector<uint8_t> removed(static_cast<size_t>(oldSize), 0);
		int removeCount = 0;
		const ItemDataECSDictionary& readOnly = *this;
		for (int i = 0; i < oldSize; ++i)
		{
			if (!predicate(readOnly.keyAt(i), readOnly.valueAt(i))) continue;
			removed[static_cast<size_t>(i)] = 1;
			++removeCount;
		}
		if (removeCount == 0) return 0;
		if (removeCount == oldSize)
		{
			clear();
			return removeCount;
		}
		mValues.compactRemoved(removed.data(), oldSize);
		mIndexMap.compactRemove(removed.data(), oldSize);
		return removeCount;
	}
	EASY_ECS_FORCE_INLINE std::optional<ItemDataRef> tryGetRef(const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE std::optional<ItemDataConstRef> tryGetRef(const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE ItemDataECSList& getValues() { return mValues; }
	EASY_ECS_FORCE_INLINE const ItemDataECSList& getValues() const { return mValues; }
	EASY_ECS_FORCE_INLINE int* getCountColumn() { return mValues.getCountColumn(); }
	EASY_ECS_FORCE_INLINE const int* getCountColumn() const { return mValues.getCountColumn(); }
	EASY_ECS_FORCE_INLINE float* getWeightColumn() { return mValues.getWeightColumn(); }
	EASY_ECS_FORCE_INLINE const float* getWeightColumn() const { return mValues.getWeightColumn(); }
	EASY_ECS_FORCE_INLINE ItemDataAoSBlock* getAoSColumn() { return mValues.getAoSColumn(); }
	EASY_ECS_FORCE_INLINE const ItemDataAoSBlock* getAoSColumn() const { return mValues.getAoSColumn(); }
	bool add(const TKey& key, const ItemData& value)
	{
		int index = size();
		if (!mIndexMap.tryAdd(key, index)) return false;
		try
		{
			mValues.add(value);
		}
		catch (...)
		{
			mIndexMap.eraseByIndex(index);
			throw;
		}
		return true;
	}
	EASY_ECS_FORCE_INLINE bool tryAdd(const TKey& key, const ItemData& value) { return add(key, value); }
	int addRange(const TKey* keys, const ItemData* values, int count)
	{
		if (keys == nullptr || values == nullptr || count <= 0) return 0;
		reserve(size() + count);
		int addedCount = 0;
		for (int i = 0; i < count; ++i)
		{
			int index = size();
			if (!mIndexMap.tryAdd(keys[i], index)) continue;
			try
			{
				mValues.add(values[i]);
			}
			catch (...)
			{
				mIndexMap.eraseByIndex(index);
				throw;
			}
			++addedCount;
		}
		return addedCount;
	}
	bool build(const TKey* keys, const ItemData* values, int count)
	{
		if (count < 0 || (count > 0 && (keys == nullptr || values == nullptr))) return false;
		clearKeepCapacity();
		if (count == 0) return true;
		mValues.reserve(count);
		if (!mIndexMap.tryBuild(keys, count)) return false;
		try
		{
			mValues.addRange(values, count);
		}
		catch (...)
		{
			mIndexMap.clearKeepCapacity();
			mValues.clearKeepCapacity();
			throw;
		}
		return true;
	}
	EASY_ECS_FORCE_INLINE std::pair<ItemDataRef, bool> getOrAdd(const TKey& key, const ItemData& defaultValue)
	{
		bool added = false;
		int index = mIndexMap.getOrAddIndex(key, size(), added);
		if (!added) return { mValues[index], false };
		try
		{
			mValues.add(defaultValue);
		}
		catch (...)
		{
			mIndexMap.eraseByIndex(index);
			throw;
		}
		return { mValues[index], true };
	}
	EASY_ECS_FORCE_INLINE std::pair<ItemDataRef, bool> getOrAdd(const TKey& key) { return getOrAdd(key, ItemData{}); }
	void set(const TKey& key, const ItemData& value)
	{
		int* existingIndex = mIndexMap.findIndex(key);
		if (existingIndex != nullptr)
		{
			mValues.set(*existingIndex, value);
			return;
		}
		int index = size();
		if (!mIndexMap.tryAdd(key, index))
		{
			int* duplicateIndex = mIndexMap.findIndex(key);
			assert(duplicateIndex != nullptr);
			mValues.set(*duplicateIndex, value);
			return;
		}
		try
		{
			mValues.add(value);
		}
		catch (...)
		{
			mIndexMap.eraseByIndex(index);
			throw;
		}
	}
	EASY_ECS_FORCE_INLINE bool remove(const TKey& key)
	{
		int removeIndex = -1;
		if (!mIndexMap.erase(key, removeIndex)) return false;
		mValues.removeAtSwapBack(removeIndex);
		return true;
	}
	EASY_ECS_FORCE_INLINE bool removeByIndex(int index)
	{
		if (!mIndexMap.eraseByIndex(index)) return false;
		mValues.removeAtSwapBack(index);
		return true;
	}
	EASY_ECS_FORCE_INLINE int removeBatch(const std::vector<TKey>& keys) { return removeBatch(keys.data(), static_cast<int>(keys.size())); }
	EASY_ECS_FORCE_INLINE int removeBatch(const TKey* keys, int keyCount)
	{
		if (keys == nullptr || keyCount <= 0 || empty()) return 0;
		int currentSize = size();
		if (keyCount * 10 < currentSize * 7) return removeBatchSmall(keys, keyCount);
		return removeBatchLarge(keys, keyCount, currentSize);
	}
	int removeByIndexBatch(const std::vector<int>& indices) { return removeByIndexBatch(indices.data(), static_cast<int>(indices.size())); }
	int removeByIndexBatch(const int* indices, int indexCount)
	{
		if (indices == nullptr || indexCount <= 0 || empty()) return 0;
		int currentSize = size();
		if (indexCount * 4 < currentSize)
		{
			std::vector<int> validIndices;
			validIndices.reserve(static_cast<size_t>(indexCount));
			for (int i = 0; i < indexCount; ++i) if (indices[i] >= 0 && indices[i] < currentSize) validIndices.push_back(indices[i]);
			std::sort(validIndices.begin(), validIndices.end(), std::greater<int>());
			validIndices.erase(std::unique(validIndices.begin(), validIndices.end()), validIndices.end());
			for (int index : validIndices) removeByIndex(index);
			return static_cast<int>(validIndices.size());
		}
		std::vector<uint8_t> removed(static_cast<size_t>(currentSize), 0);
		int removeCount = 0;
		for (int i = 0; i < indexCount; ++i)
		{
			int index = indices[i];
			if (index < 0 || index >= currentSize || removed[static_cast<size_t>(index)] != 0) continue;
			removed[static_cast<size_t>(index)] = 1;
			++removeCount;
		}
		if (removeCount == 0) return 0;
		if (removeCount == currentSize)
		{
			clear();
			return removeCount;
		}
		if (removeCount * 10 < currentSize * 7)
		{
			for (int index = currentSize - 1; index >= 0; --index) if (removed[static_cast<size_t>(index)] != 0) removeByIndex(index);
			return removeCount;
		}
		mValues.compactRemoved(removed.data(), currentSize);
		mIndexMap.compactRemove(removed.data(), currentSize);
		return removeCount;
	}
	bool tryGetValue(const TKey& key, ItemData& value) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return false;
		value = mValues.get(*index);
		return true;
	}
	ItemData get(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return mValues.get(*index); }
	void clear() { clearKeepCapacity(); }
	void clearKeepCapacity() { mIndexMap.clearKeepCapacity(); mValues.clearKeepCapacity(); }
	void clearAndRelease() { mIndexMap.clearAndRelease(); mValues.clearAndRelease(); }
	void reserve(int capacity) { if (capacity <= 0) return; mIndexMap.reserve(capacity); mValues.reserve(capacity); }
	void shrinkToFit() { mIndexMap.shrinkToFit(); mValues.shrinkToFit(); }
private:
	EASY_ECS_NO_INLINE int removeBatchSmall(const TKey* keys, int keyCount)
	{
		int removedCount = 0;
		const TKey* current = keys;
		const TKey* end = keys + keyCount;
		for (; current != end; ++current) removedCount += remove(*current) ? 1 : 0;
		return removedCount;
	}
	EASY_ECS_NO_INLINE int removeBatchLarge(const TKey* keys, int keyCount, int currentSize)
	{
		std::vector<uint8_t> removed(static_cast<size_t>(currentSize), 0);
		int removeCount = 0;
		for (int i = 0; i < keyCount; ++i)
		{
			const int* index = mIndexMap.findIndex(keys[i]);
			if (index == nullptr || removed[static_cast<size_t>(*index)] != 0) continue;
			removed[static_cast<size_t>(*index)] = 1;
			++removeCount;
		}
		if (removeCount == 0) return 0;
		if (removeCount == currentSize)
		{
			clear();
			return removeCount;
		}
		if (removeCount * 10 < currentSize * 7)
		{
			for (int index = currentSize - 1; index >= 0; --index) if (removed[static_cast<size_t>(index)] != 0) removeByIndex(index);
			return removeCount;
		}
		mValues.compactRemoved(removed.data(), currentSize);
		mIndexMap.compactRemove(removed.data(), currentSize);
		return removeCount;
	}
	IndexMap mIndexMap;
	ItemDataECSList mValues;
};
}


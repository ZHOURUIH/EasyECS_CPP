#pragma once
#include "RoleData.h"
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

// Source ECS struct: RoleData
struct RoleDataAoSBlock
{
	int mID;
	int mModelID;
	int mCamp;
};
struct RoleDataStorage
{
	void* mRawMemory = nullptr;
	uint8_t* mAlignedMemory = nullptr;
	int* mHP = nullptr;
	float* mSpeed = nullptr;
	float* mPositionX = nullptr;
	float* mPositionY = nullptr;
	RoleDataAoSBlock* mAoS = nullptr;
};
struct RoleDataRef
{
	int& mHP;
	float& mSpeed;
	float& mPositionX;
	float& mPositionY;
	int& mID;
	int& mModelID;
	int& mCamp;
};
struct RoleDataConstRef
{
	const int& mHP;
	const float& mSpeed;
	const float& mPositionX;
	const float& mPositionY;
	const int& mID;
	const int& mModelID;
	const int& mCamp;
};
template<typename TKey, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>> class RoleDataECSDictionary;
class RoleDataECSList
{
public:
	using SourceType = RoleData;
	explicit RoleDataECSList(int capacity = 4);
	~RoleDataECSList();
	RoleDataECSList(const RoleDataECSList& other);
	RoleDataECSList& operator=(const RoleDataECSList& other);
	RoleDataECSList(RoleDataECSList&& other) noexcept;
	RoleDataECSList& operator=(RoleDataECSList&& other) noexcept;
	EASY_ECS_FORCE_INLINE int size() const { return mCount; }
	EASY_ECS_FORCE_INLINE int capacity() const { return mCapacity; }
	EASY_ECS_FORCE_INLINE bool empty() const { return mCount == 0; }
	EASY_ECS_FORCE_INLINE RoleDataRef operator[](int index)
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mHP[index],
			mStorage.mSpeed[index],
			mStorage.mPositionX[index],
			mStorage.mPositionY[index],
			mStorage.mAoS[index].mID,
			mStorage.mAoS[index].mModelID,
			mStorage.mAoS[index].mCamp
		};
	}
	EASY_ECS_FORCE_INLINE RoleDataConstRef operator[](int index) const
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mHP[index],
			mStorage.mSpeed[index],
			mStorage.mPositionX[index],
			mStorage.mPositionY[index],
			mStorage.mAoS[index].mID,
			mStorage.mAoS[index].mModelID,
			mStorage.mAoS[index].mCamp
		};
	}
	EASY_ECS_FORCE_INLINE int* getHPColumn() { return mStorage.mHP; }
	EASY_ECS_FORCE_INLINE const int* getHPColumn() const { return mStorage.mHP; }
	EASY_ECS_FORCE_INLINE float* getSpeedColumn() { return mStorage.mSpeed; }
	EASY_ECS_FORCE_INLINE const float* getSpeedColumn() const { return mStorage.mSpeed; }
	EASY_ECS_FORCE_INLINE float* getPositionXColumn() { return mStorage.mPositionX; }
	EASY_ECS_FORCE_INLINE const float* getPositionXColumn() const { return mStorage.mPositionX; }
	EASY_ECS_FORCE_INLINE float* getPositionYColumn() { return mStorage.mPositionY; }
	EASY_ECS_FORCE_INLINE const float* getPositionYColumn() const { return mStorage.mPositionY; }
	EASY_ECS_FORCE_INLINE RoleDataAoSBlock* getAoSColumn() { return mStorage.mAoS; }
	EASY_ECS_FORCE_INLINE const RoleDataAoSBlock* getAoSColumn() const { return mStorage.mAoS; }
	void clear();
	void clearKeepCapacity();
	void clearAndRelease();
	void reserve(int capacity);
	void shrinkToFit();
	void add(const RoleData& value);
	void addRange(const RoleData* values, int count);
	RoleDataRef addDefault();
	template<typename TPredicate> int removeAll(TPredicate&& predicate)
	{
		int oldSize = mCount;
		if (oldSize <= 0) return 0;
		int writeIndex = 0;
		int removeCount = 0;
		const RoleDataECSList& readOnly = *this;
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
	void insert(int index, const RoleData& value);
	void removeAt(int index);
	void removeAtSwapBack(int index);
	void popBack();
	RoleData get(int index) const;
	void set(int index, const RoleData& value);
private:
	template<typename TKey, typename THash, typename TEqual> friend class RoleDataECSDictionary;
	void ensureCapacity(int requiredCapacity);
	void resizeCapacity(int newCapacity);
	void allocateStorage(RoleDataStorage& storage, int capacity);
	void releaseStorage(RoleDataStorage& storage);
	void copyStorage(RoleDataStorage& target, const RoleDataStorage& source, int count);
	void swap(RoleDataECSList& other) noexcept;
	EASY_ECS_FORCE_INLINE void writeValue(int index, const RoleData& value)
	{
		mStorage.mHP[index] = value.mHP;
		mStorage.mSpeed[index] = value.mSpeed;
		mStorage.mPositionX[index] = value.mPositionX;
		mStorage.mPositionY[index] = value.mPositionY;
		mStorage.mAoS[index].mID = value.mID;
		mStorage.mAoS[index].mModelID = value.mModelID;
		mStorage.mAoS[index].mCamp = value.mCamp;
	}
	EASY_ECS_FORCE_INLINE void copyValue(int targetIndex, int sourceIndex)
	{
		mStorage.mHP[targetIndex] = mStorage.mHP[sourceIndex];
		mStorage.mSpeed[targetIndex] = mStorage.mSpeed[sourceIndex];
		mStorage.mPositionX[targetIndex] = mStorage.mPositionX[sourceIndex];
		mStorage.mPositionY[targetIndex] = mStorage.mPositionY[sourceIndex];
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
	RoleDataStorage mStorage;
	int mCount = 0;
	int mCapacity = 0;
};
template<typename TKey, typename THash, typename TEqual>
class RoleDataECSDictionary
{
public:
	using SourceType = RoleData;
	using IndexMap = EasyECSIndexMap<TKey, THash, TEqual>;
	explicit RoleDataECSDictionary(int capacity = 4) : mIndexMap(capacity), mValues(capacity)
	{
		if (capacity < 1) capacity = 4;
		reserve(capacity);
	}
	RoleDataECSDictionary(const RoleDataECSDictionary& other) : mIndexMap(other.mIndexMap), mValues(other.mValues) {}
	RoleDataECSDictionary& operator=(const RoleDataECSDictionary& other)
	{
		if (this == &other) return *this;
		RoleDataECSDictionary copy(other);
		*this = std::move(copy);
		return *this;
	}
	RoleDataECSDictionary(RoleDataECSDictionary&& other) : mIndexMap(std::move(other.mIndexMap)), mValues(std::move(other.mValues)) {}
	RoleDataECSDictionary& operator=(RoleDataECSDictionary&& other)
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
	EASY_ECS_FORCE_INLINE RoleDataRef operator[](const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE RoleDataConstRef operator[](const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE RoleDataRef getValueByIndex(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE RoleDataConstRef getValueByIndex(int index) const { return mValues[index]; }
	EASY_ECS_FORCE_INLINE const TKey& getKeyByIndex(int index) const { return mIndexMap.getKeyByIndex(index); }
	EASY_ECS_FORCE_INLINE RoleDataRef valueAt(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE RoleDataConstRef valueAt(int index) const { return mValues[index]; }
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
		const RoleDataECSDictionary& readOnly = *this;
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
	EASY_ECS_FORCE_INLINE std::optional<RoleDataRef> tryGetRef(const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE std::optional<RoleDataConstRef> tryGetRef(const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE RoleDataECSList& getValues() { return mValues; }
	EASY_ECS_FORCE_INLINE const RoleDataECSList& getValues() const { return mValues; }
	EASY_ECS_FORCE_INLINE int* getHPColumn() { return mValues.getHPColumn(); }
	EASY_ECS_FORCE_INLINE const int* getHPColumn() const { return mValues.getHPColumn(); }
	EASY_ECS_FORCE_INLINE float* getSpeedColumn() { return mValues.getSpeedColumn(); }
	EASY_ECS_FORCE_INLINE const float* getSpeedColumn() const { return mValues.getSpeedColumn(); }
	EASY_ECS_FORCE_INLINE float* getPositionXColumn() { return mValues.getPositionXColumn(); }
	EASY_ECS_FORCE_INLINE const float* getPositionXColumn() const { return mValues.getPositionXColumn(); }
	EASY_ECS_FORCE_INLINE float* getPositionYColumn() { return mValues.getPositionYColumn(); }
	EASY_ECS_FORCE_INLINE const float* getPositionYColumn() const { return mValues.getPositionYColumn(); }
	EASY_ECS_FORCE_INLINE RoleDataAoSBlock* getAoSColumn() { return mValues.getAoSColumn(); }
	EASY_ECS_FORCE_INLINE const RoleDataAoSBlock* getAoSColumn() const { return mValues.getAoSColumn(); }
	bool add(const TKey& key, const RoleData& value)
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
	EASY_ECS_FORCE_INLINE bool tryAdd(const TKey& key, const RoleData& value) { return add(key, value); }
	int addRange(const TKey* keys, const RoleData* values, int count)
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
	bool build(const TKey* keys, const RoleData* values, int count)
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
	EASY_ECS_FORCE_INLINE std::pair<RoleDataRef, bool> getOrAdd(const TKey& key, const RoleData& defaultValue)
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
	EASY_ECS_FORCE_INLINE std::pair<RoleDataRef, bool> getOrAdd(const TKey& key) { return getOrAdd(key, RoleData{}); }
	void set(const TKey& key, const RoleData& value)
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
	bool tryGetValue(const TKey& key, RoleData& value) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return false;
		value = mValues.get(*index);
		return true;
	}
	RoleData get(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return mValues.get(*index); }
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
	RoleDataECSList mValues;
};


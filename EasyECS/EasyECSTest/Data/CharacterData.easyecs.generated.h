#pragma once
#include "CharacterData.h"
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
// Source ECS struct: EasyECSDemo::MonsterData
struct MonsterDataAoSBlock
{
	int mID;
};
struct MonsterDataStorage
{
	void* mRawMemory = nullptr;
	uint8_t* mAlignedMemory = nullptr;
	int* mHP = nullptr;
	float* mMoveSpeed = nullptr;
	MonsterDataAoSBlock* mAoS = nullptr;
};
struct MonsterDataRef
{
	int& mHP;
	float& mMoveSpeed;
	int& mID;
};
struct MonsterDataConstRef
{
	const int& mHP;
	const float& mMoveSpeed;
	const int& mID;
};
template<typename TKey, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>> class MonsterDataECSDictionary;
class MonsterDataECSList
{
public:
	using SourceType = MonsterData;
	explicit MonsterDataECSList(int capacity = 4);
	~MonsterDataECSList();
	MonsterDataECSList(const MonsterDataECSList& other);
	MonsterDataECSList& operator=(const MonsterDataECSList& other);
	MonsterDataECSList(MonsterDataECSList&& other) noexcept;
	MonsterDataECSList& operator=(MonsterDataECSList&& other) noexcept;
	EASY_ECS_FORCE_INLINE int size() const { return mCount; }
	EASY_ECS_FORCE_INLINE int capacity() const { return mCapacity; }
	EASY_ECS_FORCE_INLINE bool empty() const { return mCount == 0; }
	EASY_ECS_FORCE_INLINE MonsterDataRef operator[](int index)
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mHP[index],
			mStorage.mMoveSpeed[index],
			mStorage.mAoS[index].mID
		};
	}
	EASY_ECS_FORCE_INLINE MonsterDataConstRef operator[](int index) const
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mHP[index],
			mStorage.mMoveSpeed[index],
			mStorage.mAoS[index].mID
		};
	}
	EASY_ECS_FORCE_INLINE int* getHPColumn() { return mStorage.mHP; }
	EASY_ECS_FORCE_INLINE const int* getHPColumn() const { return mStorage.mHP; }
	EASY_ECS_FORCE_INLINE float* getMoveSpeedColumn() { return mStorage.mMoveSpeed; }
	EASY_ECS_FORCE_INLINE const float* getMoveSpeedColumn() const { return mStorage.mMoveSpeed; }
	EASY_ECS_FORCE_INLINE MonsterDataAoSBlock* getAoSColumn() { return mStorage.mAoS; }
	EASY_ECS_FORCE_INLINE const MonsterDataAoSBlock* getAoSColumn() const { return mStorage.mAoS; }
	void clear();
	void clearKeepCapacity();
	void clearAndRelease();
	void reserve(int capacity);
	void shrinkToFit();
	void add(const MonsterData& value);
	void addRange(const MonsterData* values, int count);
	MonsterDataRef addDefault();
	template<typename TPredicate> int removeAll(TPredicate&& predicate)
	{
		int oldSize = mCount;
		if (oldSize <= 0) return 0;
		int writeIndex = 0;
		int removeCount = 0;
		const MonsterDataECSList& readOnly = *this;
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
	void insert(int index, const MonsterData& value);
	void removeAt(int index);
	void removeAtSwapBack(int index);
	void popBack();
	MonsterData get(int index) const;
	void set(int index, const MonsterData& value);
private:
	template<typename TKey, typename THash, typename TEqual> friend class MonsterDataECSDictionary;
	void ensureCapacity(int requiredCapacity);
	void resizeCapacity(int newCapacity);
	void allocateStorage(MonsterDataStorage& storage, int capacity);
	void releaseStorage(MonsterDataStorage& storage);
	void copyStorage(MonsterDataStorage& target, const MonsterDataStorage& source, int count);
	void swap(MonsterDataECSList& other) noexcept;
	EASY_ECS_FORCE_INLINE void writeValue(int index, const MonsterData& value)
	{
		mStorage.mHP[index] = value.mHP;
		mStorage.mMoveSpeed[index] = value.mMoveSpeed;
		mStorage.mAoS[index].mID = value.mID;
	}
	EASY_ECS_FORCE_INLINE void copyValue(int targetIndex, int sourceIndex)
	{
		mStorage.mHP[targetIndex] = mStorage.mHP[sourceIndex];
		mStorage.mMoveSpeed[targetIndex] = mStorage.mMoveSpeed[sourceIndex];
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
	MonsterDataStorage mStorage;
	int mCount = 0;
	int mCapacity = 0;
};
template<typename TKey, typename THash, typename TEqual>
class MonsterDataECSDictionary
{
public:
	using SourceType = MonsterData;
	using IndexMap = EasyECSIndexMap<TKey, THash, TEqual>;
	explicit MonsterDataECSDictionary(int capacity = 4) : mIndexMap(capacity), mValues(capacity)
	{
		if (capacity < 1) capacity = 4;
		reserve(capacity);
	}
	MonsterDataECSDictionary(const MonsterDataECSDictionary& other) : mIndexMap(other.mIndexMap), mValues(other.mValues) {}
	MonsterDataECSDictionary& operator=(const MonsterDataECSDictionary& other)
	{
		if (this == &other) return *this;
		MonsterDataECSDictionary copy(other);
		*this = std::move(copy);
		return *this;
	}
	MonsterDataECSDictionary(MonsterDataECSDictionary&& other) : mIndexMap(std::move(other.mIndexMap)), mValues(std::move(other.mValues)) {}
	MonsterDataECSDictionary& operator=(MonsterDataECSDictionary&& other)
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
	EASY_ECS_FORCE_INLINE MonsterDataRef operator[](const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE MonsterDataConstRef operator[](const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE MonsterDataRef getValueByIndex(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE MonsterDataConstRef getValueByIndex(int index) const { return mValues[index]; }
	EASY_ECS_FORCE_INLINE const TKey& getKeyByIndex(int index) const { return mIndexMap.getKeyByIndex(index); }
	EASY_ECS_FORCE_INLINE MonsterDataRef valueAt(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE MonsterDataConstRef valueAt(int index) const { return mValues[index]; }
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
		const MonsterDataECSDictionary& readOnly = *this;
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
	EASY_ECS_FORCE_INLINE std::optional<MonsterDataRef> tryGetRef(const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE std::optional<MonsterDataConstRef> tryGetRef(const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE MonsterDataECSList& getValues() { return mValues; }
	EASY_ECS_FORCE_INLINE const MonsterDataECSList& getValues() const { return mValues; }
	EASY_ECS_FORCE_INLINE int* getHPColumn() { return mValues.getHPColumn(); }
	EASY_ECS_FORCE_INLINE const int* getHPColumn() const { return mValues.getHPColumn(); }
	EASY_ECS_FORCE_INLINE float* getMoveSpeedColumn() { return mValues.getMoveSpeedColumn(); }
	EASY_ECS_FORCE_INLINE const float* getMoveSpeedColumn() const { return mValues.getMoveSpeedColumn(); }
	EASY_ECS_FORCE_INLINE MonsterDataAoSBlock* getAoSColumn() { return mValues.getAoSColumn(); }
	EASY_ECS_FORCE_INLINE const MonsterDataAoSBlock* getAoSColumn() const { return mValues.getAoSColumn(); }
	bool add(const TKey& key, const MonsterData& value)
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
	EASY_ECS_FORCE_INLINE bool tryAdd(const TKey& key, const MonsterData& value) { return add(key, value); }
	int addRange(const TKey* keys, const MonsterData* values, int count)
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
	bool build(const TKey* keys, const MonsterData* values, int count)
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
	EASY_ECS_FORCE_INLINE std::pair<MonsterDataRef, bool> getOrAdd(const TKey& key, const MonsterData& defaultValue)
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
	EASY_ECS_FORCE_INLINE std::pair<MonsterDataRef, bool> getOrAdd(const TKey& key) { return getOrAdd(key, MonsterData{}); }
	void set(const TKey& key, const MonsterData& value)
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
	bool tryGetValue(const TKey& key, MonsterData& value) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return false;
		value = mValues.get(*index);
		return true;
	}
	MonsterData get(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return mValues.get(*index); }
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
	MonsterDataECSList mValues;
};
}

namespace EasyECSDemo
{
// Source ECS struct: EasyECSDemo::NPCData
struct NPCDataAoSBlock
{
	int mID;
};
struct NPCDataStorage
{
	void* mRawMemory = nullptr;
	uint8_t* mAlignedMemory = nullptr;
	int* mHP = nullptr;
	float* mTalkDistance = nullptr;
	NPCDataAoSBlock* mAoS = nullptr;
};
struct NPCDataRef
{
	int& mHP;
	float& mTalkDistance;
	int& mID;
};
struct NPCDataConstRef
{
	const int& mHP;
	const float& mTalkDistance;
	const int& mID;
};
template<typename TKey, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>> class NPCDataECSDictionary;
class NPCDataECSList
{
public:
	using SourceType = NPCData;
	explicit NPCDataECSList(int capacity = 4);
	~NPCDataECSList();
	NPCDataECSList(const NPCDataECSList& other);
	NPCDataECSList& operator=(const NPCDataECSList& other);
	NPCDataECSList(NPCDataECSList&& other) noexcept;
	NPCDataECSList& operator=(NPCDataECSList&& other) noexcept;
	EASY_ECS_FORCE_INLINE int size() const { return mCount; }
	EASY_ECS_FORCE_INLINE int capacity() const { return mCapacity; }
	EASY_ECS_FORCE_INLINE bool empty() const { return mCount == 0; }
	EASY_ECS_FORCE_INLINE NPCDataRef operator[](int index)
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mHP[index],
			mStorage.mTalkDistance[index],
			mStorage.mAoS[index].mID
		};
	}
	EASY_ECS_FORCE_INLINE NPCDataConstRef operator[](int index) const
	{
		assert(index >= 0 && index < mCount);
		return
		{
			mStorage.mHP[index],
			mStorage.mTalkDistance[index],
			mStorage.mAoS[index].mID
		};
	}
	EASY_ECS_FORCE_INLINE int* getHPColumn() { return mStorage.mHP; }
	EASY_ECS_FORCE_INLINE const int* getHPColumn() const { return mStorage.mHP; }
	EASY_ECS_FORCE_INLINE float* getTalkDistanceColumn() { return mStorage.mTalkDistance; }
	EASY_ECS_FORCE_INLINE const float* getTalkDistanceColumn() const { return mStorage.mTalkDistance; }
	EASY_ECS_FORCE_INLINE NPCDataAoSBlock* getAoSColumn() { return mStorage.mAoS; }
	EASY_ECS_FORCE_INLINE const NPCDataAoSBlock* getAoSColumn() const { return mStorage.mAoS; }
	void clear();
	void clearKeepCapacity();
	void clearAndRelease();
	void reserve(int capacity);
	void shrinkToFit();
	void add(const NPCData& value);
	void addRange(const NPCData* values, int count);
	NPCDataRef addDefault();
	template<typename TPredicate> int removeAll(TPredicate&& predicate)
	{
		int oldSize = mCount;
		if (oldSize <= 0) return 0;
		int writeIndex = 0;
		int removeCount = 0;
		const NPCDataECSList& readOnly = *this;
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
	void insert(int index, const NPCData& value);
	void removeAt(int index);
	void removeAtSwapBack(int index);
	void popBack();
	NPCData get(int index) const;
	void set(int index, const NPCData& value);
private:
	template<typename TKey, typename THash, typename TEqual> friend class NPCDataECSDictionary;
	void ensureCapacity(int requiredCapacity);
	void resizeCapacity(int newCapacity);
	void allocateStorage(NPCDataStorage& storage, int capacity);
	void releaseStorage(NPCDataStorage& storage);
	void copyStorage(NPCDataStorage& target, const NPCDataStorage& source, int count);
	void swap(NPCDataECSList& other) noexcept;
	EASY_ECS_FORCE_INLINE void writeValue(int index, const NPCData& value)
	{
		mStorage.mHP[index] = value.mHP;
		mStorage.mTalkDistance[index] = value.mTalkDistance;
		mStorage.mAoS[index].mID = value.mID;
	}
	EASY_ECS_FORCE_INLINE void copyValue(int targetIndex, int sourceIndex)
	{
		mStorage.mHP[targetIndex] = mStorage.mHP[sourceIndex];
		mStorage.mTalkDistance[targetIndex] = mStorage.mTalkDistance[sourceIndex];
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
	NPCDataStorage mStorage;
	int mCount = 0;
	int mCapacity = 0;
};
template<typename TKey, typename THash, typename TEqual>
class NPCDataECSDictionary
{
public:
	using SourceType = NPCData;
	using IndexMap = EasyECSIndexMap<TKey, THash, TEqual>;
	explicit NPCDataECSDictionary(int capacity = 4) : mIndexMap(capacity), mValues(capacity)
	{
		if (capacity < 1) capacity = 4;
		reserve(capacity);
	}
	NPCDataECSDictionary(const NPCDataECSDictionary& other) : mIndexMap(other.mIndexMap), mValues(other.mValues) {}
	NPCDataECSDictionary& operator=(const NPCDataECSDictionary& other)
	{
		if (this == &other) return *this;
		NPCDataECSDictionary copy(other);
		*this = std::move(copy);
		return *this;
	}
	NPCDataECSDictionary(NPCDataECSDictionary&& other) : mIndexMap(std::move(other.mIndexMap)), mValues(std::move(other.mValues)) {}
	NPCDataECSDictionary& operator=(NPCDataECSDictionary&& other)
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
	EASY_ECS_FORCE_INLINE NPCDataRef operator[](const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE NPCDataConstRef operator[](const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE NPCDataRef getValueByIndex(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE NPCDataConstRef getValueByIndex(int index) const { return mValues[index]; }
	EASY_ECS_FORCE_INLINE const TKey& getKeyByIndex(int index) const { return mIndexMap.getKeyByIndex(index); }
	EASY_ECS_FORCE_INLINE NPCDataRef valueAt(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE NPCDataConstRef valueAt(int index) const { return mValues[index]; }
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
		const NPCDataECSDictionary& readOnly = *this;
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
	EASY_ECS_FORCE_INLINE std::optional<NPCDataRef> tryGetRef(const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE std::optional<NPCDataConstRef> tryGetRef(const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE NPCDataECSList& getValues() { return mValues; }
	EASY_ECS_FORCE_INLINE const NPCDataECSList& getValues() const { return mValues; }
	EASY_ECS_FORCE_INLINE int* getHPColumn() { return mValues.getHPColumn(); }
	EASY_ECS_FORCE_INLINE const int* getHPColumn() const { return mValues.getHPColumn(); }
	EASY_ECS_FORCE_INLINE float* getTalkDistanceColumn() { return mValues.getTalkDistanceColumn(); }
	EASY_ECS_FORCE_INLINE const float* getTalkDistanceColumn() const { return mValues.getTalkDistanceColumn(); }
	EASY_ECS_FORCE_INLINE NPCDataAoSBlock* getAoSColumn() { return mValues.getAoSColumn(); }
	EASY_ECS_FORCE_INLINE const NPCDataAoSBlock* getAoSColumn() const { return mValues.getAoSColumn(); }
	bool add(const TKey& key, const NPCData& value)
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
	EASY_ECS_FORCE_INLINE bool tryAdd(const TKey& key, const NPCData& value) { return add(key, value); }
	int addRange(const TKey* keys, const NPCData* values, int count)
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
	bool build(const TKey* keys, const NPCData* values, int count)
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
	EASY_ECS_FORCE_INLINE std::pair<NPCDataRef, bool> getOrAdd(const TKey& key, const NPCData& defaultValue)
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
	EASY_ECS_FORCE_INLINE std::pair<NPCDataRef, bool> getOrAdd(const TKey& key) { return getOrAdd(key, NPCData{}); }
	void set(const TKey& key, const NPCData& value)
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
	bool tryGetValue(const TKey& key, NPCData& value) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return false;
		value = mValues.get(*index);
		return true;
	}
	NPCData get(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return mValues.get(*index); }
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
	NPCDataECSList mValues;
};
}

namespace EasyECSDemo
{
// Source ECS struct: EasyECSDemo::TypeData
struct TypeDataAoSBlock
{
	uint64_t mID;
};
struct TypeDataStorage
{
	void* mRawMemory = nullptr;
	uint8_t* mAlignedMemory = nullptr;
	unsigned* mUnsigned = nullptr;
	long long* mLongLong = nullptr;
	uint32_t* mUInt32 = nullptr;
	int64_t* mInt64 = nullptr;
	TypeDataState* mState = nullptr;
	TypeDataVector2* mPosition = nullptr;
	TypeDataAliasUInt* mAliasUInt = nullptr;
	TypeDataAliasInt64* mAliasInt64 = nullptr;
	TypeSupport::MoveState* mMoveState = nullptr;
	TypeSupport::Position* mPosition3 = nullptr;
	std::array<uint16_t, 4>* mFixedValues = nullptr;
	uint32_t* mAttributeValue = nullptr;
	uint32_t* mImmutable = nullptr;
	unsigned long long* mTailConst = nullptr;
	TypeDataAoSBlock* mAoS = nullptr;
};
struct TypeDataRef
{
	unsigned& mUnsigned;
	long long& mLongLong;
	uint32_t& mUInt32;
	int64_t& mInt64;
	TypeDataState& mState;
	TypeDataVector2& mPosition;
	TypeDataAliasUInt& mAliasUInt;
	TypeDataAliasInt64& mAliasInt64;
	TypeSupport::MoveState& mMoveState;
	TypeSupport::Position& mPosition3;
	std::array<uint16_t, 4>& mFixedValues;
	uint32_t& mAttributeValue;
	const uint32_t& mImmutable;
	const unsigned long long& mTailConst;
	uint64_t& mID;
};
struct TypeDataConstRef
{
	const unsigned& mUnsigned;
	const long long& mLongLong;
	const uint32_t& mUInt32;
	const int64_t& mInt64;
	const TypeDataState& mState;
	const TypeDataVector2& mPosition;
	const TypeDataAliasUInt& mAliasUInt;
	const TypeDataAliasInt64& mAliasInt64;
	const TypeSupport::MoveState& mMoveState;
	const TypeSupport::Position& mPosition3;
	const std::array<uint16_t, 4>& mFixedValues;
	const uint32_t& mAttributeValue;
	const uint32_t& mImmutable;
	const unsigned long long& mTailConst;
	const uint64_t& mID;
};
template<typename TKey, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>> class TypeDataECSDictionary;
class TypeDataECSList
{
public:
	using SourceType = TypeData;
	explicit TypeDataECSList(int capacity = 4);
	~TypeDataECSList();
	TypeDataECSList(const TypeDataECSList& other);
	TypeDataECSList& operator=(const TypeDataECSList& other);
	TypeDataECSList(TypeDataECSList&& other) noexcept;
	TypeDataECSList& operator=(TypeDataECSList&& other) noexcept;
	EASY_ECS_FORCE_INLINE int size() const { return mCount; }
	EASY_ECS_FORCE_INLINE int capacity() const { return mCapacity; }
	EASY_ECS_FORCE_INLINE bool empty() const { return mCount == 0; }
	EASY_ECS_FORCE_INLINE TypeDataRef operator[](int index)
	{
		assert(index >= 0 && index < mCount);
		return
		{
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
	EASY_ECS_FORCE_INLINE TypeDataConstRef operator[](int index) const
	{
		assert(index >= 0 && index < mCount);
		return
		{
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
	EASY_ECS_FORCE_INLINE unsigned* getUnsignedColumn() { return mStorage.mUnsigned; }
	EASY_ECS_FORCE_INLINE const unsigned* getUnsignedColumn() const { return mStorage.mUnsigned; }
	EASY_ECS_FORCE_INLINE long long* getLongLongColumn() { return mStorage.mLongLong; }
	EASY_ECS_FORCE_INLINE const long long* getLongLongColumn() const { return mStorage.mLongLong; }
	EASY_ECS_FORCE_INLINE uint32_t* getUInt32Column() { return mStorage.mUInt32; }
	EASY_ECS_FORCE_INLINE const uint32_t* getUInt32Column() const { return mStorage.mUInt32; }
	EASY_ECS_FORCE_INLINE int64_t* getInt64Column() { return mStorage.mInt64; }
	EASY_ECS_FORCE_INLINE const int64_t* getInt64Column() const { return mStorage.mInt64; }
	EASY_ECS_FORCE_INLINE TypeDataState* getStateColumn() { return mStorage.mState; }
	EASY_ECS_FORCE_INLINE const TypeDataState* getStateColumn() const { return mStorage.mState; }
	EASY_ECS_FORCE_INLINE TypeDataVector2* getPositionColumn() { return mStorage.mPosition; }
	EASY_ECS_FORCE_INLINE const TypeDataVector2* getPositionColumn() const { return mStorage.mPosition; }
	EASY_ECS_FORCE_INLINE TypeDataAliasUInt* getAliasUIntColumn() { return mStorage.mAliasUInt; }
	EASY_ECS_FORCE_INLINE const TypeDataAliasUInt* getAliasUIntColumn() const { return mStorage.mAliasUInt; }
	EASY_ECS_FORCE_INLINE TypeDataAliasInt64* getAliasInt64Column() { return mStorage.mAliasInt64; }
	EASY_ECS_FORCE_INLINE const TypeDataAliasInt64* getAliasInt64Column() const { return mStorage.mAliasInt64; }
	EASY_ECS_FORCE_INLINE TypeSupport::MoveState* getMoveStateColumn() { return mStorage.mMoveState; }
	EASY_ECS_FORCE_INLINE const TypeSupport::MoveState* getMoveStateColumn() const { return mStorage.mMoveState; }
	EASY_ECS_FORCE_INLINE TypeSupport::Position* getPosition3Column() { return mStorage.mPosition3; }
	EASY_ECS_FORCE_INLINE const TypeSupport::Position* getPosition3Column() const { return mStorage.mPosition3; }
	EASY_ECS_FORCE_INLINE std::array<uint16_t, 4>* getFixedValuesColumn() { return mStorage.mFixedValues; }
	EASY_ECS_FORCE_INLINE const std::array<uint16_t, 4>* getFixedValuesColumn() const { return mStorage.mFixedValues; }
	EASY_ECS_FORCE_INLINE uint32_t* getAttributeValueColumn() { return mStorage.mAttributeValue; }
	EASY_ECS_FORCE_INLINE const uint32_t* getAttributeValueColumn() const { return mStorage.mAttributeValue; }
	EASY_ECS_FORCE_INLINE const uint32_t* getImmutableColumn() { return mStorage.mImmutable; }
	EASY_ECS_FORCE_INLINE const uint32_t* getImmutableColumn() const { return mStorage.mImmutable; }
	EASY_ECS_FORCE_INLINE const unsigned long long* getTailConstColumn() { return mStorage.mTailConst; }
	EASY_ECS_FORCE_INLINE const unsigned long long* getTailConstColumn() const { return mStorage.mTailConst; }
	EASY_ECS_FORCE_INLINE TypeDataAoSBlock* getAoSColumn() { return mStorage.mAoS; }
	EASY_ECS_FORCE_INLINE const TypeDataAoSBlock* getAoSColumn() const { return mStorage.mAoS; }
	void clear();
	void clearKeepCapacity();
	void clearAndRelease();
	void reserve(int capacity);
	void shrinkToFit();
	void add(const TypeData& value);
	void addRange(const TypeData* values, int count);
	TypeDataRef addDefault();
	template<typename TPredicate> int removeAll(TPredicate&& predicate)
	{
		int oldSize = mCount;
		if (oldSize <= 0) return 0;
		int writeIndex = 0;
		int removeCount = 0;
		const TypeDataECSList& readOnly = *this;
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
	void insert(int index, const TypeData& value);
	void removeAt(int index);
	void removeAtSwapBack(int index);
	void popBack();
	TypeData get(int index) const;
	void set(int index, const TypeData& value);
private:
	template<typename TKey, typename THash, typename TEqual> friend class TypeDataECSDictionary;
	void ensureCapacity(int requiredCapacity);
	void resizeCapacity(int newCapacity);
	void allocateStorage(TypeDataStorage& storage, int capacity);
	void releaseStorage(TypeDataStorage& storage);
	void copyStorage(TypeDataStorage& target, const TypeDataStorage& source, int count);
	void swap(TypeDataECSList& other) noexcept;
	EASY_ECS_FORCE_INLINE void writeValue(int index, const TypeData& value)
	{
		mStorage.mUnsigned[index] = value.mUnsigned;
		mStorage.mLongLong[index] = value.mLongLong;
		mStorage.mUInt32[index] = value.mUInt32;
		mStorage.mInt64[index] = value.mInt64;
		mStorage.mState[index] = value.mState;
		mStorage.mPosition[index] = value.mPosition;
		mStorage.mAliasUInt[index] = value.mAliasUInt;
		mStorage.mAliasInt64[index] = value.mAliasInt64;
		mStorage.mMoveState[index] = value.mMoveState;
		mStorage.mPosition3[index] = value.mPosition3;
		mStorage.mFixedValues[index] = value.mFixedValues;
		mStorage.mAttributeValue[index] = value.mAttributeValue;
		mStorage.mImmutable[index] = value.mImmutable;
		mStorage.mTailConst[index] = value.mTailConst;
		mStorage.mAoS[index].mID = value.mID;
	}
	EASY_ECS_FORCE_INLINE void copyValue(int targetIndex, int sourceIndex)
	{
		mStorage.mUnsigned[targetIndex] = mStorage.mUnsigned[sourceIndex];
		mStorage.mLongLong[targetIndex] = mStorage.mLongLong[sourceIndex];
		mStorage.mUInt32[targetIndex] = mStorage.mUInt32[sourceIndex];
		mStorage.mInt64[targetIndex] = mStorage.mInt64[sourceIndex];
		mStorage.mState[targetIndex] = mStorage.mState[sourceIndex];
		mStorage.mPosition[targetIndex] = mStorage.mPosition[sourceIndex];
		mStorage.mAliasUInt[targetIndex] = mStorage.mAliasUInt[sourceIndex];
		mStorage.mAliasInt64[targetIndex] = mStorage.mAliasInt64[sourceIndex];
		mStorage.mMoveState[targetIndex] = mStorage.mMoveState[sourceIndex];
		mStorage.mPosition3[targetIndex] = mStorage.mPosition3[sourceIndex];
		mStorage.mFixedValues[targetIndex] = mStorage.mFixedValues[sourceIndex];
		mStorage.mAttributeValue[targetIndex] = mStorage.mAttributeValue[sourceIndex];
		mStorage.mImmutable[targetIndex] = mStorage.mImmutable[sourceIndex];
		mStorage.mTailConst[targetIndex] = mStorage.mTailConst[sourceIndex];
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
	TypeDataStorage mStorage;
	int mCount = 0;
	int mCapacity = 0;
};
template<typename TKey, typename THash, typename TEqual>
class TypeDataECSDictionary
{
public:
	using SourceType = TypeData;
	using IndexMap = EasyECSIndexMap<TKey, THash, TEqual>;
	explicit TypeDataECSDictionary(int capacity = 4) : mIndexMap(capacity), mValues(capacity)
	{
		if (capacity < 1) capacity = 4;
		reserve(capacity);
	}
	TypeDataECSDictionary(const TypeDataECSDictionary& other) : mIndexMap(other.mIndexMap), mValues(other.mValues) {}
	TypeDataECSDictionary& operator=(const TypeDataECSDictionary& other)
	{
		if (this == &other) return *this;
		TypeDataECSDictionary copy(other);
		*this = std::move(copy);
		return *this;
	}
	TypeDataECSDictionary(TypeDataECSDictionary&& other) : mIndexMap(std::move(other.mIndexMap)), mValues(std::move(other.mValues)) {}
	TypeDataECSDictionary& operator=(TypeDataECSDictionary&& other)
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
	EASY_ECS_FORCE_INLINE TypeDataRef operator[](const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE TypeDataConstRef operator[](const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		assert(index != nullptr);
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE TypeDataRef getValueByIndex(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE TypeDataConstRef getValueByIndex(int index) const { return mValues[index]; }
	EASY_ECS_FORCE_INLINE const TKey& getKeyByIndex(int index) const { return mIndexMap.getKeyByIndex(index); }
	EASY_ECS_FORCE_INLINE TypeDataRef valueAt(int index) { return mValues[index]; }
	EASY_ECS_FORCE_INLINE TypeDataConstRef valueAt(int index) const { return mValues[index]; }
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
		const TypeDataECSDictionary& readOnly = *this;
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
	EASY_ECS_FORCE_INLINE std::optional<TypeDataRef> tryGetRef(const TKey& key)
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE std::optional<TypeDataConstRef> tryGetRef(const TKey& key) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return std::nullopt;
		return mValues[*index];
	}
	EASY_ECS_FORCE_INLINE TypeDataECSList& getValues() { return mValues; }
	EASY_ECS_FORCE_INLINE const TypeDataECSList& getValues() const { return mValues; }
	EASY_ECS_FORCE_INLINE unsigned* getUnsignedColumn() { return mValues.getUnsignedColumn(); }
	EASY_ECS_FORCE_INLINE const unsigned* getUnsignedColumn() const { return mValues.getUnsignedColumn(); }
	EASY_ECS_FORCE_INLINE long long* getLongLongColumn() { return mValues.getLongLongColumn(); }
	EASY_ECS_FORCE_INLINE const long long* getLongLongColumn() const { return mValues.getLongLongColumn(); }
	EASY_ECS_FORCE_INLINE uint32_t* getUInt32Column() { return mValues.getUInt32Column(); }
	EASY_ECS_FORCE_INLINE const uint32_t* getUInt32Column() const { return mValues.getUInt32Column(); }
	EASY_ECS_FORCE_INLINE int64_t* getInt64Column() { return mValues.getInt64Column(); }
	EASY_ECS_FORCE_INLINE const int64_t* getInt64Column() const { return mValues.getInt64Column(); }
	EASY_ECS_FORCE_INLINE TypeDataState* getStateColumn() { return mValues.getStateColumn(); }
	EASY_ECS_FORCE_INLINE const TypeDataState* getStateColumn() const { return mValues.getStateColumn(); }
	EASY_ECS_FORCE_INLINE TypeDataVector2* getPositionColumn() { return mValues.getPositionColumn(); }
	EASY_ECS_FORCE_INLINE const TypeDataVector2* getPositionColumn() const { return mValues.getPositionColumn(); }
	EASY_ECS_FORCE_INLINE TypeDataAliasUInt* getAliasUIntColumn() { return mValues.getAliasUIntColumn(); }
	EASY_ECS_FORCE_INLINE const TypeDataAliasUInt* getAliasUIntColumn() const { return mValues.getAliasUIntColumn(); }
	EASY_ECS_FORCE_INLINE TypeDataAliasInt64* getAliasInt64Column() { return mValues.getAliasInt64Column(); }
	EASY_ECS_FORCE_INLINE const TypeDataAliasInt64* getAliasInt64Column() const { return mValues.getAliasInt64Column(); }
	EASY_ECS_FORCE_INLINE TypeSupport::MoveState* getMoveStateColumn() { return mValues.getMoveStateColumn(); }
	EASY_ECS_FORCE_INLINE const TypeSupport::MoveState* getMoveStateColumn() const { return mValues.getMoveStateColumn(); }
	EASY_ECS_FORCE_INLINE TypeSupport::Position* getPosition3Column() { return mValues.getPosition3Column(); }
	EASY_ECS_FORCE_INLINE const TypeSupport::Position* getPosition3Column() const { return mValues.getPosition3Column(); }
	EASY_ECS_FORCE_INLINE std::array<uint16_t, 4>* getFixedValuesColumn() { return mValues.getFixedValuesColumn(); }
	EASY_ECS_FORCE_INLINE const std::array<uint16_t, 4>* getFixedValuesColumn() const { return mValues.getFixedValuesColumn(); }
	EASY_ECS_FORCE_INLINE uint32_t* getAttributeValueColumn() { return mValues.getAttributeValueColumn(); }
	EASY_ECS_FORCE_INLINE const uint32_t* getAttributeValueColumn() const { return mValues.getAttributeValueColumn(); }
	EASY_ECS_FORCE_INLINE const uint32_t* getImmutableColumn() { return mValues.getImmutableColumn(); }
	EASY_ECS_FORCE_INLINE const uint32_t* getImmutableColumn() const { return mValues.getImmutableColumn(); }
	EASY_ECS_FORCE_INLINE const unsigned long long* getTailConstColumn() { return mValues.getTailConstColumn(); }
	EASY_ECS_FORCE_INLINE const unsigned long long* getTailConstColumn() const { return mValues.getTailConstColumn(); }
	EASY_ECS_FORCE_INLINE TypeDataAoSBlock* getAoSColumn() { return mValues.getAoSColumn(); }
	EASY_ECS_FORCE_INLINE const TypeDataAoSBlock* getAoSColumn() const { return mValues.getAoSColumn(); }
	bool add(const TKey& key, const TypeData& value)
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
	EASY_ECS_FORCE_INLINE bool tryAdd(const TKey& key, const TypeData& value) { return add(key, value); }
	int addRange(const TKey* keys, const TypeData* values, int count)
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
	bool build(const TKey* keys, const TypeData* values, int count)
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
	EASY_ECS_FORCE_INLINE std::pair<TypeDataRef, bool> getOrAdd(const TKey& key, const TypeData& defaultValue)
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
	EASY_ECS_FORCE_INLINE std::pair<TypeDataRef, bool> getOrAdd(const TKey& key) { return getOrAdd(key, TypeData{}); }
	void set(const TKey& key, const TypeData& value)
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
	bool tryGetValue(const TKey& key, TypeData& value) const
	{
		const int* index = mIndexMap.findIndex(key);
		if (index == nullptr) return false;
		TypeData newValue = mValues.get(*index);
		value.~TypeData();
		new (&value) TypeData(newValue);
		return true;
	}
	TypeData get(const TKey& key) const { const int* index = mIndexMap.findIndex(key); assert(index != nullptr); return mValues.get(*index); }
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
	TypeDataECSList mValues;
};
}


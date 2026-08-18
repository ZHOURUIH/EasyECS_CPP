#include "RoleDataDictionaryBenchmark.h"
#include "Data/EasyECS.generated.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>
#undef max
#undef min

using namespace std;

static constexpr int ENTITY_COUNT = 500000;
static constexpr int STRUCTURAL_COUNT = 100000;
static constexpr int SAMPLE_COUNT = 15;
static constexpr int WARMUP_COUNT = 3;
static constexpr int CHURN_ENTITY_COUNT = 100000;
static constexpr int CHURN_BATCH_COUNT = 10000;
static constexpr int CHURN_ROUND_COUNT = 50;
static constexpr int FUZZ_NORMAL_OPERATION_COUNT = 600000;
static constexpr int FUZZ_COLLISION_OPERATION_COUNT = 300000;
static constexpr int FUZZ_CONSTANT_OPERATION_COUNT = 100000;
static constexpr int FUZZ_TOTAL_OPERATION_COUNT = FUZZ_NORMAL_OPERATION_COUNT + FUZZ_COLLISION_OPERATION_COUNT + FUZZ_CONSTANT_OPERATION_COUNT;
static constexpr int FUZZ_VALIDATE_INTERVAL = 64;
static volatile double gDictionarySink = 0.0;
struct BenchmarkResult
{
	double mMedian;
	double mMin;
	double mMax;
	double mNsPerOperation;
};
struct CollisionHash
{
	size_t operator()(int value) const { return static_cast<size_t>(value & 7); }
};
struct MixedHash
{
	size_t operator()(int value) const
	{
		uint32_t x = static_cast<uint32_t>(value);
		x ^= x >> 16;
		x *= 0x7FEB352Du;
		x ^= x >> 15;
		x *= 0x846CA68Bu;
		x ^= x >> 16;
		return static_cast<size_t>(x);
	}
};
struct ConstantHash
{
	size_t operator()(int) const { return 0; }
};
template<typename Func>
BenchmarkResult runBenchmark(int operationCount, Func func)
{
	for (int i = 0; i < WARMUP_COUNT; ++i) func();
	vector<double> samples;
	samples.reserve(SAMPLE_COUNT);
	for (int i = 0; i < SAMPLE_COUNT; ++i)
	{
		auto start = chrono::high_resolution_clock::now();
		func();
		auto end = chrono::high_resolution_clock::now();
		samples.push_back(chrono::duration<double, milli>(end - start).count());
	}
	sort(samples.begin(), samples.end());
	double median = samples[SAMPLE_COUNT / 2];
	return { median, samples.front(), samples.back(), median * 1000000.0 / operationCount };
}
template<typename Setup, typename Func>
BenchmarkResult runPreparedBenchmark(int operationCount, Setup setup, Func func)
{
	for (int i = 0; i < WARMUP_COUNT; ++i)
	{
		setup();
		func();
	}
	vector<double> samples;
	samples.reserve(SAMPLE_COUNT);
	for (int i = 0; i < SAMPLE_COUNT; ++i)
	{
		setup();
		auto start = chrono::high_resolution_clock::now();
		func();
		auto end = chrono::high_resolution_clock::now();
		samples.push_back(chrono::duration<double, milli>(end - start).count());
	}
	sort(samples.begin(), samples.end());
	double median = samples[SAMPLE_COUNT / 2];
	return { median, samples.front(), samples.back(), median * 1000000.0 / operationCount };
}
static void printResult(const char* name, const BenchmarkResult& result)
{
	printf("%-30s Median:%9.3f ms | Min:%8.3f | Max:%8.3f | %9.3f ns/op\n", name, result.mMedian, result.mMin, result.mMax, result.mNsPerOperation);
}
static void printRatio(const char* name, double source, double target)
{
	printf("%-32s: %.2fx\n", name, source / target);
}

static void runChurnBenchmark()
{
	printf("\n================ IndexMap长期Churn(删除+新增) ================\n");
	printf("ChurnEntityCount:%d BatchCount:%d RoundCount:%d\n", CHURN_ENTITY_COUNT, CHURN_BATCH_COUNT, CHURN_ROUND_COUNT);
	unordered_map<int, int, MixedHash> stdMap;
	stdMap.reserve(CHURN_ENTITY_COUNT);
	EasyECSIndexMap<int, MixedHash> flatMap(CHURN_ENTITY_COUNT);
	const int operationCount = CHURN_BATCH_COUNT * CHURN_ROUND_COUNT * 2;
	auto stdChurn = runPreparedBenchmark(operationCount, [&]()
	{
		stdMap.clear();
		for (int i = 0; i < CHURN_ENTITY_COUNT; ++i) stdMap.emplace(i, i);
	}, [&]()
	{
		for (int round = 0; round < CHURN_ROUND_COUNT; ++round)
		{
			int removeBegin = round * CHURN_BATCH_COUNT;
			int addBegin = CHURN_ENTITY_COUNT + round * CHURN_BATCH_COUNT;
			for (int i = 0; i < CHURN_BATCH_COUNT; ++i) stdMap.erase(removeBegin + i);
			for (int i = 0; i < CHURN_BATCH_COUNT; ++i) stdMap.emplace(addBegin + i, addBegin + i);
		}
		gDictionarySink += static_cast<double>(stdMap.size());
	});
	auto flatChurn = runPreparedBenchmark(operationCount, [&]()
	{
		flatMap.clear();
		for (int i = 0; i < CHURN_ENTITY_COUNT; ++i) flatMap.tryAdd(i, flatMap.size());
	}, [&]()
	{
		for (int round = 0; round < CHURN_ROUND_COUNT; ++round)
		{
			int removeBegin = round * CHURN_BATCH_COUNT;
			int addBegin = CHURN_ENTITY_COUNT + round * CHURN_BATCH_COUNT;
			for (int i = 0; i < CHURN_BATCH_COUNT; ++i)
			{
				int removedIndex = -1;
				flatMap.erase(removeBegin + i, removedIndex);
			}
			for (int i = 0; i < CHURN_BATCH_COUNT; ++i) flatMap.tryAdd(addBegin + i, flatMap.size());
		}
		gDictionarySink += static_cast<double>(flatMap.size());
	});
	printResult("unordered_map Churn", stdChurn);
	printResult("EasyECSIndexMap Churn", flatChurn);
	printRatio("unordered_map / FlatIndexMap", stdChurn.mMedian, flatChurn.mMedian);
	EasyECSIndexMap<int, MixedHash, equal_to<int>, true> statsMap(CHURN_ENTITY_COUNT);
	for (int i = 0; i < CHURN_ENTITY_COUNT; ++i) statsMap.tryAdd(i, statsMap.size());
	statsMap.resetStats();
	for (int round = 0; round < CHURN_ROUND_COUNT; ++round)
	{
		int removeBegin = round * CHURN_BATCH_COUNT;
		int addBegin = CHURN_ENTITY_COUNT + round * CHURN_BATCH_COUNT;
		for (int i = 0; i < CHURN_BATCH_COUNT; ++i)
		{
			int removedIndex = -1;
			statsMap.erase(removeBegin + i, removedIndex);
		}
		for (int i = 0; i < CHURN_BATCH_COUNT; ++i) statsMap.tryAdd(addBegin + i, statsMap.size());
	}
	const EasyECSIndexMapStats& stats = statsMap.getStats();
	printf("Flat Churn Stats: AvgFindProbe:%.3f MaxFindProbe:%llu AvgInsertProbe:%.3f MaxInsertProbe:%llu "
		"TombstoneProbe:%llu Rehash:%llu FinalTombstone:%d Capacity:%d\n",
		stats.getAverageFindProbe(), static_cast<unsigned long long>(stats.mMaxFindProbe), stats.getAverageInsertProbe(),
		static_cast<unsigned long long>(stats.mMaxInsertProbe), static_cast<unsigned long long>(stats.mTombstoneProbeCount),
		static_cast<unsigned long long>(stats.mRehashCount), statsMap.tombstoneCount(), statsMap.capacity());
}
static RoleData createRoleData(int i)
{
	RoleData data;
	data.mHP = 100 + i % 100;
	data.mSpeed = 1.0f + static_cast<float>(i % 10) * 0.1f;
	data.mPositionX = static_cast<float>(i);
	data.mPositionY = static_cast<float>(i) * 0.5f;
	data.mID = i;
	data.mModelID = i % 100;
	data.mCamp = i % 3;
	return data;
}
struct FuzzRandom
{
	uint64_t mState;
	explicit FuzzRandom(uint64_t seed) : mState(seed) {}
	uint32_t nextUInt()
	{
		uint64_t z = (mState += 0x9E3779B97F4A7C15ull);
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
		z ^= z >> 31;
		return static_cast<uint32_t>(z >> 32);
	}
	int nextInt(int maxExclusive) { return maxExclusive > 0 ? static_cast<int>(nextUInt() % static_cast<uint32_t>(maxExclusive)) : 0; }
};
struct DictionaryFuzzResult
{
	bool mPass = true;
	int mFailureStep = -1;
	int mFailureOperation = -1;
	int mFinalSize = 0;
	int mValidateCount = 0;
	const char* mFailureReason = nullptr;
};
static RoleData createFuzzRoleData(int serial, int key)
{
	RoleData data;
	data.mHP = serial * 17 + key * 3 + 11;
	data.mSpeed = static_cast<float>((serial & 1023) - 512);
	data.mPositionX = static_cast<float>(serial & 0x7FFFFF);
	data.mPositionY = static_cast<float>(key * 4 - 2048);
	data.mID = serial;
	data.mModelID = key;
	data.mCamp = (serial + key) & 7;
	return data;
}
static bool fuzzRoleEquals(RoleDataConstRef value, const RoleData& expected)
{
	return value.mHP == expected.mHP && value.mSpeed == expected.mSpeed && value.mPositionX == expected.mPositionX &&
		value.mPositionY == expected.mPositionY && value.mID == expected.mID && value.mModelID == expected.mModelID && value.mCamp == expected.mCamp;
}
static bool fuzzRoleEquals(RoleDataRef value, const RoleData& expected)
{
	return value.mHP == expected.mHP && value.mSpeed == expected.mSpeed && value.mPositionX == expected.mPositionX &&
		value.mPositionY == expected.mPositionY && value.mID == expected.mID && value.mModelID == expected.mModelID && value.mCamp == expected.mCamp;
}
static bool fuzzRoleEquals(const RoleData& value, const RoleData& expected)
{
	return value.mHP == expected.mHP && value.mSpeed == expected.mSpeed && value.mPositionX == expected.mPositionX &&
		value.mPositionY == expected.mPositionY && value.mID == expected.mID && value.mModelID == expected.mModelID && value.mCamp == expected.mCamp;
}
static const char* getFuzzOperationName(int operation)
{
	switch (operation)
	{
	case 0: return "add";
	case 1: return "tryAdd";
	case 2: return "set";
	case 3: return "getOrAdd";
	case 4: return "remove";
	case 5: return "removeByIndex";
	case 6: return "removeBatch";
	case 7: return "removeByIndexBatch";
	case 8: return "removeAll";
	case 9: return "addRange";
	case 10: return "build";
	case 11: return "reserve";
	case 12: return "shrinkToFit";
	case 13: return "clearKeepCapacity";
	case 14: return "clearAndRelease";
	case 15: return "copyMove";
	case 16: return "tryGetRef";
	default: return "unknown";
	}
}
static int selectFuzzOperation(int randomValue)
{
	if (randomValue < 14) return 0;
	if (randomValue < 21) return 1;
	if (randomValue < 31) return 2;
	if (randomValue < 40) return 3;
	if (randomValue < 50) return 4;
	if (randomValue < 55) return 5;
	if (randomValue < 62) return 6;
	if (randomValue < 67) return 7;
	if (randomValue < 72) return 8;
	if (randomValue < 78) return 9;
	if (randomValue < 80) return 10;
	if (randomValue < 84) return 11;
	if (randomValue < 88) return 12;
	if (randomValue < 89) return 13;
	if (randomValue < 90) return 14;
	if (randomValue < 93) return 15;
	return 16;
}
template<typename TDictionary>
static bool validateFuzzDictionary(const TDictionary& dictionary, const unordered_map<int, RoleData>& reference, const char*& reason)
{
	if (dictionary.size() != static_cast<int>(reference.size()))
	{
		reason = "size mismatch";
		return false;
	}
	if (dictionary.capacity() < dictionary.size())
	{
		reason = "value capacity smaller than size";
		return false;
	}
	if (!dictionary.empty() && dictionary.indexCapacity() <= 0)
	{
		reason = "non-empty dictionary has no index capacity";
		return false;
	}
	for (int i = 0; i < dictionary.size(); ++i)
	{
		int key = dictionary.keyAt(i);
		auto referenceIt = reference.find(key);
		if (referenceIt == reference.end())
		{
			reason = "dense key missing from reference";
			return false;
		}
		int foundIndex = -1;
		if (!dictionary.tryGetIndex(key, foundIndex) || foundIndex != i)
		{
			reason = "key/index mapping mismatch";
			return false;
		}
		if (!fuzzRoleEquals(dictionary.valueAt(i), referenceIt->second))
		{
			reason = "dense value mismatch";
			return false;
		}
	}
	for (const auto& pair : reference)
	{
		if (!dictionary.containsKey(pair.first))
		{
			reason = "reference key missing from dictionary";
			return false;
		}
		RoleData value;
		if (!dictionary.tryGetValue(pair.first, value) || !fuzzRoleEquals(value, pair.second))
		{
			reason = "key lookup value mismatch";
			return false;
		}
	}
	reason = nullptr;
	return true;
}
template<typename TDictionary>
static DictionaryFuzzResult runDictionaryFuzzCase(int operationCount, int keySpace, uint64_t seed)
{
	TDictionary dictionary;
	unordered_map<int, RoleData> reference;
	reference.reserve(static_cast<size_t>(keySpace));
	FuzzRandom random(seed);
	DictionaryFuzzResult result;
	auto fail = [&](int step, int operation, const char* reason)
	{
		result.mPass = false;
		result.mFailureStep = step;
		result.mFailureOperation = operation;
		result.mFailureReason = reason;
		result.mFinalSize = dictionary.size();
		return result;
	};
	auto validate = [&](int step, int operation) -> bool
	{
		const char* reason = nullptr;
		++result.mValidateCount;
		if (validateFuzzDictionary(dictionary, reference, reason)) return true;
		result = fail(step, operation, reason);
		return false;
	};
	for (int step = 0; step < operationCount; ++step)
	{
		int operation = selectFuzzOperation(random.nextInt(100));
		int key = random.nextInt(keySpace);
		bool validateNow = false;
		switch (operation)
		{
		case 0:
		case 1:
		{
			RoleData value = createFuzzRoleData(step + 1, key);
			bool ecsAdded = operation == 0 ? dictionary.add(key, value) : dictionary.tryAdd(key, value);
			bool referenceAdded = reference.emplace(key, value).second;
			if (ecsAdded != referenceAdded) return fail(step, operation, "add result mismatch");
			break;
		}
		case 2:
		{
			RoleData value = createFuzzRoleData(step + 1000001, key);
			dictionary.set(key, value);
			reference[key] = value;
			break;
		}
		case 3:
		{
			bool useDefaultValue = (random.nextUInt() & 1u) != 0;
			if (useDefaultValue)
			{
				RoleData defaultValue = createFuzzRoleData(step + 2000001, key);
				auto ecsResult = dictionary.getOrAdd(key, defaultValue);
				auto referenceIt = reference.find(key);
				bool referenceAdded = referenceIt == reference.end();
				if (referenceAdded) referenceIt = reference.emplace(key, defaultValue).first;
				if (ecsResult.second != referenceAdded || !fuzzRoleEquals(ecsResult.first, referenceIt->second)) return fail(step, operation, "getOrAdd result mismatch");
				ecsResult.first.mHP += 1;
				referenceIt->second.mHP += 1;
			}
			else
			{
				auto ecsResult = dictionary.getOrAdd(key);
				auto referenceIt = reference.find(key);
				bool referenceAdded = referenceIt == reference.end();
				if (referenceAdded) referenceIt = reference.emplace(key, RoleData{}).first;
				if (ecsResult.second != referenceAdded ||
					!fuzzRoleEquals(ecsResult.first, referenceIt->second)) return fail(step, operation, "default getOrAdd result mismatch");
				ecsResult.first.mHP += 1;
				referenceIt->second.mHP += 1;
			}
			break;
		}
		case 4:
		{
			bool ecsRemoved = dictionary.remove(key);
			bool referenceRemoved = reference.erase(key) != 0;
			if (ecsRemoved != referenceRemoved) return fail(step, operation, "remove result mismatch");
			break;
		}
		case 5:
		{
			if (dictionary.empty())
			{
				if (dictionary.removeByIndex(0)) return fail(step, operation, "empty removeByIndex succeeded");
				break;
			}
			int removeIndex = random.nextInt(dictionary.size());
			int removeKey = dictionary.keyAt(removeIndex);
			if (!dictionary.removeByIndex(removeIndex)) return fail(step, operation, "valid removeByIndex failed");
			if (reference.erase(removeKey) != 1) return fail(step, operation, "removeByIndex reference key missing");
			break;
		}
		case 6:
		{
			int keys[8]{};
			int count = 1 + random.nextInt(8);
			int expectedRemoved = 0;
			for (int i = 0; i < count; ++i)
			{
				keys[i] = random.nextInt(keySpace + 16);
				expectedRemoved += reference.erase(keys[i]) != 0 ? 1 : 0;
			}
			int removed = dictionary.removeBatch(keys, count);
			if (removed != expectedRemoved) return fail(step, operation, "removeBatch count mismatch");
			validateNow = true;
			break;
		}
		case 7:
		{
			int indices[8]{};
			int count = 1 + random.nextInt(8);
			vector<int> removeKeys;
			removeKeys.reserve(static_cast<size_t>(count));
			vector<int> uniqueIndices;
			uniqueIndices.reserve(static_cast<size_t>(count));
			int oldSize = dictionary.size();
			for (int i = 0; i < count; ++i)
			{
				int mode = random.nextInt(5);
				if (mode == 0) indices[i] = -1;
				else if (mode == 1) indices[i] = oldSize + random.nextInt(4) + 1;
				else indices[i] = oldSize > 0 ? random.nextInt(oldSize) : 0;
				if (indices[i] < 0 || indices[i] >= oldSize) continue;
				bool duplicate = false;
				for (int existingIndex : uniqueIndices) if (existingIndex == indices[i]) { duplicate = true; break; }
				if (duplicate) continue;
				uniqueIndices.push_back(indices[i]);
				removeKeys.push_back(dictionary.keyAt(indices[i]));
			}
			int removed = dictionary.removeByIndexBatch(indices, count);
			if (removed != static_cast<int>(removeKeys.size())) return fail(step, operation, "removeByIndexBatch count mismatch");
			for (int removeKey : removeKeys) if (reference.erase(removeKey) != 1) return fail(step, operation, "removeByIndexBatch reference key missing");
			validateNow = true;
			break;
		}
		case 8:
		{
			int divisor = 2 + random.nextInt(7);
			int remainder = random.nextInt(divisor);
			int token = random.nextInt(11);
			int expectedRemoved = 0;
			for (auto it = reference.begin(); it != reference.end();)
			{
				if (((it->first + it->second.mCamp + token) % divisor) != remainder)
				{
					++it;
					continue;
				}
				it = reference.erase(it);
				++expectedRemoved;
			}
			int removed = dictionary.removeAll([&](const int& removeKey, RoleDataConstRef value) { return ((removeKey + value.mCamp + token) % divisor) == remainder; });
			if (removed != expectedRemoved) return fail(step, operation, "removeAll count mismatch");
			validateNow = true;
			break;
		}
		case 9:
		{
			int keys[8]{};
			RoleData values[8]{};
			int count = 1 + random.nextInt(8);
			int expectedAdded = 0;
			for (int i = 0; i < count; ++i)
			{
				keys[i] = random.nextInt(keySpace);
				values[i] = createFuzzRoleData(step * 8 + i + 3000001, keys[i]);
				expectedAdded += reference.emplace(keys[i], values[i]).second ? 1 : 0;
			}
			int added = dictionary.addRange(keys, values, count);
			if (added != expectedAdded) return fail(step, operation, "addRange count mismatch");
			validateNow = true;
			break;
		}
		case 10:
		{
			int keys[16]{};
			RoleData values[16]{};
			int count = random.nextInt(17);
			bool forceDuplicate = count >= 2 && random.nextInt(4) == 0;
			bool unique = true;
			for (int i = 0; i < count; ++i)
			{
				keys[i] = random.nextInt(keySpace);
				if (forceDuplicate && i == count - 1) keys[i] = keys[0];
				values[i] = createFuzzRoleData(step * 16 + i + 4000001, keys[i]);
				for (int j = 0; j < i; ++j) if (keys[j] == keys[i]) { unique = false; break; }
			}
			bool built = dictionary.build(count > 0 ? keys : nullptr, count > 0 ? values : nullptr, count);
			reference.clear();
			if (unique)
			{
				for (int i = 0; i < count; ++i) reference.emplace(keys[i], values[i]);
			}
			if (built != unique) return fail(step, operation, "build result mismatch");
			validateNow = true;
			break;
		}
		case 11:
			dictionary.reserve(random.nextInt(keySpace * 2 + 1));
			validateNow = true;
			break;
		case 12:
			dictionary.shrinkToFit();
			validateNow = true;
			break;
		case 13:
		{
			int valueCapacity = dictionary.capacity();
			int indexCapacity = dictionary.indexCapacity();
			dictionary.clearKeepCapacity();
			reference.clear();
			if (dictionary.capacity() != valueCapacity ||
				dictionary.indexCapacity() != indexCapacity) return fail(step, operation, "clearKeepCapacity changed capacity");
			validateNow = true;
			break;
		}
		case 14:
			dictionary.clearAndRelease();
			reference.clear();
			if (dictionary.capacity() != 0 || dictionary.indexCapacity() != 0 ||
				dictionary.indexMemoryUsageBytes() != 0) return fail(step, operation, "clearAndRelease retained capacity");
			validateNow = true;
			break;
		case 15:
		{
			TDictionary copyConstructed(dictionary);
			const char* copyReason = nullptr;
			if (!validateFuzzDictionary(copyConstructed, reference, copyReason))
				return fail(step, operation, copyReason != nullptr ? copyReason : "copy constructor validation failed");
			TDictionary copyAssigned;
			copyAssigned = dictionary;
			copyAssigned = copyAssigned;
			copyAssigned = std::move(copyAssigned);
			if (!validateFuzzDictionary(copyAssigned, reference, copyReason))
				return fail(step, operation, copyReason != nullptr ? copyReason : "copy assignment validation failed");
			if (!dictionary.empty())
			{
				int independentKey = dictionary.keyAt(random.nextInt(dictionary.size()));
				int originalHP = dictionary[independentKey].mHP;
				copyConstructed[independentKey].mHP += 12345;
				if (dictionary[independentKey].mHP != originalHP) return fail(step, operation, "copy is not independent");
			}
			TDictionary moveConstructed(std::move(copyAssigned));
			if (!validateFuzzDictionary(moveConstructed, reference, copyReason))
				return fail(step, operation, copyReason != nullptr ? copyReason : "move constructor validation failed");
			if (!copyAssigned.empty() || copyAssigned.capacity() != 0 ||
				copyAssigned.indexCapacity() != 0) return fail(step, operation, "move constructor source not empty");
			TDictionary moveAssigned;
			moveAssigned = std::move(moveConstructed);
			if (!validateFuzzDictionary(moveAssigned, reference, copyReason))
				return fail(step, operation, copyReason != nullptr ? copyReason : "move assignment validation failed");
			if (!moveConstructed.empty() || moveConstructed.capacity() != 0 ||
				moveConstructed.indexCapacity() != 0) return fail(step, operation, "move assignment source not empty");
			validateNow = true;
			break;
		}
		case 16:
		{
			auto ecsRef = dictionary.tryGetRef(key);
			auto referenceIt = reference.find(key);
			bool referenceFound = referenceIt != reference.end();
			if (ecsRef.has_value() != referenceFound) return fail(step, operation, "tryGetRef presence mismatch");
			if (referenceFound)
			{
				if (!fuzzRoleEquals(*ecsRef, referenceIt->second)) return fail(step, operation, "tryGetRef value mismatch");
				ecsRef->mHP += 7;
				referenceIt->second.mHP += 7;
			}
			break;
		}
		default:
			return fail(step, operation, "invalid fuzz operation");
		}
		if (validateNow || (step % FUZZ_VALIDATE_INTERVAL) == 0)
		{
			if (!validate(step, operation)) return result;
		}
	}
	if (!validate(operationCount, -1)) return result;
	result.mFinalSize = dictionary.size();
	return result;
}
static void printDictionaryFuzzResult(const char* name, int operationCount, int keySpace, uint64_t seed, const DictionaryFuzzResult& result)
{
	if (result.mPass)
	{
		printf("%-24s:PASS | Ops:%d KeySpace:%d Seed:%llu FullChecks:%d FinalSize:%d\n",
			name, operationCount, keySpace, static_cast<unsigned long long>(seed), result.mValidateCount, result.mFinalSize);
		return;
	}
	printf("%-24s:FAILED | Ops:%d KeySpace:%d Seed:%llu Step:%d Operation:%s(%d) Reason:%s FinalSize:%d\n",
		name, operationCount, keySpace, static_cast<unsigned long long>(seed), result.mFailureStep,
		getFuzzOperationName(result.mFailureOperation), result.mFailureOperation,
		result.mFailureReason != nullptr ? result.mFailureReason : "unknown", result.mFinalSize);
}
static bool runDictionaryFuzzTest()
{
	printf("\n================ Dictionary随机差分/Fuzz ================\n");
	printf("TotalOperations:%d ValidateInterval:%d\n", FUZZ_TOTAL_OPERATION_COUNT, FUZZ_VALIDATE_INTERVAL);
	constexpr uint64_t normalSeed = 0x13579BDF2468ACE1ull;
	constexpr uint64_t collisionSeed = 0x1020304050607080ull;
	constexpr uint64_t constantSeed = 0x8877665544332211ull;
	DictionaryFuzzResult normalResult = runDictionaryFuzzCase<RoleDataECSDictionary<int>>(FUZZ_NORMAL_OPERATION_COUNT, 1024, normalSeed);
	printDictionaryFuzzResult("NormalHash Fuzz Test", FUZZ_NORMAL_OPERATION_COUNT, 1024, normalSeed, normalResult);
	DictionaryFuzzResult collisionResult = runDictionaryFuzzCase<RoleDataECSDictionary<int, CollisionHash>>(FUZZ_COLLISION_OPERATION_COUNT, 256, collisionSeed);
	printDictionaryFuzzResult("CollisionHash Fuzz Test", FUZZ_COLLISION_OPERATION_COUNT, 256, collisionSeed, collisionResult);
	DictionaryFuzzResult constantResult = runDictionaryFuzzCase<RoleDataECSDictionary<int, ConstantHash>>(FUZZ_CONSTANT_OPERATION_COUNT, 64, constantSeed);
	printDictionaryFuzzResult("ConstantHash Fuzz Test", FUZZ_CONSTANT_OPERATION_COUNT, 64, constantSeed, constantResult);
	bool pass = normalResult.mPass && collisionResult.mPass && constantResult.mPass;
	printf("Dictionary Fuzz Test:%s\n", pass ? "PASS" : "FAILED");
	return pass;
}
static bool runFunctionTest()
{
	printf("\n================ Dictionary功能测试 ================\n");
	bool functionPass = true;
	auto reportTest = [&](const char* name, bool pass)
	{
		printf("%s:%s\n", name, pass ? "PASS" : "FAILED");
		functionPass = functionPass && pass;
	};
	EasyECSIndexMap<int, CollisionHash> collisionMap(4);
	bool collisionPass = true;
	for (int i = 0; i < 256; ++i) collisionPass = collisionPass && collisionMap.tryAdd(i, collisionMap.size());
	for (int i = 0; i < 256 && collisionPass; ++i)
	{
		const int* index = collisionMap.findIndex(i);
		collisionPass = index != nullptr && collisionMap.getKeyByIndex(*index) == i;
	}
	for (int i = 0; i < 256; i += 3)
	{
		int removedIndex = -1;
		collisionPass = collisionPass && collisionMap.erase(i, removedIndex);
	}
	for (int i = 0; i < 256 && collisionPass; ++i)
	{
		const int* index = collisionMap.findIndex(i);
		if (i % 3 == 0) collisionPass = index == nullptr;
		else collisionPass = index != nullptr && collisionMap.getKeyByIndex(*index) == i;
	}
	collisionMap.reserve(1024);
	for (int i = 256; i < 512; ++i) collisionPass = collisionPass && collisionMap.tryAdd(i, collisionMap.size());
	for (int i = 0; i < collisionMap.size() && collisionPass; ++i)
	{
		int key = collisionMap.getKeyByIndex(i);
		const int* index = collisionMap.findIndex(key);
		collisionPass = index != nullptr && *index == i;
	}
	reportTest("Flat IndexMap Collision Test", collisionPass);
	int buildCollisionKeys[8]{ 0, 8, 16, 24, 32, 40, 48, 56 };
	EasyECSIndexMap<int, CollisionHash> buildCollisionMap;
	bool indexBuildPass = buildCollisionMap.tryBuild(buildCollisionKeys, 8) && buildCollisionMap.size() == 8;
	for (int i = 0; i < 8 && indexBuildPass; ++i)
	{
		const int* buildIndex = buildCollisionMap.findIndex(buildCollisionKeys[i]);
		indexBuildPass = buildIndex != nullptr && *buildIndex == i && buildCollisionMap.getKeyByIndex(i) == buildCollisionKeys[i];
	}
	int duplicateBuildCollisionKeys[4]{ 1, 9, 1, 17 };
	buildCollisionMap.clearKeepCapacity();
	indexBuildPass = indexBuildPass && !buildCollisionMap.tryBuild(duplicateBuildCollisionKeys, 4) && buildCollisionMap.empty();
	reportTest("Flat IndexMap Build Test", indexBuildPass);
	using CopyMoveIndexMap = EasyECSIndexMap<int, CollisionHash, equal_to<int>, true>;
	CopyMoveIndexMap indexCopySource(64);
	for (int i = 0; i < 16; ++i) indexCopySource.tryAdd(100 + i, indexCopySource.size());
	int copyMoveRemovedIndex = -1;
	indexCopySource.erase(102, copyMoveRemovedIndex); indexCopySource.erase(105, copyMoveRemovedIndex); indexCopySource.erase(108, copyMoveRemovedIndex);
	indexCopySource.resetStats();
	indexCopySource.findIndex(107);
	int indexCopyCapacity = indexCopySource.capacity();
	int indexCopyTombstones = indexCopySource.tombstoneCount();
	CopyMoveIndexMap indexCopyConstructed(indexCopySource);
	bool indexCopyMovePass = indexCopyConstructed.size() == indexCopySource.size() && indexCopyConstructed.capacity() == indexCopyCapacity &&
		indexCopyConstructed.tombstoneCount() == indexCopyTombstones && indexCopyConstructed.getStats().mFindCount == indexCopySource.getStats().mFindCount;
	for (int i = 0; i < indexCopySource.size() && indexCopyMovePass; ++i)
	{
		int sourceKey = indexCopySource.getKeyByIndex(i);
		const int* copiedIndex = indexCopyConstructed.findIndex(sourceKey);
		indexCopyMovePass = indexCopyConstructed.getKeyByIndex(i) == sourceKey && copiedIndex != nullptr && *copiedIndex == i;
	}
	indexCopyMovePass = indexCopyMovePass && indexCopyConstructed.tryAdd(1000, indexCopyConstructed.size()) && !indexCopySource.contains(1000);
	CopyMoveIndexMap indexCopyAssigned(8);
	indexCopyAssigned.tryAdd(1, 0);
	indexCopyAssigned = indexCopySource;
	indexCopyAssigned = indexCopyAssigned;
	indexCopyMovePass = indexCopyMovePass && indexCopyAssigned.size() == indexCopySource.size() && indexCopyAssigned.capacity() == indexCopyCapacity &&
		indexCopyAssigned.tombstoneCount() == indexCopyTombstones;
	CopyMoveIndexMap indexMoveConstructed(std::move(indexCopyAssigned));
	indexCopyMovePass = indexCopyMovePass && indexMoveConstructed.size() == indexCopySource.size() &&
		indexMoveConstructed.capacity() == indexCopyCapacity && indexMoveConstructed.tombstoneCount() == indexCopyTombstones && indexCopyAssigned.empty() &&
		indexCopyAssigned.capacity() == 0 && indexCopyAssigned.tombstoneCount() == 0;
	indexCopyMovePass = indexCopyMovePass && indexCopyAssigned.tryAdd(2000, 0) && indexCopyAssigned.contains(2000);
	CopyMoveIndexMap indexMoveAssigned(8);
	indexMoveAssigned.tryAdd(2, 0);
	indexMoveAssigned = std::move(indexMoveConstructed);
	indexMoveAssigned = std::move(indexMoveAssigned);
	indexCopyMovePass = indexCopyMovePass && indexMoveAssigned.size() == indexCopySource.size() && indexMoveAssigned.capacity() == indexCopyCapacity &&
		indexMoveConstructed.empty() && indexMoveConstructed.capacity() == 0;
	indexCopyMovePass = indexCopyMovePass && indexMoveConstructed.tryAdd(3000, 0) && indexMoveConstructed.contains(3000);
	reportTest("Flat IndexMap Copy Move Test", indexCopyMovePass);
	RoleDataECSDictionary<int> dictionary;
	RoleData a = createRoleData(1), b = createRoleData(2), c = createRoleData(3);
	bool addPass = dictionary.add(1001, a) && dictionary.add(1002, b) && dictionary.add(1003, c) && dictionary.size() == 3;
	reportTest("Add Test", addPass);
	reportTest("Duplicate Add Test", !dictionary.add(1001, c) && dictionary.size() == 3);
	bool containsPass = dictionary.containsKey(1001) && dictionary.containsKey(1002) && dictionary.containsKey(1003) && !dictionary.containsKey(9999);
	reportTest("ContainsKey Test", containsPass);
	int index = -1;
	bool indexPass = dictionary.tryGetIndex(1002, index) && index == dictionary.getIndex(1002) && dictionary.getKeyByIndex(index) == 1002 &&
		!dictionary.tryGetIndex(9999, index);
	reportTest("Index Test", indexPass);
	RoleDataRef role = dictionary[1002];
	role.mHP = 999;
	role.mPositionX = 123.0f;
	reportTest("Ref Test", dictionary[1002].mHP == 999 && dictionary[1002].mPositionX == 123.0f);
	auto optionalRef = dictionary.tryGetRef(1002);
	bool tryGetRefPass = optionalRef.has_value() && optionalRef->mHP == 999 && !dictionary.tryGetRef(9999).has_value();
	if (optionalRef.has_value()) optionalRef->mHP = 998;
	const RoleDataECSDictionary<int>& constDictionary = dictionary;
	auto optionalConstRef = constDictionary.tryGetRef(1002);
	tryGetRefPass = tryGetRefPass && optionalConstRef.has_value() && optionalConstRef->mHP == 998;
	reportTest("TryGetRef Test", tryGetRefPass);
	int* directHP = dictionary.getHPColumn();
	RoleDataAoSBlock* directAoS = dictionary.getAoSColumn();
	bool directColumnPass = directHP != nullptr && directAoS != nullptr && directAoS[dictionary.getIndex(1002)].mID == dictionary[1002].mID;
	reportTest("Dictionary Direct Column Test", directColumnPass);
	RoleDataECSDictionary<int> apiDictionary;
	RoleData tryAddValue = createRoleData(500);
	tryAddValue.mHP = 555;
	bool tryAddPass = apiDictionary.tryAdd(5000, tryAddValue) && !apiDictionary.tryAdd(5000, a) && apiDictionary[5000].mHP == 555;
	reportTest("TryAdd Test", tryAddPass);
	auto existingGetOrAdd = apiDictionary.getOrAdd(5000, a);
	bool getOrAddPass = !existingGetOrAdd.second && existingGetOrAdd.first.mHP == 555;
	RoleData getOrAddValue = createRoleData(600);
	getOrAddValue.mHP = 666;
	auto newGetOrAdd = apiDictionary.getOrAdd(6000, getOrAddValue);
	getOrAddPass = getOrAddPass && newGetOrAdd.second && newGetOrAdd.first.mHP == 666 && apiDictionary[6000].mHP == 666;
	auto defaultGetOrAdd = apiDictionary.getOrAdd(7000);
	getOrAddPass = getOrAddPass && defaultGetOrAdd.second && defaultGetOrAdd.first.mHP == 0;
	reportTest("GetOrAdd Test", getOrAddPass);
	RoleDataECSDictionary<int> batchApiDictionary;
	batchApiDictionary.add(9000, createRoleData(9000));
	int batchApiKeys[5]{ 9001, 9002, 9002, 9003, 9000 };
	RoleData batchApiValues[5]{ createRoleData(9001), createRoleData(9002), createRoleData(9902), createRoleData(9003), createRoleData(9900) };
	int addRangeCount = batchApiDictionary.addRange(batchApiKeys, batchApiValues, 5);
	bool addRangePass = addRangeCount == 3 && batchApiDictionary.size() == 4 && batchApiDictionary[9000].mID == 9000 &&
		batchApiDictionary[9002].mID == 9002 && batchApiDictionary[9003].mID == 9003;
	reportTest("Dictionary AddRange Test", addRangePass);
	int buildApiKeys[4]{ 9100, 9101, 9102, 9103 };
	RoleData buildApiValues[4]{ createRoleData(9100), createRoleData(9101), createRoleData(9102), createRoleData(9103) };
	bool buildPass = batchApiDictionary.build(buildApiKeys, buildApiValues, 4) && batchApiDictionary.size() == 4 && !batchApiDictionary.containsKey(9000);
	for (int i = 0; i < 4 && buildPass; ++i) buildPass = batchApiDictionary.getIndex(buildApiKeys[i]) == i &&
		batchApiDictionary.getValueByIndex(i).mID == buildApiKeys[i];
	int duplicateBuildApiKeys[4]{ 9200, 9201, 9200, 9202 };
	buildPass = buildPass && !batchApiDictionary.build(duplicateBuildApiKeys, buildApiValues, 4) && batchApiDictionary.empty();
	batchApiDictionary.add(9300, createRoleData(9300));
	buildPass = buildPass && !batchApiDictionary.build(nullptr, buildApiValues, 1) && batchApiDictionary.size() == 1 && batchApiDictionary.containsKey(9300);
	buildPass = buildPass && batchApiDictionary.build(nullptr, nullptr, 0) && batchApiDictionary.empty();
	reportTest("Dictionary Build Test", buildPass);
	RoleDataECSDictionary<int> removeAllDictionary(32);
	int removeAllKeys[6]{ 9400, 9401, 9402, 9403, 9404, 9405 };
	RoleData removeAllValues[6]
	{
		createRoleData(9400), createRoleData(9401), createRoleData(9402),
		createRoleData(9403), createRoleData(9404), createRoleData(9405)
	};
	bool removeAllPass = removeAllDictionary.build(removeAllKeys, removeAllValues, 6);
	int removeAllValueCapacity = removeAllDictionary.capacity();
	int removeAllIndexCapacity = removeAllDictionary.indexCapacity();
	int removeAllCount = removeAllDictionary.removeAll([](const int& key, RoleDataConstRef value) { return (key & 1) == 0 && value.mID == key; });
	removeAllPass = removeAllPass && removeAllCount == 3 && removeAllDictionary.size() == 3 && removeAllDictionary.capacity() == removeAllValueCapacity &&
		removeAllDictionary.indexCapacity() == removeAllIndexCapacity;
	for (int i = 0; i < 3 && removeAllPass; ++i)
	{
		int expectedKey = 9401 + i * 2;
		removeAllPass = removeAllDictionary.keyAt(i) == expectedKey && removeAllDictionary.getIndex(expectedKey) == i &&
			removeAllDictionary.valueAt(i).mID == expectedKey;
	}
	removeAllPass = removeAllPass && !removeAllDictionary.containsKey(9400) && !removeAllDictionary.containsKey(9402) && !removeAllDictionary.containsKey(9404);
	removeAllPass = removeAllPass && removeAllDictionary.removeAll([](const int&, RoleDataConstRef) { return false; }) == 0;
	removeAllPass = removeAllPass && removeAllDictionary.removeAll([](const int&, RoleDataConstRef) { return true; }) == 3 && removeAllDictionary.empty() &&
		removeAllDictionary.capacity() == removeAllValueCapacity && removeAllDictionary.indexCapacity() == removeAllIndexCapacity;
	reportTest("Dictionary RemoveAll Test", removeAllPass);
	RoleDataECSDictionary<int, CollisionHash> dictionaryCopySource(64);
	for (int i = 0; i < 8; ++i) dictionaryCopySource.add(9500 + i, createRoleData(9500 + i));
	dictionaryCopySource.remove(9501); dictionaryCopySource.remove(9504);
	int dictionaryCopyValueCapacity = dictionaryCopySource.capacity();
	int dictionaryCopyIndexCapacity = dictionaryCopySource.indexCapacity();
	RoleDataECSDictionary<int, CollisionHash> dictionaryCopyConstructed(dictionaryCopySource);
	bool dictionaryCopyMovePass = dictionaryCopyConstructed.size() == dictionaryCopySource.size() &&
		dictionaryCopyConstructed.capacity() == dictionaryCopyValueCapacity && dictionaryCopyConstructed.indexCapacity() == dictionaryCopyIndexCapacity;
	for (int i = 0; i < dictionaryCopySource.size() && dictionaryCopyMovePass; ++i)
	{
		int sourceKey = dictionaryCopySource.keyAt(i);
		dictionaryCopyMovePass = dictionaryCopyConstructed.keyAt(i) == sourceKey && dictionaryCopyConstructed.getIndex(sourceKey) == i &&
			dictionaryCopyConstructed.valueAt(i).mID == dictionaryCopySource.valueAt(i).mID;
	}
	int independentKey = dictionaryCopySource.keyAt(0);
	int sourceHPBeforeCopyChange = dictionaryCopySource[independentKey].mHP;
	dictionaryCopyConstructed[independentKey].mHP += 1000;
	dictionaryCopyMovePass = dictionaryCopyMovePass && dictionaryCopySource[independentKey].mHP == sourceHPBeforeCopyChange &&
		dictionaryCopyConstructed[independentKey].mHP != sourceHPBeforeCopyChange;
	RoleDataECSDictionary<int, CollisionHash> dictionaryCopyAssigned;
	dictionaryCopyAssigned.add(1, createRoleData(1));
	dictionaryCopyAssigned = dictionaryCopySource;
	dictionaryCopyAssigned = dictionaryCopyAssigned;
	dictionaryCopyMovePass = dictionaryCopyMovePass && dictionaryCopyAssigned.size() == dictionaryCopySource.size() &&
		dictionaryCopyAssigned.capacity() == dictionaryCopyValueCapacity && dictionaryCopyAssigned.indexCapacity() == dictionaryCopyIndexCapacity;
	RoleDataECSDictionary<int, CollisionHash> dictionaryMoveConstructed(std::move(dictionaryCopyAssigned));
	dictionaryCopyMovePass = dictionaryCopyMovePass && dictionaryMoveConstructed.size() == dictionaryCopySource.size() &&
		dictionaryMoveConstructed.capacity() == dictionaryCopyValueCapacity && dictionaryMoveConstructed.indexCapacity() == dictionaryCopyIndexCapacity &&
		dictionaryCopyAssigned.empty() && dictionaryCopyAssigned.capacity() == 0 && dictionaryCopyAssigned.indexCapacity() == 0;
	dictionaryCopyMovePass = dictionaryCopyMovePass && dictionaryCopyAssigned.add(9600, createRoleData(9600)) && dictionaryCopyAssigned[9600].mID == 9600;
	RoleDataECSDictionary<int, CollisionHash> dictionaryMoveAssigned;
	dictionaryMoveAssigned.add(2, createRoleData(2));
	dictionaryMoveAssigned = std::move(dictionaryMoveConstructed);
	dictionaryMoveAssigned = std::move(dictionaryMoveAssigned);
	dictionaryCopyMovePass = dictionaryCopyMovePass && dictionaryMoveAssigned.size() == dictionaryCopySource.size() &&
		dictionaryMoveAssigned.containsKey(independentKey) && dictionaryMoveConstructed.empty() && dictionaryMoveConstructed.capacity() == 0 &&
		dictionaryMoveConstructed.indexCapacity() == 0;
	dictionaryCopyMovePass = dictionaryCopyMovePass && dictionaryMoveConstructed.add(9700, createRoleData(9700)) && dictionaryMoveConstructed.containsKey(9700);
	reportTest("Dictionary Copy Move Test", dictionaryCopyMovePass);
	int forEachCount = 0;
	apiDictionary.forEach([&](const int&, RoleDataRef value)
	{
		value.mHP += 1;
		++forEachCount;
	});
	const RoleDataECSDictionary<int>& constApiDictionary = apiDictionary;
	int constForEachCount = 0;
	long long forEachHPSum = 0;
	constApiDictionary.forEach([&](const int&, RoleDataConstRef value)
	{
		forEachHPSum += value.mHP;
		++constForEachCount;
	});
	bool forEachPass = forEachCount == apiDictionary.size() && constForEachCount == apiDictionary.size() && apiDictionary[5000].mHP == 556 &&
		apiDictionary[6000].mHP == 667 && apiDictionary[7000].mHP == 1 && forEachHPSum == 1224;
	reportTest("Dictionary ForEach Test", forEachPass);
	EasyECSIndexMap<int, std::hash<int>, std::equal_to<int>, true> getOrAddEdgeMap(6);
	for (int i = 0; i < 6; ++i) getOrAddEdgeMap.tryAdd(i, getOrAddEdgeMap.size());
	int edgeCapacityBefore = getOrAddEdgeMap.capacity();
	getOrAddEdgeMap.resetStats();
	bool edgeExistingAdded = true;
	int edgeExistingIndex = getOrAddEdgeMap.getOrAddIndex(3, getOrAddEdgeMap.size(), edgeExistingAdded);
	bool getOrAddEdgePass = !edgeExistingAdded && edgeExistingIndex == 3 && getOrAddEdgeMap.capacity() == edgeCapacityBefore &&
		getOrAddEdgeMap.getStats().mRehashCount == 0;
	getOrAddEdgeMap.resetStats();
	bool edgeNewAdded = false;
	int edgeNewIndex = getOrAddEdgeMap.getOrAddIndex(100, getOrAddEdgeMap.size(), edgeNewAdded);
	getOrAddEdgePass = getOrAddEdgePass && edgeNewAdded && edgeNewIndex == 6 && getOrAddEdgeMap.capacity() > edgeCapacityBefore &&
		getOrAddEdgeMap.getStats().mRehashCount == 1 && getOrAddEdgeMap.getKeyByIndex(edgeNewIndex) == 100;
	reportTest("GetOrAdd Capacity Edge Test", getOrAddEdgePass);
	RoleDataECSList capacityList(128);
	for (int i = 0; i < 12; ++i) capacityList.add(createRoleData(i));
	capacityList.shrinkToFit();
	bool listCapacityPass = capacityList.capacity() == 12 && capacityList.size() == 12 && capacityList[11].mID == 11;
	reportTest("List ShrinkToFit Test", listCapacityPass);
	RoleDataECSDictionary<int> capacityDictionary(128);
	for (int i = 0; i < 12; ++i) capacityDictionary.add(i, createRoleData(i));
	int dictionaryCapacityBeforeShrink = capacityDictionary.capacity();
	int indexCapacityBeforeShrink = capacityDictionary.indexCapacity();
	size_t indexMemoryBeforeShrink = capacityDictionary.indexMemoryUsageBytes();
	capacityDictionary.shrinkToFit();
	bool dictionaryCapacityPass = dictionaryCapacityBeforeShrink == 128 && indexCapacityBeforeShrink == 256 && capacityDictionary.capacity() == 12 &&
		capacityDictionary.indexCapacity() == 32 && capacityDictionary.indexMemoryUsageBytes() < indexMemoryBeforeShrink &&
		capacityDictionary.keyAt(5) == capacityDictionary.getKeyByIndex(5) &&
		capacityDictionary.valueAt(5).mID == capacityDictionary.getValueByIndex(5).mID;
	capacityDictionary.reserve(1000);
	dictionaryCapacityPass = dictionaryCapacityPass && capacityDictionary.capacity() >= 1000 && capacityDictionary.indexCapacity() >= 2048 &&
		capacityDictionary.size() == 12;
	reportTest("Dictionary Capacity API Test", dictionaryCapacityPass);
	EasyECSIndexMap<int> shrinkNoGrowMap(6);
	for (int i = 0; i < 6; ++i) shrinkNoGrowMap.tryAdd(i, shrinkNoGrowMap.size());
	int shrinkNoGrowRemovedIndex = -1;
	shrinkNoGrowMap.erase(0, shrinkNoGrowRemovedIndex);
	int shrinkNoGrowCapacityBefore = shrinkNoGrowMap.capacity();
	shrinkNoGrowMap.shrinkToFit();
	bool shrinkNoGrowPass = shrinkNoGrowMap.capacity() == shrinkNoGrowCapacityBefore && shrinkNoGrowMap.tombstoneCount() == 0 && shrinkNoGrowMap.size() == 5;
	for (int i = 1; i < 6 && shrinkNoGrowPass; ++i) shrinkNoGrowPass = shrinkNoGrowMap.findIndex(i) != nullptr;
	reportTest("ShrinkToFit No Grow Test", shrinkNoGrowPass);
	RoleData value;
	reportTest("TryGetValue Test", dictionary.tryGetValue(1002, value) && value.mHP == 998 && !dictionary.tryGetValue(9999, value));
	RoleData replace = createRoleData(100);
	replace.mHP = 777;
	dictionary.set(1002, replace);
	reportTest("Set Existing Test", dictionary.size() == 3 && dictionary[1002].mHP == 777);
	RoleData newValue = createRoleData(200);
	newValue.mHP = 888;
	dictionary.set(2000, newValue);
	reportTest("Set New Test", dictionary.size() == 4 && dictionary.containsKey(2000) && dictionary[2000].mHP == 888);
	int oldLastIndex = dictionary.size() - 1;
	int oldLastKey = dictionary.getKeyByIndex(oldLastIndex);
	int oldLastID = dictionary.getValueByIndex(oldLastIndex).mID;
	bool removePass = dictionary.remove(1001);
	bool swapBackPass = removePass && dictionary.size() == 3 && !dictionary.containsKey(1001) && dictionary.getKeyByIndex(0) == oldLastKey &&
		dictionary.getValueByIndex(0).mID == oldLastID && dictionary[oldLastKey].mID == oldLastID && dictionary.getIndex(oldLastKey) == 0;
	reportTest("Remove SwapBack Test", swapBackPass);
	reportTest("Remove Missing Test", !dictionary.remove(999999));
	RoleDataECSDictionary<int> rehashDictionary(1);
	for (int i = 0; i < 2000; ++i) rehashDictionary.add(i, createRoleData(i));
	rehashDictionary.reserve(10000);
	int rehashRemoveIndex = 123;
	int rehashRemoveKey = rehashDictionary.getKeyByIndex(rehashRemoveIndex);
	int rehashLastKey = rehashDictionary.getKeyByIndex(rehashDictionary.size() - 1);
	bool rehashRemove = rehashDictionary.removeByIndex(rehashRemoveIndex);
	bool rehashPass = rehashRemove && !rehashDictionary.containsKey(rehashRemoveKey) &&
		rehashDictionary.getKeyByIndex(rehashRemoveIndex) == rehashLastKey && rehashDictionary.getIndex(rehashLastKey) == rehashRemoveIndex &&
		rehashDictionary[rehashLastKey].mID == rehashLastKey;
	reportTest("Rehash RemoveByIndex Test", rehashPass);
	RoleDataECSDictionary<int> batchDictionary(16);
	for (int i = 0; i < 10; ++i) batchDictionary.add(i, createRoleData(i));
	vector<int> batchKeys{ 2, 5, 5, 7, 999 };
	int batchRemoved = batchDictionary.removeBatch(batchKeys);
	bool batchPass = batchRemoved == 3 && batchDictionary.size() == 7 && !batchDictionary.containsKey(2) && !batchDictionary.containsKey(5) &&
		!batchDictionary.containsKey(7);
	for (int i = 0; i < batchDictionary.size() && batchPass; ++i) batchPass = batchDictionary.getValueByIndex(i).mID == batchDictionary.getKeyByIndex(i);
	reportTest("RemoveBatch Test", batchPass);
	RoleDataECSDictionary<int> indexBatchDictionary(16);
	for (int i = 0; i < 10; ++i) indexBatchDictionary.add(i, createRoleData(i));
	vector<int> indexList{ 1, 4, 4, 8, -1, 100 };
	vector<int> indexRemovedKeys{ indexBatchDictionary.getKeyByIndex(1), indexBatchDictionary.getKeyByIndex(4), indexBatchDictionary.getKeyByIndex(8) };
	int indexBatchRemoved = indexBatchDictionary.removeByIndexBatch(indexList);
	bool indexBatchPass = indexBatchRemoved == 3 && indexBatchDictionary.size() == 7;
	for (int key : indexRemovedKeys) indexBatchPass = indexBatchPass && !indexBatchDictionary.containsKey(key);
	for (int i = 0; i < indexBatchDictionary.size() &&
		indexBatchPass; ++i) indexBatchPass = indexBatchDictionary.getValueByIndex(i).mID == indexBatchDictionary.getKeyByIndex(i);
	reportTest("RemoveByIndexBatch Test", indexBatchPass);
	RoleDataECSDictionary<int> rebuildBatchDictionary(16);
	for (int i = 0; i < 10; ++i) rebuildBatchDictionary.add(i, createRoleData(i));
	vector<int> rebuildKeys{ 0, 1, 2, 3, 4, 5, 6 };
	int rebuildRemoved = rebuildBatchDictionary.removeBatch(rebuildKeys);
	bool rebuildPass = rebuildRemoved == 7 && rebuildBatchDictionary.size() == 3 && rebuildBatchDictionary.containsKey(7) &&
		rebuildBatchDictionary.containsKey(8) && rebuildBatchDictionary.containsKey(9);
	for (int i = 0; i < rebuildBatchDictionary.size() &&
		rebuildPass; ++i) rebuildPass = rebuildBatchDictionary.getValueByIndex(i).mID == rebuildBatchDictionary.getKeyByIndex(i);
	reportTest("RemoveBatch Compact Test", rebuildPass);
	EasyECSIndexMap<int, CollisionHash> cleanupMap(16);
	for (int i = 0; i < 100; ++i) cleanupMap.tryAdd(i, cleanupMap.size());
	for (int i = 0; i < 80; ++i)
	{
		int removedIndex = -1;
		cleanupMap.erase(i, removedIndex);
	}
	int tombstoneBeforeAdd = cleanupMap.tombstoneCount();
	bool cleanupAdd = cleanupMap.tryAdd(1000, cleanupMap.size());
	bool cleanupPass = cleanupAdd && tombstoneBeforeAdd > cleanupMap.size() && cleanupMap.tombstoneCount() == 0;
	for (int i = 80; i < 100 && cleanupPass; ++i) cleanupPass = cleanupMap.findIndex(i) != nullptr;
	reportTest("Adaptive Tombstone Cleanup Test", cleanupPass);
	EasyECSIndexMap<int, MixedHash, equal_to<int>, false, 90> loadFactorMap(1);
	bool loadFactorPass = true;
	for (int i = 0; i < 4096; ++i) loadFactorPass = loadFactorPass && loadFactorMap.tryAdd(i, loadFactorMap.size());
	for (int i = 0; i < 4096 && loadFactorPass; ++i)
	{
		const int* foundIndex = loadFactorMap.findIndex(i);
		loadFactorPass = foundIndex != nullptr && loadFactorMap.getKeyByIndex(*foundIndex) == i;
	}
	for (int i = 0; i < 4096; i += 2)
	{
		int removedIndex = -1;
		loadFactorPass = loadFactorPass && loadFactorMap.erase(i, removedIndex);
	}
	for (int i = 0; i < 4096 && loadFactorPass; ++i)
	{
		const int* foundIndex = loadFactorMap.findIndex(i);
		if ((i & 1) == 0) loadFactorPass = foundIndex == nullptr;
		else loadFactorPass = foundIndex != nullptr && loadFactorMap.getKeyByIndex(*foundIndex) == i;
	}
	reportTest("LoadFactor Variant Test", loadFactorPass);
	RoleDataECSList clearList(128);
	for (int i = 0; i < 32; ++i) clearList.add(createRoleData(i));
	int clearListCapacity = clearList.capacity();
	clearList.clearKeepCapacity();
	bool clearKeepCapacityPass = clearList.empty() && clearList.capacity() == clearListCapacity;
	clearList.add(createRoleData(100));
	clearKeepCapacityPass = clearKeepCapacityPass && clearList.size() == 1 && clearList[0].mID == 100;
	reportTest("List ClearKeepCapacity Test", clearKeepCapacityPass);
	clearList.clearAndRelease();
	bool clearReleasePass = clearList.empty() && clearList.capacity() == 0;
	clearList.add(createRoleData(101));
	clearReleasePass = clearReleasePass && clearList.size() == 1 && clearList.capacity() >= 4 && clearList[0].mID == 101;
	reportTest("List ClearAndRelease Test", clearReleasePass);
	RoleDataECSDictionary<int> clearDictionary(128);
	for (int i = 0; i < 32; ++i) clearDictionary.add(i, createRoleData(i));
	int clearValueCapacity = clearDictionary.capacity();
	int clearIndexCapacity = clearDictionary.indexCapacity();
	size_t clearIndexMemory = clearDictionary.indexMemoryUsageBytes();
	clearDictionary.clearKeepCapacity();
	bool dictionaryClearKeepPass = clearDictionary.empty() && clearDictionary.capacity() == clearValueCapacity &&
		clearDictionary.indexCapacity() == clearIndexCapacity && clearDictionary.indexMemoryUsageBytes() == clearIndexMemory;
	clearDictionary.add(1000, createRoleData(1000));
	dictionaryClearKeepPass = dictionaryClearKeepPass && clearDictionary.size() == 1 && clearDictionary[1000].mID == 1000;
	reportTest("Dictionary ClearKeepCapacity Test", dictionaryClearKeepPass);
	clearDictionary.clearAndRelease();
	bool dictionaryClearReleasePass = clearDictionary.empty() && clearDictionary.capacity() == 0 && clearDictionary.indexCapacity() == 0 &&
		clearDictionary.indexMemoryUsageBytes() == 0;
	clearDictionary.add(1001, createRoleData(1001));
	dictionaryClearReleasePass = dictionaryClearReleasePass && clearDictionary.size() == 1 && clearDictionary[1001].mID == 1001 &&
		clearDictionary.capacity() >= 4 && clearDictionary.indexCapacity() >= 8;
	reportTest("Dictionary ClearAndRelease Test", dictionaryClearReleasePass);
	int legacyClearValueCapacity = dictionary.capacity();
	int legacyClearIndexCapacity = dictionary.indexCapacity();
	dictionary.clear();
	reportTest("Clear Test", dictionary.empty() && dictionary.size() == 0 && dictionary.capacity() == legacyClearValueCapacity &&
		dictionary.indexCapacity() == legacyClearIndexCapacity);
	printf("Dictionary Function Test:%s\n", functionPass ? "PASS" : "FAILED");
	return functionPass;
}
int runRoleDataDictionaryBenchmark()
{
	printf("================ RoleData C++ ECSDictionary Benchmark Start ================\n");
	printf("EntityCount:%d\nStructuralCount:%d\nSampleCount:%d\nWarmupCount:%d\n\n", ENTITY_COUNT, STRUCTURAL_COUNT, SAMPLE_COUNT, WARMUP_COUNT);
	unordered_map<int, RoleData> normalDictionary;
	normalDictionary.reserve(ENTITY_COUNT);
	RoleDataECSDictionary<int> ecsDictionary(ENTITY_COUNT);
	for (int i = 0; i < ENTITY_COUNT; ++i)
	{
		RoleData data = createRoleData(i);
		normalDictionary.emplace(i, data);
		ecsDictionary.add(i, data);
	}
	vector<int> lookupKeys(ENTITY_COUNT);
	for (int i = 0; i < ENTITY_COUNT; ++i) lookupKeys[i] = i;
	mt19937 random(123456);
	shuffle(lookupKeys.begin(), lookupKeys.end(), random);
	unordered_map<int, int> stdIndexMap;
	stdIndexMap.reserve(ENTITY_COUNT);
	EasyECSIndexMap<int> flatIndexMap(ENTITY_COUNT);
	for (int i = 0; i < ENTITY_COUNT; ++i)
	{
		stdIndexMap.emplace(i, i);
		flatIndexMap.tryAdd(i, i);
	}
	printf("EasyECSIndexMap Memory:%.3f MB | Capacity:%d | Tombstone:%d\n",
		static_cast<double>(flatIndexMap.memoryUsageBytes()) / (1024.0 * 1024.0), flatIndexMap.capacity(), flatIndexMap.tombstoneCount());
	printf("================ IndexMap随机Find ================\n");
	auto stdIndexFind = runBenchmark(ENTITY_COUNT, [&]()
	{
		long long sum = 0;
		for (int key : lookupKeys)
		{
			auto iter = stdIndexMap.find(key);
			if (iter != stdIndexMap.end()) sum += iter->second;
		}
		gDictionarySink += static_cast<double>(sum);
	});
	auto flatIndexFind = runBenchmark(ENTITY_COUNT, [&]()
	{
		long long sum = 0;
		for (int key : lookupKeys)
		{
			const int* index = flatIndexMap.findIndex(key);
			if (index != nullptr) sum += *index;
		}
		gDictionarySink += static_cast<double>(sum);
	});
	printResult("unordered_map<int,int> Find", stdIndexFind);
	printResult("EasyECSIndexMap<int> Find", flatIndexFind);
	printRatio("unordered_map / FlatIndexMap", stdIndexFind.mMedian, flatIndexFind.mMedian);
	printf("\n================ IndexMap Add性能 ================\n");
	auto stdIndexAdd = runBenchmark(STRUCTURAL_COUNT, [&]()
	{
		unordered_map<int, int> map;
		map.reserve(STRUCTURAL_COUNT);
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) map.emplace(i, i);
		gDictionarySink += map.size();
	});
	auto flatIndexAdd = runBenchmark(STRUCTURAL_COUNT, [&]()
	{
		EasyECSIndexMap<int> map(STRUCTURAL_COUNT);
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) map.tryAdd(i, i);
		gDictionarySink += map.size();
	});
	printResult("unordered_map<int,int> Add", stdIndexAdd);
	printResult("EasyECSIndexMap<int> Add", flatIndexAdd);
	printRatio("unordered_map / FlatIndexMap", stdIndexAdd.mMedian, flatIndexAdd.mMedian);
	vector<int> indexMapRemoveKeys(STRUCTURAL_COUNT);
	for (int i = 0; i < STRUCTURAL_COUNT; ++i) indexMapRemoveKeys[i] = i;
	shuffle(indexMapRemoveKeys.begin(), indexMapRemoveKeys.end(), random);
	unordered_map<int, int> stdRemoveIndexMap;
	stdRemoveIndexMap.reserve(STRUCTURAL_COUNT);
	EasyECSIndexMap<int> flatRemoveIndexMap(STRUCTURAL_COUNT);
	printf("\n================ IndexMap Remove性能 ================\n");
	auto stdIndexRemove = runPreparedBenchmark(STRUCTURAL_COUNT, [&]()
	{
		stdRemoveIndexMap.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) stdRemoveIndexMap.emplace(i, i);
	}, [&]()
	{
		for (int key : indexMapRemoveKeys) stdRemoveIndexMap.erase(key);
		gDictionarySink += stdRemoveIndexMap.size();
	});
	auto flatIndexRemove = runPreparedBenchmark(STRUCTURAL_COUNT, [&]()
	{
		flatRemoveIndexMap.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) flatRemoveIndexMap.tryAdd(i, i);
	}, [&]()
	{
		for (int key : indexMapRemoveKeys)
		{
			int removedIndex = -1;
			flatRemoveIndexMap.erase(key, removedIndex);
		}
		gDictionarySink += flatRemoveIndexMap.size();
	});
	printResult("unordered_map<int,int> Remove", stdIndexRemove);
	printResult("EasyECSIndexMap<int> Remove", flatIndexRemove);
	printRatio("unordered_map / FlatIndexMap", stdIndexRemove.mMedian, flatIndexRemove.mMedian);
	printf("\n================ 按Key随机修改1个字段 ================\n");
	auto normalRandom1 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) normalDictionary[lookupKeys[i]].mHP += 1;
	});
	auto ecsRandom1 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) ecsDictionary[lookupKeys[i]].mHP += 1;
	});
	auto ecsIndexDirect1 = runBenchmark(ENTITY_COUNT, [&]()
	{
		int* EASY_ECS_RESTRICT hp = ecsDictionary.getHPColumn();
		for (int i = 0; i < ENTITY_COUNT; ++i) hp[ecsDictionary.getIndex(lookupKeys[i])] += 1;
	});
	printResult("unordered_map<int, RoleData>", normalRandom1);
	printResult("ECS Dictionary Ref", ecsRandom1);
	printResult("ECS Dictionary IndexDirect", ecsIndexDirect1);
	printRatio("unordered_map / ECS Ref", normalRandom1.mMedian, ecsRandom1.mMedian);
	printRatio("unordered_map / ECS IndexDirect", normalRandom1.mMedian, ecsIndexDirect1.mMedian);

	printf("\n================ 按Key随机访问2个字段 ================\n");
	auto normalRandom2 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleData& value = normalDictionary[lookupKeys[i]];
			value.mPositionX += value.mSpeed;
		}
	});
	auto ecsRandom2 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleDataRef value = ecsDictionary[lookupKeys[i]];
			value.mPositionX += value.mSpeed;
		}
	});
	auto ecsIndexDirect2 = runBenchmark(ENTITY_COUNT, [&]()
	{
		float* EASY_ECS_RESTRICT speed = ecsDictionary.getSpeedColumn();
		float* EASY_ECS_RESTRICT positionX = ecsDictionary.getPositionXColumn();
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			int valueIndex = ecsDictionary.getIndex(lookupKeys[i]);
			positionX[valueIndex] += speed[valueIndex];
		}
	});
	printResult("unordered_map<int, RoleData>", normalRandom2);
	printResult("ECS Dictionary Ref", ecsRandom2);
	printResult("ECS Dictionary IndexDirect", ecsIndexDirect2);
	printRatio("unordered_map / ECS Ref", normalRandom2.mMedian, ecsRandom2.mMedian);
	printRatio("unordered_map / ECS IndexDirect", normalRandom2.mMedian, ecsIndexDirect2.mMedian);

	printf("\n================ 顺序遍历修改1个字段 ================\n");
	auto normalIterate1 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (auto& pair : normalDictionary) pair.second.mHP += 1;
	});
	auto ecsRefIterate1 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (int i = 0; i < ecsDictionary.size(); ++i) ecsDictionary.getValueByIndex(i).mHP += 1;
	});
	auto ecsDirectIterate1 = runBenchmark(ENTITY_COUNT, [&]()
	{
		int* EASY_ECS_RESTRICT hp = ecsDictionary.getHPColumn();
		int count = ecsDictionary.size();
		for (int i = 0; i < count; ++i) hp[i] += 1;
	});
	printResult("unordered_map iteration", normalIterate1);
	printResult("ECS Dictionary Ref", ecsRefIterate1);
	printResult("ECS Dictionary Direct", ecsDirectIterate1);
	printRatio("unordered_map / ECS Ref", normalIterate1.mMedian, ecsRefIterate1.mMedian);
	printRatio("unordered_map / ECS Direct", normalIterate1.mMedian, ecsDirectIterate1.mMedian);

	printf("\n================ 顺序遍历访问4个字段 ================\n");
	auto normalIterate4 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (auto& pair : normalDictionary)
		{
			RoleData& value = pair.second;
			value.mHP += 1;
			value.mSpeed += 0.001f;
			value.mPositionX += value.mSpeed;
			value.mPositionY -= value.mSpeed;
		}
	});
	auto ecsRefIterate4 = runBenchmark(ENTITY_COUNT, [&]()
	{
		for (int i = 0; i < ecsDictionary.size(); ++i)
		{
			RoleDataRef value = ecsDictionary.getValueByIndex(i);
			value.mHP += 1;
			value.mSpeed += 0.001f;
			value.mPositionX += value.mSpeed;
			value.mPositionY -= value.mSpeed;
		}
	});
	auto ecsDirectIterate4 = runBenchmark(ENTITY_COUNT, [&]()
	{
		int* EASY_ECS_RESTRICT hp = ecsDictionary.getHPColumn();
		float* EASY_ECS_RESTRICT speed = ecsDictionary.getSpeedColumn();
		float* EASY_ECS_RESTRICT x = ecsDictionary.getPositionXColumn();
		float* EASY_ECS_RESTRICT y = ecsDictionary.getPositionYColumn();
		int count = ecsDictionary.size();
		for (int i = 0; i < count; ++i)
		{
			hp[i] += 1;
			speed[i] += 0.001f;
			x[i] += speed[i];
			y[i] -= speed[i];
		}
	});
	printResult("unordered_map iteration", normalIterate4);
	printResult("ECS Dictionary Ref", ecsRefIterate4);
	printResult("ECS Dictionary Direct", ecsDirectIterate4);
	printRatio("unordered_map / ECS Ref", normalIterate4.mMedian, ecsRefIterate4.mMedian);
	printRatio("unordered_map / ECS Direct", normalIterate4.mMedian, ecsDirectIterate4.mMedian);

	printf("\n================ 顺序只读4个字段 ================\n");
	auto normalRead = runBenchmark(ENTITY_COUNT, [&]()
	{
		double hp = 0.0, speed = 0.0, x = 0.0, y = 0.0;
		for (const auto& pair : normalDictionary)
		{
			const RoleData& value = pair.second;
			hp += value.mHP;
			speed += value.mSpeed;
			x += value.mPositionX;
			y += value.mPositionY;
		}
		gDictionarySink += hp + speed + x + y;
	});
	auto ecsRefRead = runBenchmark(ENTITY_COUNT, [&]()
	{
		double hp = 0.0, speed = 0.0, x = 0.0, y = 0.0;
		const RoleDataECSDictionary<int>& values = ecsDictionary;
		for (int i = 0; i < values.size(); ++i)
		{
			RoleDataConstRef value = values.getValueByIndex(i);
			hp += value.mHP;
			speed += value.mSpeed;
			x += value.mPositionX;
			y += value.mPositionY;
		}
		gDictionarySink += hp + speed + x + y;
	});
	auto ecsDirectRead = runBenchmark(ENTITY_COUNT, [&]()
	{
		const RoleDataECSDictionary<int>& values = ecsDictionary;
		const int* EASY_ECS_RESTRICT hp = values.getHPColumn();
		const float* EASY_ECS_RESTRICT speed = values.getSpeedColumn();
		const float* EASY_ECS_RESTRICT x = values.getPositionXColumn();
		const float* EASY_ECS_RESTRICT y = values.getPositionYColumn();
		double hpSum = 0.0, speedSum = 0.0, xSum = 0.0, ySum = 0.0;
		int count = values.size();
		for (int i = 0; i < count; ++i)
		{
			hpSum += hp[i];
			speedSum += speed[i];
			xSum += x[i];
			ySum += y[i];
		}
		gDictionarySink += hpSum + speedSum + xSum + ySum;
	});
	printResult("unordered_map iteration", normalRead);
	printResult("ECS Dictionary Ref", ecsRefRead);
	printResult("ECS Dictionary Direct", ecsDirectRead);

	printf("\n================ Add性能 ================\n");
	auto normalAdd = runBenchmark(STRUCTURAL_COUNT, [&]()
	{
		unordered_map<int, RoleData> dictionary;
		dictionary.reserve(STRUCTURAL_COUNT);
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) dictionary.emplace(i, createRoleData(i));
		gDictionarySink += dictionary.size();
	});
	auto ecsAdd = runBenchmark(STRUCTURAL_COUNT, [&]()
	{
		RoleDataECSDictionary<int> dictionary(STRUCTURAL_COUNT);
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) dictionary.add(i, createRoleData(i));
		gDictionarySink += dictionary.size();
	});
	printResult("unordered_map Add", normalAdd);
	printResult("ECS Dictionary Add", ecsAdd);
	printRatio("ECS / unordered_map", ecsAdd.mMedian, normalAdd.mMedian);

	vector<int> batchBuildKeys(STRUCTURAL_COUNT);
	vector<RoleData> batchBuildValues(STRUCTURAL_COUNT);
	for (int i = 0; i < STRUCTURAL_COUNT; ++i)
	{
		batchBuildKeys[i] = i;
		batchBuildValues[i] = createRoleData(i);
	}
	printf("\n================ Dictionary批量构建性能 ================\n");
	auto ecsRepeatedBuild = runBenchmark(STRUCTURAL_COUNT, [&]()
	{
		RoleDataECSDictionary<int> dictionary;
		dictionary.reserve(STRUCTURAL_COUNT);
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) dictionary.add(batchBuildKeys[i], batchBuildValues[i]);
		gDictionarySink += dictionary.size();
	});
	auto ecsAddRangeBuild = runBenchmark(STRUCTURAL_COUNT, [&]()
	{
		RoleDataECSDictionary<int> dictionary;
		gDictionarySink += dictionary.addRange(batchBuildKeys.data(), batchBuildValues.data(), STRUCTURAL_COUNT);
	});
	auto ecsBatchBuild = runBenchmark(STRUCTURAL_COUNT, [&]()
	{
		RoleDataECSDictionary<int> dictionary;
		bool success = dictionary.build(batchBuildKeys.data(), batchBuildValues.data(), STRUCTURAL_COUNT);
		gDictionarySink += success ? dictionary.size() : 0;
	});
	printResult("Repeated add + reserve", ecsRepeatedBuild);
	printResult("Dictionary addRange", ecsAddRangeBuild);
	printResult("Dictionary build", ecsBatchBuild);
	printRatio("Repeated / addRange", ecsRepeatedBuild.mMedian, ecsAddRangeBuild.mMedian);
	printRatio("Repeated / build", ecsRepeatedBuild.mMedian, ecsBatchBuild.mMedian);
	printRatio("addRange / build", ecsAddRangeBuild.mMedian, ecsBatchBuild.mMedian);
	printf("\n================ Remove性能(仅统计Remove) ================\n");
	vector<int> removeKeys(STRUCTURAL_COUNT);
	for (int i = 0; i < STRUCTURAL_COUNT; ++i) removeKeys[i] = i;
	shuffle(removeKeys.begin(), removeKeys.end(), random);
	unordered_map<int, RoleData> normalRemoveDictionary;
	normalRemoveDictionary.reserve(STRUCTURAL_COUNT);
	RoleDataECSDictionary<int> ecsRemoveDictionary(STRUCTURAL_COUNT);
	auto normalRemove = runPreparedBenchmark(STRUCTURAL_COUNT, [&]()
	{
		normalRemoveDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) normalRemoveDictionary.emplace(i, createRoleData(i));
	}, [&]()
	{
		for (int key : removeKeys) normalRemoveDictionary.erase(key);
		gDictionarySink += normalRemoveDictionary.size();
	});
	auto ecsRemove = runPreparedBenchmark(STRUCTURAL_COUNT, [&]()
	{
		ecsRemoveDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) ecsRemoveDictionary.add(i, createRoleData(i));
	}, [&]()
	{
		for (int key : removeKeys) ecsRemoveDictionary.remove(key);
		gDictionarySink += ecsRemoveDictionary.size();
	});
	printResult("unordered_map Remove", normalRemove);
	printResult("ECS Dictionary Remove", ecsRemove);
	printRatio("ECS / unordered_map", ecsRemove.mMedian, normalRemove.mMedian);
	printf("\n================ 条件删除性能(删除50%%) ================\n");
	unordered_map<int, RoleData> normalPredicateRemoveDictionary;
	normalPredicateRemoveDictionary.reserve(STRUCTURAL_COUNT);
	RoleDataECSDictionary<int> ecsPredicateRemoveDictionary(STRUCTURAL_COUNT);
	auto normalPredicateRemove = runPreparedBenchmark(STRUCTURAL_COUNT, [&]()
	{
		normalPredicateRemoveDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) normalPredicateRemoveDictionary.emplace(i, batchBuildValues[i]);
	}, [&]()
	{
		int removedCount = 0;
		for (auto it = normalPredicateRemoveDictionary.begin(); it != normalPredicateRemoveDictionary.end();)
		{
			if ((it->first & 1) != 0)
			{
				++it;
				continue;
			}
			it = normalPredicateRemoveDictionary.erase(it);
			++removedCount;
		}
		gDictionarySink += removedCount + normalPredicateRemoveDictionary.size();
	});
	auto ecsPredicateRemove = runPreparedBenchmark(STRUCTURAL_COUNT, [&]()
	{
		ecsPredicateRemoveDictionary.clearKeepCapacity();
		ecsPredicateRemoveDictionary.build(batchBuildKeys.data(), batchBuildValues.data(), STRUCTURAL_COUNT);
	}, [&]()
	{
		int removedCount = ecsPredicateRemoveDictionary.removeAll([](const int& key, RoleDataConstRef value) { return (key & 1) == 0 && value.mID == key; });
		gDictionarySink += removedCount + ecsPredicateRemoveDictionary.size();
	});
	printResult("unordered_map predicate erase", normalPredicateRemove);
	printResult("ECS Dictionary removeAll", ecsPredicateRemove);
	printRatio("unordered_map / ECS", normalPredicateRemove.mMedian, ecsPredicateRemove.mMedian);
	const int halfRemoveCount = STRUCTURAL_COUNT / 2;
	vector<int> halfRemoveKeys(removeKeys.begin(), removeKeys.begin() + halfRemoveCount);
	vector<int> halfRemoveIndices = halfRemoveKeys;
	RoleDataECSDictionary<int> ecsRepeatedHalfDictionary(STRUCTURAL_COUNT);
	RoleDataECSDictionary<int> ecsBatchHalfDictionary(STRUCTURAL_COUNT);
	RoleDataECSDictionary<int> ecsIndexBatchHalfDictionary(STRUCTURAL_COUNT);
	printf("\n================ Batch Remove性能(删除50%%) ================\n");
	auto ecsRepeatedHalf = runPreparedBenchmark(halfRemoveCount, [&]()
	{
		ecsRepeatedHalfDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) ecsRepeatedHalfDictionary.add(i, createRoleData(i));
	}, [&]()
	{
		for (int key : halfRemoveKeys) ecsRepeatedHalfDictionary.remove(key);
		gDictionarySink += ecsRepeatedHalfDictionary.size();
	});
	auto ecsBatchHalf = runPreparedBenchmark(halfRemoveCount, [&]()
	{
		ecsBatchHalfDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) ecsBatchHalfDictionary.add(i, createRoleData(i));
	}, [&]()
	{
		gDictionarySink += ecsBatchHalfDictionary.removeBatch(halfRemoveKeys);
	});
	auto ecsIndexBatchHalf = runPreparedBenchmark(halfRemoveCount, [&]()
	{
		ecsIndexBatchHalfDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) ecsIndexBatchHalfDictionary.add(i, createRoleData(i));
	}, [&]()
	{
		gDictionarySink += ecsIndexBatchHalfDictionary.removeByIndexBatch(halfRemoveIndices);
	});
	printResult("ECS Repeated Remove 50%", ecsRepeatedHalf);
	printResult("ECS removeBatch 50%", ecsBatchHalf);
	printResult("ECS removeByIndexBatch 50%", ecsIndexBatchHalf);
	printRatio("Repeated / removeBatch", ecsRepeatedHalf.mMedian, ecsBatchHalf.mMedian);
	printRatio("Repeated / IndexBatch", ecsRepeatedHalf.mMedian, ecsIndexBatchHalf.mMedian);
	const int largeRemoveCount = STRUCTURAL_COUNT * 8 / 10;
	vector<int> largeRemoveKeys(removeKeys.begin(), removeKeys.begin() + largeRemoveCount);
	vector<int> largeRemoveIndices = largeRemoveKeys;
	RoleDataECSDictionary<int> ecsRepeatedLargeDictionary(STRUCTURAL_COUNT);
	RoleDataECSDictionary<int> ecsBatchLargeDictionary(STRUCTURAL_COUNT);
	RoleDataECSDictionary<int> ecsIndexBatchLargeDictionary(STRUCTURAL_COUNT);
	printf("\n================ Batch Remove性能(删除80%%) ================\n");
	auto ecsRepeatedLarge = runPreparedBenchmark(largeRemoveCount, [&]()
	{
		ecsRepeatedLargeDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) ecsRepeatedLargeDictionary.add(i, createRoleData(i));
	}, [&]()
	{
		for (int key : largeRemoveKeys) ecsRepeatedLargeDictionary.remove(key);
		gDictionarySink += ecsRepeatedLargeDictionary.size();
	});
	auto ecsBatchLarge = runPreparedBenchmark(largeRemoveCount, [&]()
	{
		ecsBatchLargeDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) ecsBatchLargeDictionary.add(i, createRoleData(i));
	}, [&]()
	{
		gDictionarySink += ecsBatchLargeDictionary.removeBatch(largeRemoveKeys);
	});
	auto ecsIndexBatchLarge = runPreparedBenchmark(largeRemoveCount, [&]()
	{
		ecsIndexBatchLargeDictionary.clear();
		for (int i = 0; i < STRUCTURAL_COUNT; ++i) ecsIndexBatchLargeDictionary.add(i, createRoleData(i));
	}, [&]()
	{
		gDictionarySink += ecsIndexBatchLargeDictionary.removeByIndexBatch(largeRemoveIndices);
	});
	printResult("ECS Repeated Remove 80%", ecsRepeatedLarge);
	printResult("ECS removeBatch 80%", ecsBatchLarge);
	printResult("ECS removeByIndexBatch 80%", ecsIndexBatchLarge);
	printRatio("Repeated / removeBatch", ecsRepeatedLarge.mMedian, ecsBatchLarge.mMedian);
	printRatio("Repeated / IndexBatch", ecsRepeatedLarge.mMedian, ecsIndexBatchLarge.mMedian);
	runChurnBenchmark();
	bool functionPass = runFunctionTest();
	printf("\nDictionarySink:%f\n", static_cast<double>(gDictionarySink));
	return functionPass ? 0 : 1;
}
int runRoleDataDictionaryFuzzTest()
{
	return runDictionaryFuzzTest() ? 0 : 1;
}

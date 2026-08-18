#include "RoleDataListBenchmark.h"
#include "Data/EasyECS.generated.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <utility>
#include <vector>
#undef max
#undef min

using namespace std;

static constexpr int ENTITY_COUNT = 500000;
static constexpr int SAMPLE_COUNT = 15;
static constexpr int WARMUP_COUNT = 3;
static constexpr size_t MEMORY_ALIGNMENT = 64;
static volatile double gReadSink = 0.0;

struct BenchmarkResult
{
	double mMedian;
	double mMin;
	double mMax;
	double mNsPerEntity;
};

struct RawRoleDataSoA
{
	void* mRawMemory = nullptr;
	int* mHP = nullptr;
	float* mSpeed = nullptr;
	float* mPositionX = nullptr;
	float* mPositionY = nullptr;
	explicit RawRoleDataSoA(int count)
	{
		size_t offset = 0;
		auto alignUp = [](size_t value) { return (value + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT - 1); };
		offset = alignUp(offset);
		size_t hpOffset = offset;
		offset += sizeof(int) * count;
		offset = alignUp(offset);
		size_t speedOffset = offset;
		offset += sizeof(float) * count;
		offset = alignUp(offset);
		size_t xOffset = offset;
		offset += sizeof(float) * count;
		offset = alignUp(offset);
		size_t yOffset = offset;
		offset += sizeof(float) * count;
		mRawMemory = std::malloc(offset + MEMORY_ALIGNMENT - 1);
		if (mRawMemory == nullptr) throw std::bad_alloc();
		uintptr_t value = reinterpret_cast<uintptr_t>(mRawMemory);
		value = (value + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT - 1);
		uint8_t* memory = reinterpret_cast<uint8_t*>(value);
		mHP = reinterpret_cast<int*>(memory + hpOffset);
		mSpeed = reinterpret_cast<float*>(memory + speedOffset);
		mPositionX = reinterpret_cast<float*>(memory + xOffset);
		mPositionY = reinterpret_cast<float*>(memory + yOffset);
	}
	~RawRoleDataSoA() { std::free(mRawMemory); }
	RawRoleDataSoA(const RawRoleDataSoA&) = delete;
	RawRoleDataSoA& operator=(const RawRoleDataSoA&) = delete;
};

template<typename Func>
BenchmarkResult runBenchmark(Func func)
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
	return { median, samples.front(), samples.back(), median * 1000000.0 / ENTITY_COUNT };
}
template<typename Setup, typename Func>
BenchmarkResult runPreparedBenchmark(Setup setup, Func func)
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
	return { median, samples.front(), samples.back(), median * 1000000.0 / ENTITY_COUNT };
}
static void printResult(const char* name, const BenchmarkResult& result)
{
	printf("%-20s Median:%9.3f ms | Min:%8.3f | Max:%8.3f | %9.3f ns/entity\n", name, result.mMedian, result.mMin, result.mMax, result.mNsPerEntity);
}
static void printRatio(const char* name, double source, double target)
{
	printf("%-24s: %.2fx\n", name, source / target);
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
int runRoleDataListBenchmark()
{
	printf("================ RoleData C++ EasyECS Benchmark Start ================\n");
	printf("EntityCount:%d\nSampleCount:%d\nWarmupCount:%d\n\n", ENTITY_COUNT, SAMPLE_COUNT, WARMUP_COUNT);
	vector<RoleData> vectorData(ENTITY_COUNT);
	RoleData* arrayData = new RoleData[ENTITY_COUNT];
	RoleDataECSList ecsData(ENTITY_COUNT);
	RawRoleDataSoA rawData(ENTITY_COUNT);
	for (int i = 0; i < ENTITY_COUNT; ++i)
	{
		RoleData data = createRoleData(i);
		vectorData[i] = data;
		arrayData[i] = data;
		ecsData.add(data);
		rawData.mHP[i] = data.mHP;
		rawData.mSpeed[i] = data.mSpeed;
		rawData.mPositionX[i] = data.mPositionX;
		rawData.mPositionY[i] = data.mPositionY;
	}

	printf("================ List批量构建性能(Row-wise) ================\n");
	RoleDataECSList repeatedBuildList(ENTITY_COUNT);
	RoleDataECSList rangeBuildList(ENTITY_COUNT);
	auto repeatedBuild = runBenchmark([&]()
	{
		repeatedBuildList.clearKeepCapacity();
		for (int i = 0; i < ENTITY_COUNT; ++i) repeatedBuildList.add(vectorData[i]);
		gReadSink += repeatedBuildList.size() + repeatedBuildList.getHPColumn()[ENTITY_COUNT - 1];
	});
	auto rangeBuild = runBenchmark([&]()
	{
		rangeBuildList.clearKeepCapacity();
		rangeBuildList.addRange(vectorData.data(), ENTITY_COUNT);
		gReadSink += rangeBuildList.size() + rangeBuildList.getHPColumn()[ENTITY_COUNT - 1];
	});
	printResult("Repeated add", repeatedBuild);
	printResult("addRange Row-wise", rangeBuild);
	printRatio("Repeated / Row-wise", repeatedBuild.mMedian, rangeBuild.mMedian);

	printf("\n================ List条件删除性能(删除50%%) ================\n");
	vector<RoleData> vectorRemoveData;
	vectorRemoveData.reserve(ENTITY_COUNT);
	RoleDataECSList ecsRemoveAllData(ENTITY_COUNT);
	auto vectorRemoveAll = runPreparedBenchmark([&]()
	{
		vectorRemoveData = vectorData;
	}, [&]()
	{
		auto newEnd = remove_if(vectorRemoveData.begin(), vectorRemoveData.end(), [](const RoleData& value) { return (value.mID & 1) == 0; });
		int removedCount = static_cast<int>(vectorRemoveData.end() - newEnd);
		vectorRemoveData.erase(newEnd, vectorRemoveData.end());
		gReadSink += removedCount + vectorRemoveData.size();
	});
	auto ecsRemoveAll = runPreparedBenchmark([&]()
	{
		ecsRemoveAllData.clearKeepCapacity();
		ecsRemoveAllData.addRange(vectorData.data(), ENTITY_COUNT);
	}, [&]()
	{
		int removedCount = ecsRemoveAllData.removeAll([](RoleDataConstRef value) { return (value.mID & 1) == 0; });
		gReadSink += removedCount + ecsRemoveAllData.size();
	});
	printResult("vector erase/remove_if", vectorRemoveAll);
	printResult("ECS removeAll", ecsRemoveAll);
	printRatio("Vector / ECS", vectorRemoveAll.mMedian, ecsRemoveAll.mMedian);

	printf("\n================ 修改1个字段 ================\n");
	auto vector1 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) vectorData[i].mHP += 1;
	});
	auto array1 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) arrayData[i].mHP += 1;
	});
	auto index1 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) ecsData[i].mHP += 1;
	});
	auto ref1 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleDataRef role = ecsData[i];
			role.mHP += 1;
		}
	});
	auto direct1 = runBenchmark([&]()
	{
		int* EASY_ECS_RESTRICT hp = ecsData.getHPColumn();
		for (int i = 0; i < ENTITY_COUNT; ++i) hp[i] += 1;
	});
	auto raw1 = runBenchmark([&]()
	{
		int* EASY_ECS_RESTRICT hp = rawData.mHP;
		for (int i = 0; i < ENTITY_COUNT; ++i) hp[i] += 1;
	});
	printResult("vector<RoleData>", vector1);
	printResult("RoleData[]", array1);
	printResult("ECS list[i]", index1);
	printResult("ECS Ref", ref1);
	printResult("ECS Direct", direct1);
	printResult("Raw SoA", raw1);
	printRatio("Index / Direct", index1.mMedian, direct1.mMedian);
	printRatio("Ref / Direct", ref1.mMedian, direct1.mMedian);
	printRatio("Vector / Direct", vector1.mMedian, direct1.mMedian);

	printf("\n================ 访问2个字段 ================\n");
	auto vector2 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) vectorData[i].mPositionX += vectorData[i].mSpeed;
	});
	auto array2 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) arrayData[i].mPositionX += arrayData[i].mSpeed;
	});
	auto index2 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i) ecsData[i].mPositionX += ecsData[i].mSpeed;
	});
	auto ref2 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleDataRef role = ecsData[i];
			role.mPositionX += role.mSpeed;
		}
	});
	auto direct2 = runBenchmark([&]()
	{
		float* EASY_ECS_RESTRICT speed = ecsData.getSpeedColumn();
		float* EASY_ECS_RESTRICT x = ecsData.getPositionXColumn();
		for (int i = 0; i < ENTITY_COUNT; ++i) x[i] += speed[i];
	});
	auto raw2 = runBenchmark([&]()
	{
		float* EASY_ECS_RESTRICT speed = rawData.mSpeed;
		float* EASY_ECS_RESTRICT x = rawData.mPositionX;
		for (int i = 0; i < ENTITY_COUNT; ++i) x[i] += speed[i];
	});
	printResult("vector<RoleData>", vector2);
	printResult("RoleData[]", array2);
	printResult("ECS list[i]", index2);
	printResult("ECS Ref", ref2);
	printResult("ECS Direct", direct2);
	printResult("Raw SoA", raw2);
	printRatio("Index / Direct", index2.mMedian, direct2.mMedian);
	printRatio("Ref / Direct", ref2.mMedian, direct2.mMedian);
	printRatio("Vector / Direct", vector2.mMedian, direct2.mMedian);

	printf("\n================ 访问4个字段 ================\n");
	auto vector4 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleData& role = vectorData[i];
			role.mHP += 1;
			role.mSpeed += 0.001f;
			role.mPositionX += role.mSpeed;
			role.mPositionY -= role.mSpeed;
		}
	});
	auto array4 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleData& role = arrayData[i];
			role.mHP += 1;
			role.mSpeed += 0.001f;
			role.mPositionX += role.mSpeed;
			role.mPositionY -= role.mSpeed;
		}
	});
	auto index4 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			ecsData[i].mHP += 1;
			ecsData[i].mSpeed += 0.001f;
			ecsData[i].mPositionX += ecsData[i].mSpeed;
			ecsData[i].mPositionY -= ecsData[i].mSpeed;
		}
	});
	auto ref4 = runBenchmark([&]()
	{
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleDataRef role = ecsData[i];
			role.mHP += 1;
			role.mSpeed += 0.001f;
			role.mPositionX += role.mSpeed;
			role.mPositionY -= role.mSpeed;
		}
	});
	auto direct4 = runBenchmark([&]()
	{
		int* EASY_ECS_RESTRICT hp = ecsData.getHPColumn();
		float* EASY_ECS_RESTRICT speed = ecsData.getSpeedColumn();
		float* EASY_ECS_RESTRICT x = ecsData.getPositionXColumn();
		float* EASY_ECS_RESTRICT y = ecsData.getPositionYColumn();
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			hp[i] += 1;
			speed[i] += 0.001f;
			x[i] += speed[i];
			y[i] -= speed[i];
		}
	});
	auto raw4 = runBenchmark([&]()
	{
		int* EASY_ECS_RESTRICT hp = rawData.mHP;
		float* EASY_ECS_RESTRICT speed = rawData.mSpeed;
		float* EASY_ECS_RESTRICT x = rawData.mPositionX;
		float* EASY_ECS_RESTRICT y = rawData.mPositionY;
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			hp[i] += 1;
			speed[i] += 0.001f;
			x[i] += speed[i];
			y[i] -= speed[i];
		}
	});
	printResult("vector<RoleData>", vector4);
	printResult("RoleData[]", array4);
	printResult("ECS list[i]", index4);
	printResult("ECS Ref", ref4);
	printResult("ECS Direct", direct4);
	printResult("Raw SoA", raw4);
	printRatio("Index / Direct", index4.mMedian, direct4.mMedian);
	printRatio("Ref / Direct", ref4.mMedian, direct4.mMedian);
	printRatio("Vector / Direct", vector4.mMedian, direct4.mMedian);

	printf("\n================ 只读4个字段 ================\n");
	auto vectorRead = runBenchmark([&]()
	{
		double hp = 0.0, speed = 0.0, x = 0.0, y = 0.0;
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			const RoleData& role = vectorData[i];
			hp += role.mHP;
			speed += role.mSpeed;
			x += role.mPositionX;
			y += role.mPositionY;
		}
		gReadSink += hp + speed + x + y;
	});
	auto arrayRead = runBenchmark([&]()
	{
		double hp = 0.0, speed = 0.0, x = 0.0, y = 0.0;
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			const RoleData& role = arrayData[i];
			hp += role.mHP;
			speed += role.mSpeed;
			x += role.mPositionX;
			y += role.mPositionY;
		}
		gReadSink += hp + speed + x + y;
	});
	auto refRead = runBenchmark([&]()
	{
		double hp = 0.0, speed = 0.0, x = 0.0, y = 0.0;
		const RoleDataECSList& values = ecsData;
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			RoleDataConstRef role = values[i];
			hp += role.mHP;
			speed += role.mSpeed;
			x += role.mPositionX;
			y += role.mPositionY;
		}
		gReadSink += hp + speed + x + y;
	});
	auto directRead = runBenchmark([&]()
	{
		double hpSum = 0.0, speedSum = 0.0, xSum = 0.0, ySum = 0.0;
		const RoleDataECSList& values = ecsData;
		const int* EASY_ECS_RESTRICT hp = values.getHPColumn();
		const float* EASY_ECS_RESTRICT speed = values.getSpeedColumn();
		const float* EASY_ECS_RESTRICT x = values.getPositionXColumn();
		const float* EASY_ECS_RESTRICT y = values.getPositionYColumn();
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			hpSum += hp[i];
			speedSum += speed[i];
			xSum += x[i];
			ySum += y[i];
		}
		gReadSink += hpSum + speedSum + xSum + ySum;
	});
	auto rawRead = runBenchmark([&]()
	{
		double hpSum = 0.0, speedSum = 0.0, xSum = 0.0, ySum = 0.0;
		const int* EASY_ECS_RESTRICT hp = rawData.mHP;
		const float* EASY_ECS_RESTRICT speed = rawData.mSpeed;
		const float* EASY_ECS_RESTRICT x = rawData.mPositionX;
		const float* EASY_ECS_RESTRICT y = rawData.mPositionY;
		for (int i = 0; i < ENTITY_COUNT; ++i)
		{
			hpSum += hp[i];
			speedSum += speed[i];
			xSum += x[i];
			ySum += y[i];
		}
		gReadSink += hpSum + speedSum + xSum + ySum;
	});
	printResult("vector<RoleData>", vectorRead);
	printResult("RoleData[]", arrayRead);
	printResult("ECS Ref", refRead);
	printResult("ECS Direct", directRead);
	printResult("Raw SoA", rawRead);

	printf("\n================ List功能测试 ================\n");
	bool functionPass = true;
	auto reportTest = [&](const char* name, bool pass)
	{
		printf("%s:%s\n", name, pass ? "PASS" : "FAILED");
		functionPass = functionPass && pass;
	};
	RoleDataECSList test(2);
	RoleData a = createRoleData(1), b = createRoleData(2), c = createRoleData(3), d = createRoleData(4);
	test.add(a);
	test.add(c);
	test.insert(1, b);
	reportTest("Insert Test", test.size() == 3 && test[0].mID == 1 && test[1].mID == 2 && test[2].mID == 3);
	test.removeAt(1);
	reportTest("RemoveAt Test", test.size() == 2 && test[0].mID == 1 && test[1].mID == 3);
	test.add(d);
	int oldLastID = test[2].mID;
	test.removeAtSwapBack(0);
	reportTest("RemoveAtSwapBack Test", test.size() == 2 && test[0].mID == oldLastID);
	int oldSize = test.size();
	test.popBack();
	reportTest("PopBack Test", test.size() == oldSize - 1);
	RoleDataECSList defaultTest;
	RoleDataRef defaultValue = defaultTest.addDefault();
	bool addDefaultPass = defaultTest.size() == 1 && defaultValue.mHP == 0 && defaultValue.mSpeed == 0.0f &&
		defaultValue.mPositionX == 0.0f && defaultValue.mPositionY == 0.0f && defaultValue.mID == 0 &&
		defaultValue.mModelID == 0 && defaultValue.mCamp == 0;
	defaultValue.mHP = 777;
	defaultValue.mID = 888;
	addDefaultPass = addDefaultPass && defaultTest[0].mHP == 777 && defaultTest[0].mID == 888;
	reportTest("AddDefault Test", addDefaultPass);
	RoleData rangeValues[4] = { createRoleData(10), createRoleData(11), createRoleData(12), createRoleData(13) };
	RoleDataECSList rangeTest(1);
	rangeTest.add(createRoleData(9));
	rangeTest.addRange(rangeValues, 4);
	rangeTest.addRange(nullptr, 4);
	rangeTest.addRange(rangeValues, 0);
	bool addRangePass = rangeTest.size() == 5 && rangeTest[0].mID == 9 && rangeTest[1].mID == 10 &&
		rangeTest[2].mHP == rangeValues[1].mHP && rangeTest[3].mSpeed == rangeValues[2].mSpeed &&
		rangeTest[4].mCamp == rangeValues[3].mCamp;
	reportTest("AddRange Test", addRangePass);
	RoleData removeAllValues[6] = { createRoleData(20), createRoleData(21), createRoleData(22), createRoleData(23), createRoleData(24), createRoleData(25) };
	RoleDataECSList removeAllTest(16);
	removeAllTest.addRange(removeAllValues, 6);
	int removeAllCapacity = removeAllTest.capacity();
	int removeAllCount = removeAllTest.removeAll([](RoleDataConstRef value) { return (value.mID & 1) == 0; });
	bool removeAllPass = removeAllCount == 3 && removeAllTest.size() == 3 && removeAllTest.capacity() == removeAllCapacity &&
		removeAllTest[0].mID == 21 && removeAllTest[1].mID == 23 && removeAllTest[2].mID == 25;
	removeAllPass = removeAllPass && removeAllTest.removeAll([](RoleDataConstRef) { return false; }) == 0;
	removeAllPass = removeAllPass && removeAllTest.removeAll([](RoleDataConstRef) { return true; }) == 3 &&
		removeAllTest.empty() && removeAllTest.capacity() == removeAllCapacity;
	reportTest("RemoveAll Test", removeAllPass);
	RoleDataECSList copySource(16);
	copySource.add(createRoleData(30));
	copySource.add(createRoleData(31));
	copySource.add(createRoleData(32));
	RoleDataECSList copyConstructed(copySource);
	bool copyMovePass = copyConstructed.size() == 3 && copyConstructed.capacity() == copySource.capacity() && copyConstructed[1].mID == 31;
	copyConstructed[1].mHP = 9999;
	copyMovePass = copyMovePass && copySource[1].mHP != 9999;
	RoleDataECSList copyAssigned(2);
	copyAssigned.add(createRoleData(100));
	copyAssigned = copySource;
	copyAssigned = copyAssigned;
	copyMovePass = copyMovePass && copyAssigned.size() == 3 && copyAssigned.capacity() == copySource.capacity() && copyAssigned[2].mID == 32;
	int moveCapacity = copyAssigned.capacity();
	RoleDataECSList moveConstructed(std::move(copyAssigned));
	copyMovePass = copyMovePass && moveConstructed.size() == 3 && moveConstructed.capacity() == moveCapacity &&
		copyAssigned.empty() && copyAssigned.capacity() == 0;
	copyAssigned.add(createRoleData(200));
	copyMovePass = copyMovePass && copyAssigned.size() == 1 && copyAssigned[0].mID == 200;
	RoleDataECSList moveAssigned(8);
	moveAssigned.add(createRoleData(300));
	moveAssigned = std::move(moveConstructed);
	moveAssigned = std::move(moveAssigned);
	copyMovePass = copyMovePass && moveAssigned.size() == 3 && moveAssigned[0].mID == 30 && moveConstructed.empty() && moveConstructed.capacity() == 0;
	moveConstructed.add(createRoleData(400));
	copyMovePass = copyMovePass && moveConstructed.size() == 1 && moveConstructed[0].mID == 400;
	reportTest("Copy Move Test", copyMovePass);
	printf("\n================ Ref字段访问测试 ================\n");
	RoleDataRef role = ecsData[0];
	role.mHP = 123456;
	role.mSpeed = 8.0f;
	role.mPositionX = 100.0f;
	role.mPositionY = 200.0f;
	role.mID = 98765;
	RoleData check = ecsData.get(0);
	bool refPass = check.mHP == 123456 && check.mSpeed == 8.0f && check.mPositionX == 100.0f && check.mPositionY == 200.0f && check.mID == 98765;
	reportTest("Ref Field Test", refPass);
	printf("List Function Test:%s\n", functionPass ? "PASS" : "FAILED");
	printf("ReadSink:%f\n", static_cast<double>(gReadSink));
	delete[] arrayData;
	return functionPass ? 0 : 1;
}

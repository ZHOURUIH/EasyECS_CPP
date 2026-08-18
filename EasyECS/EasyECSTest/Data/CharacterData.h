#pragma once
#include "EasyECS.h"
#include <array>
#include <cstdint>

namespace EasyECSDemo
{
ECS()
struct MonsterData
{
	int mHP = 0;
	float mMoveSpeed = 0.0f;
	NOT_ECS() int mID = 0;
};
ECS()
struct NPCData
{
	int mHP = 0;
	float mTalkDistance = 0.0f;
	NOT_ECS() int mID = 0;
};
enum class TypeDataState : uint8_t
{
	Idle,
	Running,
};
struct TypeDataVector2
{
	float mX = 0.0f;
	float mY = 0.0f;
};
using TypeDataAliasUInt = uint32_t;
typedef int64_t TypeDataAliasInt64;
namespace TypeSupport
{
enum class MoveState : uint8_t
{
	Idle,
	Moving,
};
struct Vector3
{
	float mX = 0.0f;
	float mY = 0.0f;
	float mZ = 0.0f;
};
using Position = Vector3;
}
ECS()
struct TypeData
{
	unsigned mUnsigned = 1;
	long long mLongLong = 2;
	uint32_t mUInt32 = 3;
	int64_t mInt64 = -4;
	TypeDataState mState = TypeDataState::Idle;
	TypeDataVector2 mPosition{};
	TypeDataAliasUInt mAliasUInt = 5;
	TypeDataAliasInt64 mAliasInt64 = -6;
	TypeSupport::MoveState mMoveState = TypeSupport::MoveState::Idle;
	TypeSupport::Position mPosition3{};
	std::array<uint16_t, 4> mFixedValues{};
	[[maybe_unused]] uint32_t mAttributeValue = 9;
	const uint32_t mImmutable = 777;
	unsigned long long const mTailConst = 888;
	NOT_ECS() uint64_t mID = 0;
};
}

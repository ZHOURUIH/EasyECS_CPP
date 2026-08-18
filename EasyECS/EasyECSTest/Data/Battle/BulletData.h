#pragma once
#include "EasyECS.h"

namespace EasyECSDemo::Battle
{
ECS()
struct BulletData
{
	float mPositionX = 0.0f;
	float mPositionY = 0.0f;
	NOT_ECS() int mOwnerID = 0;
};
}

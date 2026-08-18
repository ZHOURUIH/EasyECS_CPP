#pragma once
#include "EasyECS.h"

namespace EasyECSDemo
{
ECS()
struct ItemData
{
	int mCount = 0;
	float mWeight = 0.0f;
	NOT_ECS() int mID = 0;
};
}

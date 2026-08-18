#pragma once
#include "EasyECS.h"

ECS()
struct RoleData
{
	int mHP = 0;
	float mSpeed = 0.0f;
	float mPositionX = 0.0f;
	float mPositionY = 0.0f;
	NOT_ECS() int mID = 0;
	NOT_ECS() int mModelID = 0;
	NOT_ECS() int mCamp = 0;
};

#pragma once

#include "Grip.generated.h"

UENUM(BlueprintType)
enum class EGrip : uint8
{
	Open,
	CanGrab,
	Grab
};
#pragma once
#include "../Common/AnimationState.h"
#include "../Common/AnimationController.h"

class Dance2State : public AnimationState
{
public:
    Dance2State(
        const AnimationClip* clip,
        AnimationController* controller)
        : AnimationState("Dance_2", clip, controller) {
    }
};

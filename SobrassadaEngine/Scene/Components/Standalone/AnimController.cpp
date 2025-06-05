#include "AnimController.h"

#include "Application.h"
#include "EngineTimer.h"
#include "ResourceAnimation.h"
#include "ResourcesModule.h"

AnimController::AnimController()
    : resource(0), currentTime(0), loop(false), playAnimation(false), playbackSpeed(1.0f), currentAnimation(nullptr)
{
}

AnimController::~AnimController()
{
    App->GetResourcesModule()->ReleaseResource(currentAnimation);
}

void AnimController::Play(UID newResource, bool shouldLoop, float clipSpeed)
{
    if (currentAnimation == nullptr) Stop();
    resource          = newResource;
    currentTime       = 0.0f;
    loop              = shouldLoop;
    currentAnimation  = static_cast<ResourceAnimation*>(App->GetResourcesModule()->RequestResource(resource));
    playAnimation     = true;
    playAnimation     = true;
    animationFinished = false;
    playbackSpeed     = clipSpeed;
}

void AnimController::Stop()
{
    playAnimation = false;
    currentTime   = 0.0f;

    if (currentAnimation != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentAnimation);
        currentAnimation = nullptr;
    }
}

Quat AnimController::Interpolate(Quat& first, Quat& second, float lambda)
{
    if (first.Dot(second) >= 0.0f) return Quat::Lerp(first, second, lambda).Normalized();
    else return Quat::Lerp(first, second.Neg(), lambda).Normalized();
}

size_t AnimController::FindChannelIndex(const std::vector<float>& animChannelVector, float time) const
{
    int left  = 0;
    int right = (int)animChannelVector.size() - 1;

    while (left <= right)
    {
        int mid = (right + left) / 2;
        if (time == animChannelVector[mid]) return mid;
        else if (animChannelVector[mid] > time) right = mid - 1;
        else left = mid + 1;
    }

    return left;
}

update_status AnimController::Update(float deltaTime)
{
    if (!playAnimation || resource == INVALID_UID) return UPDATE_CONTINUE;

    deltaTime          *= playbackSpeed;

    float previousTime  = currentTime;
    currentTime        += deltaTime;

    if (currentAnimation == nullptr) return UPDATE_CONTINUE;

    const float duration = currentAnimation->GetDuration();

    if (targetAnimation != nullptr)
    {
        currentTargetTime          += deltaTime;
        const float targetDuration  = targetAnimation->GetDuration();
        if (currentTargetTime > targetDuration) currentTargetTime = fmod(currentTargetTime, targetDuration);

        fadeTime += deltaTime;

        if (fadeTime >= transitionTime)
        {
            App->GetResourcesModule()->ReleaseResource(currentAnimation);
            currentAnimation  = targetAnimation;
            targetAnimation   = nullptr;
            currentTime       = currentTargetTime;
            fadeTime          = 0;
            currentTargetTime = 0;
        }
    }
    else if (currentTime > duration)
    {
        if (loop)
        {
            currentTime = fmod(currentTime, duration);
        }
        else
        {
            currentTime       = duration;
            playAnimation     = false;
            animationFinished = true;

            // if (currentAnimation != nullptr)
            //{
            //     App->GetResourcesModule()->ReleaseResource(currentAnimation);
            //     currentAnimation = nullptr;
            // }
        }
    }

    return UPDATE_CONTINUE;
}

void AnimController::GetTransform(const HashString& nodeName, float3& pos, Quat& rot)
{
    if (!playAnimation || resource == INVALID_UID || currentAnimation == nullptr) return;

    if (targetAnimation == nullptr)
    {
        Channel* animChannel = currentAnimation->GetChannel(nodeName);
        if (animChannel == nullptr)
        {
            // GLOG("No channel for node %s", nodeName.c_str());
            return; // IMPORTANT: Don't modify pos/rot if no channel exists
        }

        // CRITICAL: Only modify position if there's position data
        // Otherwise leave the input position unchanged
        if (animChannel->numPositions > 0)
        {
            GetChannelPosition(animChannel, pos, currentTime);
        }

        // CRITICAL: Only modify rotation if there's rotation data
        // Otherwise leave the input rotation unchanged
        if (animChannel->numRotations > 0)
        {
            GetChannelRotation(animChannel, rot, currentTime);
        }
    }
    else
    {
        float weight               = transitionTime != 0 ? fadeTime / transitionTime : 1;
        Channel* animChannel       = currentAnimation->GetChannel(nodeName);
        Channel* targetAnimChannel = targetAnimation->GetChannel(nodeName);

        if (animChannel == nullptr && targetAnimChannel == nullptr)
        {
            // GLOG("No channel for node %s in either animation", nodeName.c_str());
            return; // Don't modify pos/rot if no channel exists in either animation
        }

        float3 animPos = float3(pos);
        Quat animQuat  = Quat(rot);

        // Only get transforms from animation channels if they exist
        if (animChannel)
        {
            if (animChannel->numPositions > 0)
            {
                GetChannelPosition(animChannel, animPos, currentTime);
            }
            if (animChannel->numRotations > 0)
            {
                GetChannelRotation(animChannel, animQuat, currentTime);
            }
        }

        float3 targetAnimPos = float3(pos);
        Quat targetAnimQuat  = Quat(rot);

        // Only get transforms from target animation channels if they exist
        if (targetAnimChannel)
        {
            if (targetAnimChannel->numPositions > 0)
            {
                GetChannelPosition(targetAnimChannel, targetAnimPos, currentTargetTime);
            }
            if (targetAnimChannel->numRotations > 0)
            {
                GetChannelRotation(targetAnimChannel, targetAnimQuat, currentTargetTime);
            }
        }

        // Blend the animations
        bool hasPositions = (animChannel && animChannel->numPositions > 0) ||
                            (targetAnimChannel && targetAnimChannel->numPositions > 0);
        bool hasRotations = (animChannel && animChannel->numRotations > 0) ||
                            (targetAnimChannel && targetAnimChannel->numRotations > 0);

        // Only blend if there's actual data to blend
        if (hasPositions)
        {
            pos = animPos.Lerp(targetAnimPos, weight);
        }
        if (hasRotations)
        {
            rot = Quat::Slerp(animQuat, targetAnimQuat, weight);
        }
    }
}

void AnimController::SetTargetAnimationResource(UID uid, unsigned timeTransition, bool shouldLoop)
{
    targetAnimation   = static_cast<ResourceAnimation*>(App->GetResourcesModule()->RequestResource(uid));
    transitionTime    = static_cast<float>(timeTransition) / 1000;
    loop              = shouldLoop;
    playAnimation     = true;
    animationFinished = false;
}

void AnimController::GetChannelPosition(const Channel* animChannel, float3& pos, const float time) const
{
    if (animChannel->numPositions > 0)
    {
        if (animChannel->numPositions == 1)
        {
            pos = animChannel->positions[0];
        }
        else
        {
            size_t nextIndex = FindChannelIndex(animChannel->posTimeStamps, time);

            size_t prevIndex = (nextIndex > 0) ? nextIndex - 1 : 0;

            if (nextIndex >= animChannel->numPositions)
            {
                pos = animChannel->positions[animChannel->numPositions - 1];
            }
            else if (nextIndex == 0)
            {
                pos = animChannel->positions[0];
            }
            else
            {
                const float startTime = animChannel->posTimeStamps[prevIndex];
                const float endTime   = animChannel->posTimeStamps[nextIndex];
                const float timeDiff  = endTime - startTime;

                float lambda          = (timeDiff > 0.0001f) ? (time - startTime) / timeDiff : 0.0f;

                lambda                = (lambda < 0) ? 0 : (lambda > 1) ? 1 : lambda;

                pos = float3::Lerp(animChannel->positions[prevIndex], animChannel->positions[nextIndex], lambda);

                // GLOG(
                //     "Position interpolation: From (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f) with lambda %.2f = "
                //     "(%.2f,%.2f,%.2f)",
                //     animChannel->positions[prevIndex].x, animChannel->positions[prevIndex].y,
                //     animChannel->positions[prevIndex].z, animChannel->positions[nextIndex].x,
                //     animChannel->positions[nextIndex].y, animChannel->positions[nextIndex].z, lambda, pos.x, pos.y,
                //     pos.z
                //);
            }
        }
    }
}

void AnimController::GetChannelRotation(Channel* animChannel, Quat& rot, const float time)
{
    if (animChannel->numRotations > 0)
    {
        if (animChannel->numRotations == 1)
        {
            rot = animChannel->rotations[0].Normalized();
        }
        else
        {
            size_t nextIndex = FindChannelIndex(animChannel->rotTimeStamps, time);

            size_t prevIndex = (nextIndex > 0) ? nextIndex - 1 : 0;

            if (nextIndex >= animChannel->numRotations)
            {
                rot = animChannel->rotations[animChannel->numRotations - 1].Normalized();
            }
            else if (nextIndex == 0)
            {
                rot = animChannel->rotations[0].Normalized();
            }
            else
            {
                const float startTime = animChannel->rotTimeStamps[prevIndex];
                const float endTime   = animChannel->rotTimeStamps[nextIndex];
                const float timeDiff  = endTime - startTime;

                float lambda          = (timeDiff > 0.0001f) ? (time - startTime) / timeDiff : 0.0f;

                lambda                = (lambda < 0) ? 0 : (lambda > 1) ? 1 : lambda;

                rot = Interpolate(animChannel->rotations[prevIndex], animChannel->rotations[nextIndex], lambda);
            }
        }
    }
}

#pragma once

class AABBUpdatable
{
public:

    virtual ~AABBUpdatable() = default;

    virtual void PassAABBUpdateToParent() = 0;
};

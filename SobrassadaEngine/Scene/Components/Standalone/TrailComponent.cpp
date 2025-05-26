#include "TrailComponent.h"

#include "Application.h"
#include "GameObject.h"
#include "ShaderModule.h"
#include "SplineComponent.h"
#include "glew.h"

TrailComponent::TrailComponent(UID uid, GameObject* parent) : Component(uid, parent, "Trail", COMPONENT_TRAIL)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(float3), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
}

TrailComponent::TrailComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(float3), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
}

TrailComponent::~TrailComponent()
{
}

void TrailComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
    spline->ClearPoints();
}

void TrailComponent::Clone(const Component* other)
{
}

void TrailComponent::Update(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (!spline)
    {
        spline = parent->GetComponent<SplineComponent*>();
        spline->ClearPoints();
    }
    if (!spline) return;

    for (TrailPoint& tp : points)
        tp.time += deltaTime;

    while (!points.empty() && points.front().time > lifeTime)
        points.pop_front();

    float4x4 globalMatrix = parent->GetGlobalTransform();
    float3 position       = globalMatrix.TranslatePart();
    float3 lastPos        = points.empty() ? float3::zero : points.back().position;

    spline->ClearPoints();
    vertices.clear();
    indices.clear();

    for (int i = 0; i < points.size(); ++i)
    {
        vertices.reserve(2 * points.size());
        indices.reserve(6 * (points.size() - 1));
        const TrailPoint& tp = points[i];

        spline->AddPoint(tp.position - position);
        float3 left  = tp.position - tp.perpendicular * width;
        float3 right = tp.position + tp.perpendicular * width;

        vertices.push_back(left);
        vertices.push_back(right);

        if (i > 0)
        {
            int leftCurrent  = 2 * i;
            int rightCurrent = 2 * i + 1;
            int leftPrev     = 2 * (i - 1);
            int rightPrev    = 2 * (i - 1) + 1;

            indices.push_back(leftPrev);
            indices.push_back(rightPrev);
            indices.push_back(leftCurrent);

            indices.push_back(rightPrev);
            indices.push_back(rightCurrent);
            indices.push_back(leftCurrent);
        }
    }
    RecalculateAABB();

    if (!points.empty() && (position - lastPos).LengthSq() < minDistance * minDistance) return;
    float3 perpendicular = globalMatrix.Col3(0);
    points.push_back({position, perpendicular, 0.0f});
}

void TrailComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (vertices.empty() || indices.empty()) return;

    const unsigned int program = App->GetShaderModule()->GetTrailProgram();
    glUseProgram(program);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float3), vertices.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());

    float4x4 modelMatrix = parent->GetGlobalTransform();
    glUniformMatrix4fv(4, 1, GL_FALSE, &modelMatrix[0][0]);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
}

void TrailComponent::RenderDebug(float deltaTime)
{
}

void TrailComponent::RenderEditorInspector()
{
}

void TrailComponent::ParentUpdated()
{
    RecalculateAABB();
}

void TrailComponent::RecalculateAABB()
{
    if (points.empty())
    {
        localComponentAABB = AABB(float3::zero, float3::zero);
        parent->OnAABBUpdated();
        return;
    }

    AABB aabb;
    float3 position = parent->GetGlobalTransform().TranslatePart();
    aabb.minPoint   = position - points[0].position;
    aabb.maxPoint   = position - points[0].position;

    for (const TrailPoint& tp : points)
    {
        float3 localPos = position - tp.position;
        float3 left   = localPos - tp.perpendicular * width;
        float3 right  = localPos + tp.perpendicular * width;

        aabb.minPoint = aabb.minPoint.Min(left);
        aabb.minPoint = aabb.minPoint.Min(right);
        aabb.maxPoint = aabb.maxPoint.Max(left);
        aabb.maxPoint = aabb.maxPoint.Max(right);
    }

    localComponentAABB = aabb;
    parent->OnAABBUpdated();
}

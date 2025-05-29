#include "TrailComponent.h"

#include "Application.h"
#include "GameObject.h"
#include "ShaderModule.h"
#include "SplineComponent.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_curves.h"

TrailComponent::TrailComponent(UID uid, GameObject* parent) : Component(uid, parent, "Trail", COMPONENT_TRAIL)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(TrailVertex), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)(sizeof(float3)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    gradientMarks = gradient.getMarks();
}

TrailComponent::TrailComponent(const rapidjson::Value& initialState, GameObject* parent)
    : Component(initialState, parent)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(TrailVertex), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)(sizeof(float3)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    if (initialState.HasMember("MinDistance")) minDistance = initialState["MinDistance"].GetFloat();
    if (initialState.HasMember("LifeTime")) lifeTime = initialState["LifeTime"].GetFloat();
    if (initialState.HasMember("Width")) width = initialState["Width"].GetFloat();
    if (initialState.HasMember("InvertCurve")) invertCurve = initialState["InvertCurve"].GetBool();

    if (initialState.HasMember("Curve"))
    {
        const rapidjson::Value& initCurve = initialState["Curve"];
        for (int i = 0; i < 5; ++i)
            curve[i] = initCurve[i].GetFloat();
    }

    //for (ImGradientMark* mark : gradient.getMarks())
    //    gradient.removeMark(mark);

    if (initialState.HasMember("Color"))
    {
        const rapidjson::Value& colorArray = initialState["Color"];
        for (rapidjson::SizeType i = 0; i < colorArray.Size(); i += 5)
        {
            float color[4] = {
                colorArray[i].GetFloat(), colorArray[i + 1].GetFloat(), colorArray[i + 2].GetFloat(),
                colorArray[i + 3].GetFloat()
            };
            float position = colorArray[i + 4].GetFloat();
            gradient.addMark(position, ImColor(color[0], color[1], color[2], color[3]));
        }

        gradientMarks = gradient.getMarks();
    }
}

TrailComponent::~TrailComponent()
{
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    if (spline) spline->ClearPoints();
    spline = nullptr;
}

void TrailComponent::Save(rapidjson::Value& targetState, rapidjson::Document::AllocatorType& allocator) const
{
    Component::Save(targetState, allocator);
    if (spline) spline->ClearPoints();
    targetState.AddMember("MinDistance", minDistance, allocator);
    targetState.AddMember("LifeTime", lifeTime, allocator);
    targetState.AddMember("Width", width, allocator);
    targetState.AddMember("InvertCurve", invertCurve, allocator);
    rapidjson::Value curveArray(rapidjson::kArrayType);
    curveArray.PushBack(curve[0], allocator)
        .PushBack(curve[1], allocator)
        .PushBack(curve[2], allocator)
        .PushBack(curve[3], allocator)
        .PushBack(curve[4], allocator);
    targetState.AddMember("Curve", curveArray, allocator);

    rapidjson::Value colorArray(rapidjson::kArrayType);
    for (const ImGradientMark* mark : gradientMarks)
    {
        colorArray.PushBack(mark->color[0], allocator);
        colorArray.PushBack(mark->color[1], allocator);
        colorArray.PushBack(mark->color[2], allocator);
        colorArray.PushBack(mark->color[3], allocator);
        colorArray.PushBack(mark->position, allocator);
    }
    targetState.AddMember("Color", colorArray, allocator);
}

void TrailComponent::Clone(const Component* other)
{
    if (other->GetType() != COMPONENT_TRAIL) return;

    const TrailComponent* otherTrail = static_cast<const TrailComponent*>(other);
    minDistance                      = otherTrail->minDistance;
    lifeTime                         = otherTrail->lifeTime;
    width                            = otherTrail->width;
    invertCurve                      = otherTrail->invertCurve;
    for (int i = 0; i < 5; ++i)
        curve[i] = otherTrail->curve[i];
    gradient   = otherTrail->gradient;
    enabled    = otherTrail->enabled;
    wasEnabled = otherTrail->wasEnabled;
}

void TrailComponent::Update(float deltaTime)
{
    for (TrailPoint& tp : points)
        tp.time += deltaTime;

    while (!points.empty() && points.front().time > lifeTime)
        points.pop_front();

    vertices.clear();
    indices.clear();

    if (!spline) spline = parent->GetComponent<SplineComponent*>();
    if (!spline) return;
    spline->ClearPoints();
    if (!IsEffectivelyEnabled()) return;
    if (!spline->IsEffectivelyEnabled()) return;

    float4x4 globalMatrix = parent->GetGlobalTransform();
    float3 position       = globalMatrix.TranslatePart();
    float3 lastPos        = points.empty() ? float3::zero : points.back().position;

    if (!points.empty())
    {
        vertices.reserve(2 * points.size());
        indices.reserve(6 * (points.size() - 1));
    }

    for (int i = 0; i < points.size(); ++i)
    {
        const TrailPoint& tp = points[i];

        spline->AddPoint(tp.position - position);

        float bezier = ImGui::BezierValue(tp.time / lifeTime, curve);
        float widthL = (invertCurve ? (1.0f - bezier) : bezier) * width;
        float3 left  = spline->GetPointLocal(i) - tp.perpendicular * widthL;
        float3 right = spline->GetPointLocal(i) + tp.perpendicular * widthL;

        float color[4];
        gradient.getColorAt(tp.time / lifeTime, color);
        float4 colorVec = float4(color[0], color[1], color[2], color[3]);

        vertices.push_back({left, colorVec});
        vertices.push_back({right, colorVec});

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
    float3 perpendicular = globalMatrix.Col3(0).Normalized();
    points.push_back({position, perpendicular, 0.0f});
}

void TrailComponent::Render(float deltaTime)
{
    if (!IsEffectivelyEnabled()) return;
    if (vertices.empty() || indices.empty()) return;

    const unsigned int program = App->GetShaderModule()->GetTrailProgram();
    glUseProgram(program);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(TrailVertex), vertices.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());

    float4x4 modelMatrix = parent->GetGlobalTransform();
    glUniformMatrix4fv(4, 1, GL_TRUE, &modelMatrix[0][0]);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
}

void TrailComponent::RenderDebug(float deltaTime)
{
}

void TrailComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Trail Component");

    ImGui::DragFloat("Min Distance", &minDistance, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("LifeTime", &lifeTime, 0.01f, 0.0f, 2.0f);
    ImGui::DragFloat("Width", &width, 0.01f, 0.0f, 5.0f);

    ImGui::Checkbox("Invert Curve", &invertCurve);
    ImGui::Bezier("Trail Curve", curve);

    if (ImGui::GradientEditor(&gradient, draggingMark, selectedMark))
    {
        gradientMarks.clear();
        gradientMarks = gradient.getMarks();
    }
}

void TrailComponent::ParentUpdated()
{
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
    aabb.minPoint   = spline->GetPointLocal(0);
    aabb.maxPoint   = spline->GetPointLocal(0);

    for (int i = 0; i < points.size(); ++i)
    {
        const TrailPoint& tp = points[i];
        float3 localPos      = spline->GetPointLocal(i);
        float3 left          = localPos - tp.perpendicular * width;
        float3 right         = localPos + tp.perpendicular * width;

        aabb.minPoint        = aabb.minPoint.Min(left);
        aabb.minPoint        = aabb.minPoint.Min(right);
        aabb.maxPoint        = aabb.maxPoint.Max(left);
        aabb.maxPoint        = aabb.maxPoint.Max(right);
    }

    localComponentAABB = aabb;
    parent->OnAABBUpdated();
}

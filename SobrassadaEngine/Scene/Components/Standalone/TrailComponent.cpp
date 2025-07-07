#include "TrailComponent.h"

#include "Application.h"
#include "CameraComponent.h"
#include "CameraModule.h"
#include "EditorUIModule.h"
#include "GameObject.h"
#include "Interpolation.h"
#include "LibraryModule.h"
#include "ResourceTexture.h"
#include "ResourcesModule.h"
#include "ShaderModule.h"
#include "SplineComponent.h"
#include "glew.h"
#include "imgui.h"
#include "imgui_color_gradient.h"
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

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)(sizeof(float3) + sizeof(float4)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    gradient = new ImGradient();

    vertices.reserve(maxVertices * sizeof(TrailVertex));
    indices.reserve(maxIndices * sizeof(uint32_t));

    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
    parent->OnAABBUpdated();
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

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)(sizeof(float3) + sizeof(float4)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    if (initialState.HasMember("MinDistance")) minDistance = initialState["MinDistance"].GetFloat();
    if (initialState.HasMember("UseCurve")) useCurve = initialState["UseCurve"].GetBool();
    if (initialState.HasMember("LifeTime")) lifeTime = initialState["LifeTime"].GetFloat();
    if (initialState.HasMember("Width")) width = initialState["Width"].GetFloat();
    if (initialState.HasMember("InvertCurve")) invertCurve = initialState["InvertCurve"].GetBool();
    if (initialState.HasMember("Cutoff")) cutoff = initialState["Cutoff"].GetFloat();

    if (initialState.HasMember("Curve"))
    {
        const rapidjson::Value& initCurve = initialState["Curve"];
        for (int i = 0; i < 5; ++i)
            curve[i] = initCurve[i].GetFloat();
    }

    if (initialState.HasMember("HasTexture")) hasTexture = initialState["HasTexture"].GetBool();
    if (initialState.HasMember("Texture")) UpdateTexture(initialState["Texture"].GetUint64());

    gradient = new ImGradient();
    gradient->getMarks().clear();

    if (initialState.HasMember("Color"))
    {
        const rapidjson::Value& colorArray = initialState["Color"];
        for (rapidjson::SizeType i = 0; i < colorArray.Size(); i += 5)
        {
            const float color[4] = {
                colorArray[i].GetFloat(), colorArray[i + 1].GetFloat(), colorArray[i + 2].GetFloat(),
                colorArray[i + 3].GetFloat()
            };
            const float position = colorArray[i + 4].GetFloat();
            gradient->addMark(position, ImColor(color[0], color[1], color[2], color[3]));
        }
    }

    vertices.reserve(maxVertices * sizeof(TrailVertex));
    indices.reserve(maxIndices * sizeof(uint32_t));

    localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
    parent->OnAABBUpdated();
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
    targetState.AddMember("Cutoff", cutoff, allocator);
    targetState.AddMember("UseCurve", useCurve, allocator);
    targetState.AddMember("InvertCurve", invertCurve, allocator);
    rapidjson::Value curveArray(rapidjson::kArrayType);
    curveArray.PushBack(curve[0], allocator)
        .PushBack(curve[1], allocator)
        .PushBack(curve[2], allocator)
        .PushBack(curve[3], allocator)
        .PushBack(curve[4], allocator);
    targetState.AddMember("Curve", curveArray, allocator);

    targetState.AddMember(
        "Texture", currentTexture != nullptr ? currentTexture->GetUID() : FALLBACK_TEXTURE_UID, allocator
    );
    targetState.AddMember("HasTexture", hasTexture, allocator);

    rapidjson::Value colorArray(rapidjson::kArrayType);
    for (const ImGradientMark* mark : gradient->getMarks())
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
    if (other->GetType() == ComponentType::COMPONENT_TRAIL)
    {
        const TrailComponent* otherTrail = static_cast<const TrailComponent*>(other);
        minDistance                      = otherTrail->minDistance;
        lifeTime                         = otherTrail->lifeTime;
        width                            = otherTrail->width;
        cutoff                           = otherTrail->cutoff;
        invertCurve                      = otherTrail->invertCurve;
        useCurve                         = otherTrail->useCurve;
        for (int i = 0; i < 5; ++i)
            curve[i] = otherTrail->curve[i];
        gradient   = otherTrail->gradient;
        enabled    = otherTrail->enabled;
        wasEnabled = otherTrail->wasEnabled;
        hasTexture = otherTrail->hasTexture;
        UpdateTexture(otherTrail->currentTextureUID);
    }
}

void TrailComponent::Update(float deltaTime)
{
    vertices.clear();
    indices.clear();

    for (TrailPoint& tp : points)
        tp.time += deltaTime;
    if (!points.empty() && points.front().time > lifeTime) points.pop_front();

    float3 cameraPos;
    if (App->GetSceneModule()->GetInPlayMode() && App->GetSceneModule()->GetScene()->GetMainCamera() != nullptr)
    {
        cameraPos = App->GetSceneModule()->GetScene()->GetMainCamera()->GetCameraPosition();
    }
    else cameraPos = App->GetCameraModule()->GetCameraPosition();

    const float3 position = parent->GetGlobalTransform().TranslatePart();
    const float3 lastPos  = points.empty() ? float3::zero : points.back().position;

    if (!spline) spline = parent->GetComponent<SplineComponent*>();

    if (IsEffectivelyEnabled() && (points.empty() || (position - lastPos).LengthSq() >= minDistance * minDistance))
    {
        const float3 viewDir       = (cameraPos - position).Normalized();

        const float3 direction     = (position - lastPos).Normalized();
        const float3 up            = float3::unitY;
        const float3 perpendicular = direction.Cross(viewDir).Normalized();

        points.push_back({position, perpendicular, 0.0f});
    }

    const int smoothCount = 8; // últimos N puntos a suavizar

    std::vector<TrailPoint> renderPoints;
    const int smoothStart = std::max(0, (int)points.size() - smoothCount);

    for (int i = 0; i < smoothStart; ++i)
        renderPoints.push_back(points[i]);
    if (points.size() >= 4 && spline)
    {
        const int stepsPerSegment = 2;

        for (int i = std::max(1, smoothStart - 1); i < points.size() - 2; ++i)
        {
            const TrailPoint& P0 = points[i - 1];
            const TrailPoint& P1 = points[i];
            const TrailPoint& P2 = points[i + 1];
            const TrailPoint& P3 = points[i + 2];

            for (int step = 0; step < stepsPerSegment; ++step)
            {
                const float t        = (float)step / stepsPerSegment;
                const float3 pos     = spline->CatmullRom(P0.position, P1.position, P2.position, P3.position, t);

                const float3 viewDir = (cameraPos - P2.position).Normalized();

                const float3 dir     = (P2.position - P1.position).Normalized();
                const float3 perp    = dir.Cross(viewDir).Normalized();

                const float interpolatedTime = Interpolation::Lerp(P1.time, P2.time, t);
                renderPoints.push_back({pos, perp, interpolatedTime});
            }
        }

        renderPoints.push_back(points.back());
    }
    else renderPoints.assign(points.begin(), points.end());

    for (int i = 0; i < renderPoints.size(); ++i)
    {
        const TrailPoint tp        = renderPoints[i];
        const float normalizedTime = tp.time / lifeTime;

        float widthL;
        if (useCurve)
        {
            const float bezier = ImGui::BezierValue(normalizedTime, curve);
            widthL             = (invertCurve ? (1.0f - bezier) : bezier) * width;
        }
        else widthL = width;

        const float3 left  = tp.position - tp.perpendicular * widthL;
        const float3 right = tp.position + tp.perpendicular * widthL;

        float color[4];
        gradient->getColorAt(normalizedTime, color);
        const float4 colorVec = float4(color[0], color[1], color[2], color[3]);

        vertices.push_back({left, colorVec, float2(normalizedTime, 0.0f)});
        vertices.push_back({right, colorVec, float2(normalizedTime, 1.0f)});

        if (i > 0)
        {
            const int leftCurrent  = 2 * i;
            const int rightCurrent = 2 * i + 1;
            const int leftPrev     = 2 * (i - 1);
            const int rightPrev    = 2 * (i - 1) + 1;

            indices.push_back(leftPrev);
            indices.push_back(rightPrev);
            indices.push_back(leftCurrent);

            indices.push_back(rightPrev);
            indices.push_back(rightCurrent);
            indices.push_back(leftCurrent);
        }
    }

    RecalculateAABB();
}

void TrailComponent::Render(float deltaTime)
{
    if (vertices.empty() || indices.empty()) return;

    const unsigned int program = App->GetShaderModule()->GetTrailProgram();
    glUseProgram(program);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(TrailVertex), vertices.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());

    glUniformMatrix4fv(4, 1, GL_TRUE, &modelMatrix[0][0]);

    if (hasTexture && currentTexture != nullptr)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture->GetTextureID());
        glUniform1i(glGetUniformLocation(program, "useTexture"), 1);
    }
    else glUniform1i(glGetUniformLocation(program, "useTexture"), 0);

    glUniform1f(glGetUniformLocation(program, "cutOff"), cutoff);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TrailComponent::RenderDebug(float deltaTime)
{
}

void TrailComponent::RenderEditorInspector()
{
    Component::RenderEditorInspector();

    ImGui::SeparatorText("Trail Component");

    ImGui::DragFloat("Min Distance", &minDistance, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("LifeTime", &lifeTime, 0.01f, 0.1f, 2.0f);
    ImGui::DragFloat("Width", &width, 0.01f, 0.0f, 5.0f);
    ImGui::DragFloat("Cutoff", &cutoff, 0.01f, 0.0f, 1.0f);

    ImGui::NewLine();
    ImGui::Checkbox("Use Curve", &useCurve);
    if (useCurve)
    {
        ImGui::Checkbox("Invert Curve", &invertCurve);
        ImGui::Bezier("Trail Curve", curve);
    }

    ImGui::NewLine();
    ImGui::GradientEditor(gradient, draggingMark, selectedMark);

    ImGui::Checkbox("Has Texture", &hasTexture);
    if (hasTexture)
    {
        if (ImGui::Button("Select texture"))
        {
            ImGui::OpenPopup(CONSTANT_TEXTURE_SELECT_DIALOG_ID);
        }

        if (ImGui::IsPopupOpen(CONSTANT_TEXTURE_SELECT_DIALOG_ID))
        {
            UpdateTexture(App->GetEditorUIModule()->RenderResourceSelectDialog<UID>(
                CONSTANT_TEXTURE_SELECT_DIALOG_ID, App->GetLibraryModule()->GetTextureMap(), INVALID_UID
            ));
        }

        if (currentTexture != nullptr)
        {
            ImGui::Text("Texture");
            ImGui::Image((ImTextureID)(intptr_t)currentTexture->GetTextureID(), ImVec2(256, 256));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(
                    "Texture Dimensions: %d, %d", currentTexture->GetTextureWidth(), currentTexture->GetTextureWidth()
                );
            }
        }
    }
}

void TrailComponent::UpdateTexture(UID newTextureUID)
{
    if (currentTexture != nullptr && currentTexture->GetUID() == newTextureUID) return;

    ResourceTexture* newTexture =
        dynamic_cast<ResourceTexture*>(App->GetResourcesModule()->RequestResource(newTextureUID));

    if (newTexture != nullptr)
    {
        App->GetResourcesModule()->ReleaseResource(currentTexture);
        currentTexture      = newTexture;
        currentResourceName = currentTexture->GetName();
        currentTextureUID   = currentTexture->GetUID();
    }
}

void TrailComponent::ParentUpdated()
{
}

void TrailComponent::RecalculateAABB()
{
    if (points.empty())
    {
        //localComponentAABB = AABB(float3::zero, float3::zero);
        localComponentAABB = AABB(float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5));
        parent->OnAABBUpdated();
        return;
    }

    AABB globalAABB;
    globalAABB.minPoint = float3::inf;
    globalAABB.maxPoint = -float3::inf;

    for (const TrailPoint& tp : points)
    {
        const float3 left  = tp.position - tp.perpendicular * width;
        const float3 right = tp.position + tp.perpendicular * width;

        globalAABB.Enclose(float3(left.x + 0.1f, left.y + 0.1f, left.z + 0.1f));
        globalAABB.Enclose(float3(right.x - 0.1f, right.y - 0.1f, right.z - 0.1f));
    }

    // Convertir AABB global a espacio local
    const float4x4 invTransform = parent->GetGlobalTransform().Inverted();
    const float3 localMin       = invTransform.MulPos(globalAABB.minPoint);
    const float3 localMax       = invTransform.MulPos(globalAABB.maxPoint);

    localComponentAABB          = AABB(localMin.Min(localMax), localMin.Max(localMax));
    parent->OnAABBUpdated();
}

#include "CubeMesh.h"

CubeMesh::CubeMesh(UID uid, UID uidParent, UID uidRoot, const Transform& parentGlobalTransform)
: Component(uid, uidParent, uidRoot, "Mesh component", COMPONENT_CUBE_MESH, parentGlobalTransform)
{

}

void CubeMesh::Update()
{
}

void CubeMesh::Render()
{
    Component::Render();
}

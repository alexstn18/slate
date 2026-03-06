#include "pch.hpp"
#include "rendering/render_components.hpp"
#include "rendering/texture.hpp"

using namespace slate;

void Transform::SetMatrixDirty()
{
    m_MatrixDirty = true;
}

const glm::mat4& Transform::World()
{
    if (m_MatrixDirty) {
        const auto translation = glm::translate(glm::mat4(1.0f), m_Translation);
        const auto rotation = glm::toMat4(m_Rotation);
        const auto scale = glm::scale(glm::mat4(1.0f), m_Scale);
        m_WorldMatrix = translation * rotation * scale;
        
        m_MatrixDirty = false;
    }

    return m_WorldMatrix;
}

void Transform::SetFromMatrix(const glm::mat4& transform)
{
    m_Translation.x = transform[3][0];
    m_Translation.y = transform[3][1];
    m_Translation.z = transform[3][2];

    m_Scale.x = glm::length(glm::vec3(transform[0][0], transform[0][1], transform[0][2]));
    m_Scale.y = glm::length(glm::vec3(transform[1][0], transform[1][1], transform[1][2]));
    m_Scale.z = glm::length(glm::vec3(transform[2][0], transform[2][1], transform[2][2]));

    glm::mat4 myrot(transform[0][0] / m_Scale.x,
        transform[0][1] / m_Scale.x,
        transform[0][2] / m_Scale.x,
        0,
        transform[1][0] / m_Scale.y,
        transform[1][1] / m_Scale.y,
        transform[1][2] / m_Scale.y,
        0,
        transform[2][0] / m_Scale.z,
        transform[2][1] / m_Scale.z,
        transform[2][2] / m_Scale.z,
        0,
        0,
        0,
        0,
        1);
    m_Rotation = glm::quat_cast(myrot);

    SetMatrixDirty();
}

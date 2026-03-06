#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <imgui/ImReflect.hpp>

namespace ImReflect
{
	inline void tag_invoke(ImInput_t, const char* label, glm::vec3& v,
		ImSettings& settings, ImResponse& response)
	{
		auto& r = response.get<glm::vec3>();
		if (ImGui::DragFloat3(label, &v.x, 0.05f)) r.changed();
		ImReflect::Detail::check_input_states(r);
	}
}

namespace slate {
	class Texture;

	struct Material {
		std::shared_ptr<Texture> Albedo{ nullptr };
		std::shared_ptr<Texture> Normal{ nullptr };
		std::shared_ptr<Texture> OcclusionRoughnessMetallic{ nullptr };
		std::shared_ptr<Texture> Emissive{ nullptr };
	
		glm::vec4 albedoFactor{ 1.0f };
		float metallicFactor{ 1.0f };
		float roughnessFactor{ 1.0f };
		glm::vec3 emissiveFactor{ 0.0f };
	};

	struct Light {
		glm::vec3 Color{ 1.0f };
		float Intensity{ 0.5f };
		glm::vec3 Position{ 0.0f };
		float Range{ 10000.0f };
		glm::vec3 Direction{ 1.0f };
		enum class Type : u32 {
			Directional = 0u,
			Point,
		} type = Type::Directional;
	};

	struct Transform {
	private:
		glm::vec3 m_Translation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_Scale{ 0.0f, 0.0f, 0.0f };
		glm::quat m_Rotation{ glm::identity<glm::quat>() };

		glm::mat4 m_WorldMatrix{ glm::identity<glm::mat4>() };
		bool m_MatrixDirty{ false };

		void SetMatrixDirty();
	public:
		/// <summary>Gets this Transform's translation in local space.</summary>
		inline const glm::vec3& GetTranslation() const { return m_Translation; }
		/// <summary>Gets this Transform's scale in local space.</summary>
		inline const glm::vec3& GetScale() const { return m_Scale; }
		/// <summary>Gets this Transform's rotation in local space.</summary>
		inline const glm::quat& GetRotation() const { return m_Rotation; }
	
		/// <summary>Gets the matrix that transforms from local space to world space.
		/// Calling this function recomputes the matrix when necessary.</summary>
		const glm::mat4& World();

		/// <summary>Updates the translation of this Transform.
		/// Also marks this Transform and its children as dirty.</summary>
		/// <param name="translation">The new translation vector to use.</param>
		void SetTranslation(const glm::vec3& translation)
		{
			m_Translation = translation;
			SetMatrixDirty();
		}

		/// <summary>Updates the scale of this Transform.
		/// Also marks this Transform and its children as dirty.</summary>
		/// <param name="scale">The new scale vector to use.</param>
		void SetScale(const glm::vec3& scale)
		{
			m_Scale = scale;
			SetMatrixDirty();
		}

		/// <summary>Updates the rotation of this Transform.
		/// Also marks this Transform and its children as dirty.</summary>
		/// <param name="rotation">The new rotation quaternion to use.</param>
		void SetRotation(const glm::quat& rotation)
		{
			m_Rotation = rotation;
			SetMatrixDirty();
		}

		/// Decomposes a transformation matrix into its translation, scale and rotation components,
		/// and stores the result in this Transform.
		void SetFromMatrix(const glm::mat4& transform);
	};

	struct Camera {
		glm::vec3 position{ 0.f, 0.f, 5.f };
		float yaw{ 0.f };     // radians
		float pitch{ 0.f };   // radians
		float fovY{ glm::radians(45.f) };
		float nearZ{ 0.1f };
		float farZ{ 1000.f };

		glm::mat4 GetView() const {
			glm::vec3 fwd{
				cos(pitch) * sin(yaw),
				sin(pitch),
				cos(pitch) * cos(yaw)
			};
			return glm::lookAtLH(position, position + fwd, { 0,1,0 });
		}

		glm::mat4 GetProjection(float width, float height) const {
			return glm::perspectiveFovLH(fovY, width, height, nearZ, farZ);
		}
	};

	struct ModelConstants {
		glm::mat4 NormalMatrix{};
		glm::mat4 Model{};
		glm::mat4 MVP{};
		glm::vec3 cameraPos{};
		u32 LightCount = 1u;
	};
}

IMGUI_REFLECT(slate::Light, Intensity, Position, Range, Direction)
IMGUI_REFLECT(slate::Camera, position)
#pragma once
#include <glm/glm.hpp>

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
		glm::vec3 Position{ 0.0f };
		glm::vec3 Direction{ 1.0f };
		
		float Intensity{ 0.5f };
		
		enum class Type : i32 {
			Ambient = 0,
			Point,
			Directional,
		} type = Type::Point;
	};
}
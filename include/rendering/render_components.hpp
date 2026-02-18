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
}
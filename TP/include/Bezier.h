#pragma once

#include <vector>
#include <unordered_map>

#include <Vroom/Asset/AssetData/MeshData.h>

#include <glm/glm.hpp>

template <>
struct std::hash<std::pair<uint32_t, uint32_t>>
{
	size_t operator()(const std::pair<uint32_t, uint32_t>& element) const noexcept
	{
		// From Boost lib (hash_combine)
		std::hash<uint32_t> hasher;
		size_t seed = hasher(element.first);
		return seed ^= hasher(element.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
};

class Bezier
{
public:
	Bezier(uint32_t degreeU, uint32_t degreeV, uint32_t resolutionU, uint32_t resolutionV);

	void setControlPoint(uint32_t u, uint32_t v, const glm::vec3& p);
	void setDegrees(uint32_t degreeU, uint32_t degreeV);
	void setResolution(uint32_t resolutionU, uint32_t resolutionV);

	const glm::vec3& getControlPoint(uint32_t u, uint32_t v) const;

	const vrm::MeshData& polygonize() const;

private:
	void computeMesh() const;
	glm::vec3 computeBezier(float u, float v) const;

protected:
	inline static uint32_t Binomial(uint32_t n, uint32_t k);
	inline static uint32_t Factorial(uint32_t n);
	inline static float Bernstein(uint32_t n, uint32_t k, float t);

private:
	uint32_t m_DegreeU, m_DegreeV;
	uint32_t m_ResolutionU, m_ResolutionV;
	std::vector<glm::vec3> m_ControlPoints;

	mutable vrm::MeshData m_PolygonizedCache;
	mutable bool m_NeedsCompute = true;

	static std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t> s_Binomials;
	static std::vector<uint32_t> s_Factorials;
};
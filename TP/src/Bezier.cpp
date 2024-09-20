#include "Bezier.h"

#include <Vroom/Core/Log.h>

std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t> Bezier::s_Binomials;
std::vector<uint32_t> Bezier::s_Factorials = { 1, 1 };

Bezier::Bezier(uint32_t degreeU, uint32_t degreeV, uint32_t resolutionU, uint32_t resolutionV)
	: m_DegreeU(degreeU), m_DegreeV(degreeV), m_ResolutionU(resolutionU), m_ResolutionV(resolutionV)
{
	m_ControlPoints.assign(static_cast<size_t>((degreeU + 1) * (degreeV + 1)), glm::vec3());
}

void Bezier::setControlPoint(uint32_t u, uint32_t v, const glm::vec3& p)
{
	m_ControlPoints.at(static_cast<size_t>(u * (m_DegreeV + 1) + v)) = p;
	m_NeedsCompute = true;
}

void Bezier::setDegrees(uint32_t degreeU, uint32_t degreeV)
{
	m_DegreeU = degreeU;
	m_DegreeV = degreeV;

	m_NeedsCompute = true;
}

void Bezier::setResolution(uint32_t resolutionU, uint32_t resolutionV)
{
	m_ResolutionU = resolutionU;
	m_ResolutionV = resolutionV;

	m_NeedsCompute = true;
}

const glm::vec3& Bezier::getControlPoint(uint32_t u, uint32_t v) const
{
	return m_ControlPoints.at(static_cast<size_t>(u * (m_DegreeV + 1) + v));
}

const vrm::MeshData& Bezier::polygonize() const
{
	if (m_NeedsCompute)
		computeMesh();

	return m_PolygonizedCache;
}

void Bezier::computeMesh() const
{
	std::vector<vrm::Vertex> vertices;
	std::vector<uint32_t> indices;

	size_t triangleCount = static_cast<size_t>(m_ResolutionU) * static_cast<size_t>(m_ResolutionV) * 2;
	indices.reserve(triangleCount * 3);
	vertices.reserve(triangleCount * 3);

	for (uint32_t sampleU = 0; sampleU < m_ResolutionU - 1; sampleU++)
	{
		for (uint32_t sampleV = 0; sampleV < m_ResolutionV - 1; sampleV++)
		{
			const float u0 = static_cast<float>(sampleU) / static_cast<float>(m_ResolutionU);
			const float u1 = static_cast<float>(sampleU + 1) / static_cast<float>(m_ResolutionU);
			const float v0 = static_cast<float>(sampleV) / static_cast<float>(m_ResolutionV);
			const float v1 = static_cast<float>(sampleV + 1) / static_cast<float>(m_ResolutionV);

			vrm::Vertex A;
				A.position = computeBezier(u0, v0);
			vrm::Vertex B;
				B.position = computeBezier(u1, v0);
			vrm::Vertex C;
				C.position = computeBezier(u1, v1);
			vrm::Vertex D;
				D.position = computeBezier(u0, v1);

			glm::vec3 AC = C.position - A.position;
			glm::vec3 AD = D.position - A.position;
			glm::vec3 AB = B.position - A.position;
			
			glm::vec3 normal0 = glm::normalize(glm::cross(AB, AC));
			glm::vec3 normal1 = glm::normalize(glm::cross(AC, AD));

			A.normal = normal0;
			B.normal = normal0;
			C.normal = normal0;

			uint32_t offset = static_cast<uint32_t>(vertices.size());
			indices.push_back(offset + 0);
			indices.push_back(offset + 1);
			indices.push_back(offset + 2);
			vertices.push_back(A);
			vertices.push_back(B);
			vertices.push_back(C);

			A.normal = normal1;
			C.normal = normal1;
			D.normal = normal1;

			offset += 3;
			indices.push_back(offset + 0);
			indices.push_back(offset + 1);
			indices.push_back(offset + 2);
			vertices.push_back(A);
			vertices.push_back(C);
			vertices.push_back(D);
		}
	}

	m_PolygonizedCache = vrm::MeshData(std::move(vertices), std::move(indices));

	m_NeedsCompute = false;
}

glm::vec3 Bezier::computeBezier(float u, float v) const
{
	glm::vec3 out = glm::vec3{ 0.0, 0.0, 0.0 };

	for (uint32_t i = 0; i < (m_DegreeU + 1); i++)
	{
		for (uint32_t j = 0; j < (m_DegreeV + 1); j++)
		{
			out += Bernstein(m_DegreeU, i, u) * Bernstein(m_DegreeV, j, v) * getControlPoint(i, j);
		}
	}

	return out;
}

uint32_t Bezier::Binomial(uint32_t n, uint32_t k)
{
	if (!s_Binomials.contains({ n, k }))
		s_Binomials[{n, k}] = (Factorial(n) / Factorial(k) / Factorial(n - k));

	return s_Binomials.at({ n, k });
}

uint32_t Bezier::Factorial(uint32_t n)
{
	s_Factorials.reserve(static_cast<size_t>(n) + 1);

	for (; !(s_Factorials.size() > n);)
		s_Factorials.push_back(s_Factorials.back() * static_cast<uint32_t>(s_Factorials.size()));

	return s_Factorials.at(n);
}

float Bezier::Bernstein(uint32_t n, uint32_t k, float t)
{
	return static_cast<float>(
		Binomial(n, k))
		* powf(t, static_cast<float>(k))
		* powf((1.f - t), static_cast<float>(n - k)
	);
}

#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	return (1.0f - x) * p0 + x * p1;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	auto X = glm::vec4(1.0, x, x * x, x * x * x);
	auto A = glm::mat4x4(
		0.0,  -t,       2.0 * t,      -t,
		1.0, 0.0,       t - 3.0, 2.0 - t,
		0.0,   t, 3.0 - 2.0 * t, t - 2.0,
		0.0, 0.0,            -t,       t
	);
	auto ws = X * A;

	auto p_inter = ws[0] * p0 + ws[1] * p1 + ws[2] * p2 + ws[3] * p3;
	return p_inter;
}

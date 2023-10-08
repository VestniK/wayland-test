#include <scene/triangular_net.hpp>

namespace triangular {

const glm::mat2 to_cartesian_transformation{
    1., 0., std::cos(M_PI / 3), std::sin(M_PI / 3)};

} // namespace triangular

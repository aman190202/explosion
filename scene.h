#include "Eigen/Dense"
#include <cmath>

bool intersectGround(const Eigen::Vector3d& rayOrigin, 
                    const Eigen::Vector3d& rayDir,
                    double& t) {
    // Ground plane is at y = 0
    if (std::abs(rayDir.y()) < 1e-6) return false; // Ray parallel to plane
    
    t = -rayOrigin.y() / rayDir.y();
    return t > 0;
}

// Function to calculate checkerboard pattern color
Eigen::Vector3d getGroundColor(const Eigen::Vector3d& point) {
    // Create checkerboard pattern
    int x = static_cast<int>(std::floor(point.x() / 2.0));
    int z = static_cast<int>(std::floor(point.z() / 2.0));
    
    if ((x + z) % 2 == 0) {
        return Eigen::Vector3d(0.8, 0.8, 0.8); // Light gray
    } else {
        return Eigen::Vector3d(0.2, 0.2, 0.2); // Dark gray
    }
}
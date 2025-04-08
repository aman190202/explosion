#include <iostream>
#include <omp.h>

#include "Image.h"
#include "Camera.h"
#include "Lighting.h"

// Function to check if a ray intersects with the ground plane
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

int main() {
    // Create image
    const int width = 800;
    const int height = 600;
    Image image(width, height);
    
    // Create camera
    Camera camera(
        Eigen::Vector3d(0, 10, 20),  // Position camera above and behind the ground
        Eigen::Vector3d(0, 0, 0),   // Look at the origin
        Eigen::Vector3d(0, 1, 0),   // Up vector
        60.0,                       // Field of view
        static_cast<double>(width) / height, // Aspect ratio
        0.1,                        // Near plane
        1000.0                      // Far plane
    );
    
    // Create lighting system
    Lighting lighting;
    
    // Create a grid of small point lights
    const int gridSize = 5;
    const double spacing = 1.0;
    const double lightHeight = 5.0;
    const double lightRadius = 0.001;  // Very small radius for point-like appearance
    
    for (int i = -gridSize/2; i <= gridSize/2; ++i) {
        for (int j = -gridSize/2; j <= gridSize/2; ++j) {
            // Create a light at each grid point
            Light light(
                Eigen::Vector3d(i * spacing, lightHeight, j * spacing),  // Position
                // make light color factor of i and j
                Eigen::Vector3d(i/10.0, j/10.0, 0),                               // White light
                2.0,                                                    // Intensity
                lightRadius                                            // Small radius
            );
            lighting.addLight(light);
        }
    }
    
    // Render the scene
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Convert pixel coordinates to normalized device coordinates
            double u = static_cast<double>(x) / width;
            double v = 1.0 - static_cast<double>(y) / height;  // Invert v-coordinate
            
            // Generate ray from camera
            Eigen::Vector3d rayDir = camera.generateRay(u, v);
            Eigen::Vector3d rayOrigin = camera.getPosition();
            
            // Check for intersection with ground
            double t;
            if (intersectGround(rayOrigin, rayDir, t)) {
                // Calculate intersection point
                Eigen::Vector3d hitPoint = rayOrigin + t * rayDir;
                
                // Get base color from checkerboard pattern
                Eigen::Vector3d baseColor = getGroundColor(hitPoint);
                
                // Calculate normal (always pointing up for ground plane)
                Eigen::Vector3d normal(0, 1, 0);
                
                // Calculate view direction
                Eigen::Vector3d viewDir = -rayDir.normalized();
                
                // Calculate final color using Phong lighting
                Eigen::Vector3d finalColor = lighting.calculatePhongLighting(
                    hitPoint, normal, viewDir, baseColor);
                
                // Set pixel color
                image.setPixel(x, y, finalColor.x(), finalColor.y(), finalColor.z());
            } else 
            {
                // Sky color
                image.setPixel(x, y, 0, 0, 0);
            }
        }
    }
    
    
    // Save the image
    if (image.savePPM("lighted_scene.ppm")) {
        std::cout << "Image saved successfully as lighted_scene.ppm" << std::endl;
    } else {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }
    
    return 0;
} 
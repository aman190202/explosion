#ifndef LIGHTING_H
#define LIGHTING_H

#include "Eigen/Dense"
#include <vector>

class Light {
public:
    Eigen::Vector3d position;
    Eigen::Vector3d color;
    double intensity;
    double radius;  // Size of the light point

    Light(const Eigen::Vector3d& pos = Eigen::Vector3d(0, 2, 0),
          const Eigen::Vector3d& col = Eigen::Vector3d(1, 1, 1),
          double inten = 1.0,
          double rad = 0.1)
        : position(pos), color(col), intensity(inten), radius(rad) {}
};

class Lighting {
private:
    std::vector<Light> lights;
    
    // Phong BRDF parameters
    double ambientCoefficient;
    double diffuseCoefficient;
    double specularCoefficient;
    double shininess;

    // Helper function to calculate reflection vector
    Eigen::Vector3d reflect(const Eigen::Vector3d& incident, const Eigen::Vector3d& normal) const {
        return incident - 2.0 * incident.dot(normal) * normal;
    }

public:
    Lighting()
        : ambientCoefficient(0.2),    // Increased ambient
          diffuseCoefficient(0.8),    // Increased diffuse
          specularCoefficient(0.5),   // Increased specular
          shininess(16.0) {}          // Reduced shininess for broader highlights

    // Add a new light
    void addLight(const Light& light) {
        lights.push_back(light);
    }


    // Clear all lights
    void clearLights() {
        lights.clear();
    }

    // Getters and setters for Phong parameters
    void setAmbientCoefficient(double coeff) { ambientCoefficient = coeff; }
    void setDiffuseCoefficient(double coeff) { diffuseCoefficient = coeff; }
    void setSpecularCoefficient(double coeff) { specularCoefficient = coeff; }
    void setShininess(double s) { shininess = s; }

    // Calculate Phong BRDF lighting from all lights
    Eigen::Vector3d calculatePhongLighting(
        const Eigen::Vector3d& point,
        const Eigen::Vector3d& normal,
        const Eigen::Vector3d& viewDir,
        const Eigen::Vector3d& baseColor) const {
        
        Eigen::Vector3d totalLight(0, 0, 0);
        
        for (const auto& light : lights) {
            // Calculate light direction
            Eigen::Vector3d lightDir = (light.position - point).normalized();
            
            // Calculate distance attenuation - reduced falloff
            double distance = (light.position - point).norm();
            double attenuation = 1.0 / (1.0 + 0.05 * distance + 0.001 * distance * distance);
            
            // Calculate light contribution
            Eigen::Vector3d lightContribution(0, 0, 0);
            
            // Ambient component
            lightContribution += ambientCoefficient * baseColor;
            
            // Diffuse component
            double diffuseFactor = std::max(0.0, normal.dot(lightDir));
            lightContribution += diffuseCoefficient * diffuseFactor * baseColor;
            
            // Specular component
            Eigen::Vector3d reflectDir = reflect(-lightDir, normal);
            double specularFactor = std::pow(std::max(0.0, reflectDir.dot(viewDir)), shininess);
            lightContribution += specularCoefficient * specularFactor * light.color;
            
            // Add this light's contribution with attenuation
            totalLight += lightContribution * attenuation * light.intensity;
        }
        
        // Clamp the result to [0,1]
        return totalLight.cwiseMax(0.0).cwiseMin(1.0);
    }
};

#endif // LIGHTING_H 
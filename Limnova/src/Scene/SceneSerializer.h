#pragma once

#include <yaml-cpp/yaml.h>

#include "Scene.h"

#include <Orbital/OrbitalScene.h>


namespace Limnova
{

    class SceneSerializer
    {
    public:
        SceneSerializer() = delete;

        static void Serialize(Scene* scene, const std::string& filepath);
        static void SerializeRuntime(Scene* scene, const std::string& filepath);
        static bool Deserialize(Scene* scene, const std::string& filepath);
        static bool DeserializeRuntime(Scene* scene, const std::string& filepath);

        static void Serialize(OrbitalScene* scene, const std::string& filepath);
        static void SerializeRuntime(OrbitalScene* scene, const std::string& filepath);
        static bool Deserialize(OrbitalScene* scene, const std::string& filepath);
        static bool DeserializeRuntime(OrbitalScene* scene, const std::string& filepath);
    private:
        static void SerializeEntity(Scene* scene, YAML::Emitter& out, Entity entity);
        static void DeserializeEntity(Scene* scene, YAML::Node entityNode);
    };

}

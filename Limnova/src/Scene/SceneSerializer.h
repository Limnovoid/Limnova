#pragma once

#include <yaml-cpp/yaml.h>

#include "Scene.h"

#include <Orbital/OrbitalScene.h>


namespace Limnova
{

    class SceneSerializer
    {
    public:
        SceneSerializer(const Ref<Scene>& scene);

        virtual void Serialize(const std::string& filepath);
        virtual void SerializeRuntime(const std::string& filepath);

        virtual bool Deserialize(const std::string& filepath);
        virtual bool DeserializeRuntime(const std::string& filepath);
    protected:
        void SerializeEntity(YAML::Emitter& out, Entity entity);
        void DeserializeEntity(YAML::Node entityNode);
    private:
        Ref<Scene> m_Scene;
    };


    class OrbitalSceneSerializer : public SceneSerializer
    {
    public:
        OrbitalSceneSerializer(const Ref<OrbitalScene>& scene);

        void Serialize(const std::string& filepath) override;
        void SerializeRuntime(const std::string& filepath) override;

        bool Deserialize(const std::string& filepath) override;
        bool DeserializeRuntime(const std::string& filepath) override;
    private:
        Ref<OrbitalScene> m_Scene;
    };
}

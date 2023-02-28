#pragma once

#include "Limnova.h"


struct Transform
{
    Limnova::Vector3 Position;
};


class Entity
{
public:
    Entity() : m_Id(ReserveId()) {}
    Entity(const std::string& name) : m_Id(ReserveId()), m_Name(name) {}
    ~Entity() {}

public:
    virtual void OnUpdate(Limnova::Timestep dT) = 0;

    virtual void Destroy() { m_ReusableIds.insert(m_Id); }
public:
    void SetName(const std::string& name) { m_Name = name; }
    void SetPosition(const Limnova::Vector3& position) { m_Transform.Position = position; }

    const uint32_t GetId() { return m_Id; }
    const std::string& GetName() { return m_Name; }
    const Limnova::Vector3& GetPosition() { return m_Transform.Position; }
private:
    uint32_t m_Id;
    std::string m_Name;

    Transform m_Transform;
private:
    static uint32_t m_IdTop;
    static std::unordered_set<uint32_t> m_ReusableIds;
private:
    static uint32_t ReserveId();
    static void ReleaseId(const uint32_t id);
};


using EntityRef = std::shared_ptr<Entity>;

#pragma once

#include "Math/Math.h"
#include "Util/Util.h"

#include <limits>

namespace Limnova
{

class OrbitalPhysics
{
public:
	using TNodeId				= NTree::TNodeId;	/// The NTree node ID type.
	using Node					= NTree::Node;		/// The NTree node type.
	using TId					= uint32_t;			/// The Storage class ID type.

	static constexpr TNodeId	NNull						= NTree::NNull;
	static constexpr TId		IdNull						= ~TId(0);

	OrbitalPhysics() = delete;

	/* Basis of the reference frame: the XY-plane represents the orbital plane of the system which has the root object as its primary */
	static constexpr Vector3	kReferenceX					= { 1.f, 0.f, 0.f };
	static constexpr Vector3	kReferenceY					= { 0.f, 0.f,-1.f };
	static constexpr Vector3	kReferenceNormal			= { 0.f, 1.f, 0.f };
	static constexpr Vector3	kReferenceXd				= Vector3d(kReferenceX);
	static constexpr Vector3	kReferenceYd				= Vector3d(kReferenceY);
	static constexpr Vector3	kReferenceNormald			= Vector3d(kReferenceNormal);

	static constexpr double		kGravitational				= 6.6743e-11;

	// Simulation tuning parameters
	// TODO : choose numbers based on reasoning/testing
	static constexpr float		kDefaultLSpaceRadius		= 0.1f;
	static constexpr float		kLocalSpaceEscapeRadius		= 1.01f;

	static constexpr float		kEccentricityEpsilon		= 1e-4f;

	static constexpr float		kMaxLSpaceRadius			= 0.2f;
	static constexpr float		kMinLSpaceRadius			= 0.004f;
	static constexpr float		kEpsLSpaceRadius			= 1e-6f;

	static constexpr float		kMaxObjectUpdates			= 20.f; /* highest number of updates each object is allowed before the total number of updates (across all objects) in a single frame becomes too high (resulting in unacceptable FPS drops) */
	static constexpr double		kDefaultMinDT				= 1.0 / (60.0 * static_cast<double>(kMaxObjectUpdates)); /* above constraint expressed as a maximum delta time for a single integration step */
	static constexpr float		kMaxPositionStep			= 1e-6f; /* largest delta position we can allow before integration step becomes too visible (for when object DT is much greater than frame DT) */
	static constexpr double		kMaxPositionStepd			= static_cast<double>(kMaxPositionStep);
	static constexpr double		kMaxVelocityStep			= kMaxPositionStepd / 10.0;
	static constexpr double		kMinUpdateTrueAnomaly		= std::numeric_limits<double>::epsilon() * 1e3; /* smallest delta true anomaly we can allow before precision error becomes unacceptable for long-term angular integration */

private:
	class LSpaceNode;

	struct Object;
	struct State;
	struct Motion;
	struct LocalSpace;
	struct OrbitSection;
	struct Elements;
	struct Dynamics;
	struct Integration;

public:
	class ObjectNode
	{
		friend class OrbitalPhysics;

	public:
		constexpr ObjectNode();

		ObjectNode(ObjectNode const& rhs);

		ObjectNode(TNodeId const nodeId);

		/// <summary> Get the node ID. </summary>
		TNodeId Id() const;

		/// <summary> Get a copy of the Null object node. </summary>
		/// <returns></returns>
		static constexpr ObjectNode NNull();

		bool IsNull() const;
		bool IsRoot() const;
		bool IsDynamic() const;

		/// <summary> Does the object have a sphere of influence. </summary>
		bool IsInfluencing() const;

		/// <summary> Does the object have at least one local space attached to it. </summary>
		bool HasChildLSpace() const;

		OrbitalPhysics::Object const& GetObj() const;
		OrbitalPhysics::State const& GetState() const;
		OrbitalPhysics::Motion const& GetMotion() const;
		OrbitalPhysics::Dynamics const& GetDynamics() const;

		/// <summary> Computes or updates the Orbit and returns its first section. </summary>
		OrbitalPhysics::OrbitSection const& GetOrbit(size_t const maxSections = 1) const;

		LSpaceNode ParentLsp() const;
		ObjectNode ParentObj() const;

		LSpaceNode PrimaryLsp() const;
		ObjectNode PrimaryObj() const;

		LSpaceNode FirstChildLSpace() const;
		LSpaceNode SphereOfInfluence() const;

		/// <summary>
		/// Get the position of the object relative to its primary object.
		/// The primary object is the dominant source of gravity for the object - i.e, the parent of the smallest influencing local space which contains the object.
		/// The object's parent object (the parent of the object's parent local space) is not the primary if the parent has no sphere of influence, or if the object is in a local space outside the radius of the sphere of influence.
		/// The returned position is relative to the primary object, with a length unit parameterised by the parent local space radius.
		/// This is useful, e.g, for computing the object's orbit when it is occupying a non-influencing local space - the orbit will be computed with respect to the primary local space but conveniently scaled to the parent local space.
		/// </summary>
		/// <returns>Position of the object relative to its primary, scaled to the parent local space.</returns>
		Vector3 LocalPositionFromPrimary() const;

		/// <summary>
		/// Get the velocity of the object relative to its primary object.
		/// The primary object is the dominant source of gravity for the object - i.e, the parent of the smallest influencing local space which contains the object.
		/// The object's parent object (the parent of the object's parent local space) is not the primary if it is has no sphere of influence, or if the object is in a local space outside the radius of the sphere of influence.
		/// The returned velocity is relative to the primary object, with a length unit parameterised by the parent local space radius.
		/// This is useful, e.g, for computing the object's orbit when it is occupying a non-influencing local space - the orbit will be computed with respect to the primary local space but conveniently scaled to the parent local space.
		/// </summary>
		/// <returns>Velocity of the object relative to its primary, scaled to the parent local space.</returns>
		Vector3d LocalVelocityFromPrimary() const;

		/// <summary>
		/// Get the local spaces attached to this object node as an ordered list.
		/// The local spaces are added to the back of the given vector, largest first, smallest last.
		/// </summary>
		/// <param name="lspNodes">Storage for the list of local spaces.</param>
		/// <returns>The number of local spaces added to the vector.</returns>
		size_t GetLocalSpaces(std::vector<LSpaceNode>& lspNodes) const;

		/// <summary> Set the object's parent local space. </summary>
		void SetLocalSpace(LSpaceNode const newLspNode) const;

		/// <summary> Set the object's mass. </summary>
		void SetMass(double const mass) const;

		/// <summary> Set the object's position relative to its parent local space (its magnitude parameterised by the local space radius). </summary>
		void SetPosition(Vector3 const& position) const;

		/// <summary> Set the object's velocity relative to its parent local space (its magnitude parameterised by the local space radius). </summary>
		void SetVelocity(Vector3d const& velocity) const;

		/// <summary>
		/// Returns velocity for a circular counter-clockwise orbit around the object's current primary, given its current mass and position.
		/// </summary>
		/// <param name="object">Physics object ID</param>
		Vector3d CircularOrbitVelocity() const;

		/// <summary> Enable or disable dynamic integration. </summary>
		/// <param name="isDynamic">Set true to enable, false to disable.</param>
		void SetDynamic(bool const isDynamic) const;

		/// <summary>
		/// Set the continuous dynamic acceleration of the object.
		/// The acceleration is applied to the object's motion as though it is constant, like, e.g, acceleration due to engine thrust.
		/// This is in addition to its acceleration due to gravity, i.e, calls to this function do not affect the simulation of gravity.
		/// </summary>
		/// <param name="acceleration">Magnitude is absolute (not scaled to the local space)</param>
		void SetContinuousAcceleration(Vector3d const& acceleration) const;

		/// <summary>
		/// Set the continuous dynamic thrust of the object.
		/// The thrust is applied to the object's motion as though it is constant, like, e.g, engine thrust.
		/// This is in addition to its acceleration due to gravity, i.e, calls to this function do not affect the simulation of gravity.
		/// </summary>
		/// <param name="thrust">Magnitude is absolute (not scaled to the local space)</param>
		void SetContinuousThrust(Vector3d const& thrust) const;

		/// <summary> Add a local space to this object. </summary>
		/// <param name="radius">Radius of the local space, measured in the object's parent local space.</param>
		/// <returns>The new local space node.</returns>
		LSpaceNode AddLocalSpace(float const radius = kDefaultLSpaceRadius);

		bool operator==(ObjectNode const& rhs) const;
		bool operator!=(ObjectNode const& rhs) const;

	private:
		Node const& Node() const;
		int Height() const;
		Object& Object() const;
		State& State() const;
		Motion& Motion() const;
		Dynamics& Dynamics() const;

		OrbitSection& Orbit() const;

		operator TNodeId() const;

		TNodeId				m_NodeId;			/// The tree node ID.
	};

	class LSpaceNode
	{
		friend class OrbitalPhysics;

	public:
		constexpr LSpaceNode();
		LSpaceNode(LSpaceNode const& rhs);
		LSpaceNode(TNodeId const nodeId);

		TNodeId Id() const;

		static constexpr LSpaceNode NNull();

		bool IsNull() const;
		bool IsRoot() const;

		bool IsLargestLSpaceOnObject() const;
		bool IsSmallestLSpaceOnObject() const;

		/// <summary> Is this local space influencing, i.e, is the parent object the dominant source of gravity for objects in this local space. </summary>
		bool IsInfluencing() const;

		/// <summary> Is this local space the parent object's sphere of influence, i.e, is it the largest influencing local space attached to this object. </summary>
		bool IsSphereOfInfluence() const;

		LocalSpace const& GetLSpace() const;

		ObjectNode ParentObj() const;
		LSpaceNode ParentLsp() const;

		LSpaceNode PrimaryLsp() const;
		ObjectNode PrimaryObj() const;

		/// <summary>
		/// Get the objects in this local space as an ordered list.
		/// The objects are added to the back of the given vector, in order of their appearance in their sibling list.
		/// </summary>
		/// <param name="objNodes">Storage for the list of objects nodes.</param>
		/// <returns>The number of object nodes added to the vector.</returns>
		size_t GetLocalObjects(std::vector<ObjectNode>& objNodes) const;

		/// <summary>
		/// Get the next-largest local space containing this local space.
		/// This is either the next-largest local space attached to the parent object, or the parent local space of the parent object.
		/// </summary>
		LSpaceNode OuterLSpace() const;

		/// <summary> Get the next-smallest local space attached to this local space's parent object. </summary>
		LSpaceNode InnerLSpace() const;

		/// <summary> Get the radius of the inner local space relative to this local space, or zero if no inner local space exists. </summary>
		float InnerLSpaceLocalRadius() const;

		/// <summary>
		/// Get the position of the local space relative to its primary local space.
		/// The primary local space is the smallest influencing local space containing this local space.
		/// The returned position is relative to the primary local space, with a length unit parameterised by the radius of this local space.
		/// </summary>
		/// <returns>Position of the local space relative to its primary, scaled to this local space's radius.</returns>
		Vector3 LocalOffsetFromPrimary() const;

		/// <summary>
		/// Get the velocity of the local space relative to its primary local space.
		/// The primary local space is the smallest influencing local space containing this local space.
		/// The returned velocity is relative to the primary local space, with a length unit parameterised by the radius of this local space.
		/// </summary>
		/// <returns>Velocity of the local space relative to its primary, scaled to this local space's radius.</returns>
		Vector3d LocalVelocityFromPrimary() const;

		/// <summary> Set the radius of the local space. </summary>
		/// <param name="radius">The radius of the local space, measured in the parent local space of the parent object.</param>
		void SetRadius(float const radius) const;

		/// <summary> Attempts to set the radius of the local space. </summary>
		/// <returns>True if radius was successfully set, otherwise false.</returns>
		bool TrySetRadius(float const radius) const;

		bool operator==(LSpaceNode const& rhs) const;
		bool operator!=(LSpaceNode const& rhs) const;

	private:
		Node const& Node() const;
		int Height() const;
		LocalSpace& LSpace() const;

		/// <summary>
		/// Recursive implementation of function LocalOffsetFromPrimary.
		/// </summary>
		/// <param name="lspId">ID of the local space node for which to compute the local offset.</param>
		/// <param name="primaryLspId">The ID of the node's primary local space node.</param>
		/// <returns>The locally-scaled position vector of the local space relative to its primary.</returns>
		Vector3 LocalOffsetFromPrimaryRecurs(TNodeId const lspId, TNodeId const primaryLspId) const;

		Vector3d LocalVelocityFromPrimaryRecurs(TNodeId const lspId, TNodeId const primaryLspId) const;

		/// <summary>
		/// Implementation of SetRadius/TrySetRadius.
		/// Does not prevent setting the radius of a sphere of influence.
		/// </summary>
		void SetRadiusImpl(float const radius) const;

		operator TNodeId() const;

		TNodeId		m_NodeId;	/// The NTree node ID.
	};

	/// <summary>
	/// Validity indicator enum.
	/// </summary>
	enum class Validity : uint8_t
	{
		InvalidParent				= 0,		/// The parent's validity is not Valid.
		InvalidSpace,							/// The local space cannot support the object.
		InvalidMass,							/// Object mass is invalid (e.g, too high in relation to the parent's mass).
		InvalidPosition,						/// Object position is invalid (e.g, outside the local space or overlapping a co-local space).
		InvalidMotion,							/// Object's computed orbit path is invalid (e.g, exits the local space but object is not dynamic).
		Valid						= 100		/// Object state is valid.
	};

	enum class OrbitType
	{
		Circle = 0,
		Ellipse = 1,
		Hyperbola = 2
	};

	struct Object
	{
		Validity Validity			= Validity::InvalidParent;
		LSpaceNode Influence		= {};					/// Local space node representing this object's sphere of influence: Null if object is not influencing
	};

	struct State
	{
		double Mass					= 0.0;
		Vector3 Position			= { 0.f };
		Vector3d Velocity			= { 0.0 };
		Vector3d Acceleration		= { 0.0 };
	};

	struct Motion
	{
		enum class Integration
		{
			Angular = 0,
			Linear,
			Dynamic
		};

		Integration Integration		= Integration::Angular;
		bool ForceLinear			= false;
		double TrueAnomaly			= 0.f;

	private:
		friend class OrbitalPhysics;

		double PrevDT				= 0.0;
		double UpdateTimer			= 0.0;
		double DeltaTrueAnomaly		= 0.f;
		ObjectNode UpdateNext		= {};

		TId Orbit					= IdNull;
	};

	struct Dynamics
	{
		Vector3d ContAcceleration	= { 0.0 };				/// Acceleration assumed to be constant between timesteps
		Vector3d DeltaPosition		= { 0.0 };
	};

	struct LocalSpace
	{
		float Radius				= 0.f;					/// Measured in parent's influence
		double MetersPerRadius		= 0.f;
		double Grav					= 0.f;					/// Gravitational parameter (mu)

		LSpaceNode Primary			= {};
	};

	class Elements
	{
	public:
		double H					= 0.0;					/// Orbital specific angular momentum
		float E						= 0.f;					/// Eccentricity
		double VConstant			= 0.f;					/// Constant factor of orbital velocity:             mu / h
		double MConstant			= 0.f;					/// Constant factor of mean anomaly for e >= 1:      mu^2 / h^3

		OrbitType Type				= OrbitType::Circle;	/// Type of orbit - defined by eccentricity, indicates the type of shape which describes the orbit path

		/* Dimensions */
		float SemiMajor				= 0.f;
		float SemiMinor				= 0.f;					/// Semi-major and semi-minor axes
		float C						= 0.f;					/// Signed distance from occupied focus to centre, measured along perifocal frame's x-axis
		double T					= 0.0;					/// Orbit period, measured in seconds
		float P						= 0.f;					/// Orbit parameter, or semi-latus rectum:   h^2 / mu

		/* Orientation */
		float I						= 0.f;					/// Inclination
		Vector3 N					= { 0.f };				/// Direction of ascending node
		float Omega					= 0.f;					/// Right ascension of ascending node
		float ArgPeriapsis			= 0.f;					/// Argument of periapsis

		/* Perifocal frame */
		Vector3 PerifocalX			= { 0.f };
		Vector3 PerifocalY			= { 0.f };
		Vector3 PerifocalNormal		= { 0.f };
		Quaternion PerifocalOrientation;					/// Orientation of the perifocal frame relative to the reference frame

	public:
		float RadiusAt(float const trueAnomaly) const;

		Vector3 PositionAt(float const trueAnomaly) const;

		Vector3d VelocityAt(float const trueAnomaly) const;

		float TrueAnomalyOf(Vector3 const& positionDirection) const;

		/// <summary> Compute the time since periapsis of a given true anomaly. </summary>
		float ComputeTimeSincePeriapsis(float const trueAnomaly) const;

		/// <summary> Solve for true anomaly given the time since last periapse passage. </summary>
		float SolveTrueAnomaly(float const timeSincePeriapsis, float const tolerance = 0.001f, size_t const nMaxIterations = 100) const;

		/// <summary> Solve for a final true anomaly given an initial true anomaly and a time separation between them. </summary>
		/// <param name="timeSeparation">Time in seconds between the initial and final true anomalies</param>
		float SolveFinalTrueAnomaly(float const initialTrueAnomaly, float const timeSeparation) const;
	};

	class OrbitSection
	{
	public:
		Vector3 LocalPositionAt(float trueAnomaly) const;

		LSpaceNode LocalSpace;								/// The local space through which this orbit section describes its object's motion
		Elements Elements;									/// Orbital motion description (shape, duration, etc)
		float TaEntry				= 0.f;					/// True anomaly of orbit's point of entry into the local space (if the section escapes the local space, otherwise has value 0)
		float TaExit				= PI2f;					/// True anomaly of orbit's point of escape from the local space (if the section escapes the local space, otherwise has value 2Pi)
		TId Next					= NNull;				/// Reference to next orbit section which will describe the object's motion after escaping this, or entering a new, local space (or NNull if neither of these events occur)
	};

	class Context
	{
		friend class OrbitalPhysics;

	public:
		Context();
		Context(Context const& other) = default;

		~Context();

		std::function<void(ObjectNode)>		m_ParentLSpaceChangedCallback;
		std::function<void(ObjectNode)>		m_ChildLSpacesChangedCallback;

	private:
		NTree m_Tree;
		Storage<OrbitSection>				m_OrbitSections;

		MappedStorage<TNodeId, Object>		m_Objects;
		MappedStorage<TNodeId, State>		m_States;
		MappedStorage<TNodeId, Motion>		m_Motions;
		MappedStorage<TNodeId, Dynamics>	m_Dynamics;
		MappedStorage<TNodeId, LocalSpace>	m_LSpaces;

		ObjectNode							m_UpdateQueueFront;
	};

	static void SetContext(Context* ctx);

public:
	/// <summary> Perform a simulation update step. </summary>
	/// <param name="dT">Frame delta time.</param>
	static void OnUpdate(double dT);

	static ObjectNode GetRootObjectNode();
	static LSpaceNode GetRootLSpaceNode();

	/// <summary>
	/// Sets scaling of the root local space.
	/// Scaling is measured in meters per unit-radii of the root local space.
	/// E.g, a vector with length 1 in the root orbital space represents a simulated length equal to the value of parameter 'meters'.
	/// </summary>
	/// <param name="meters">A positive (non-zero) number.</param>
	static void SetRootSpaceScaling(double meters);

	/// <returns>True if ID belongs to a physics object in the simulation, false otherwise.</returns>
	static bool Has(TNodeId nodeId);

	/// <summary> Create an orbital physics object in the specified local space. </summary>
	/// <param name="lspNode">The local space to contain the new object.</param>
	/// <param name="mass">The mass of the object.</param>
	/// <param name="position">Initial position of the object in the local space.</param>
	/// <param name="position">Initial velocity of the object in the local space.</param>
	/// <param name="isDynamic">Whether the object should support dynamic integration (allows dynamic acceleration and moving between local spaces).</param>
	static ObjectNode Create(LSpaceNode lspNode, double mass, Vector3 const& position, Vector3d const& velocity, bool isDynamic = false);

	/// <summary>
	/// Create an orbital physics object in the specified local space.
	/// The created object is initialized with a circular orbit starting from the given position.
	/// </summary>
	/// <param name="lspNode">The local space to contain the new object.</param>
	/// <param name="mass">The mass of the object.</param>
	/// <param name="position">Initial position of the object in the local space.</param>
	/// <param name="isDynamic">Whether the object should support dynamic integration (allows dynamic acceleration and moving between local spaces).</param>
	static ObjectNode Create(LSpaceNode lspNode, double mass, Vector3 const& position, bool isDynamic = false);

	/// <summary>
	/// Create an orbital physics object in the specified local space.
	/// The created object is uninitialized - zero position and velocity in the local space, not attached to the simulation.
	/// </summary>
	/// <param name="lspNode">The local space to contain the new object.</param>
	/// <param name="isDynamic">Whether the object should support dynamic integration (allows dynamic acceleration and moving between local spaces).</param>
	static ObjectNode Create(LSpaceNode lspNode, bool isDynamic = false);

	/// <summary>
	/// Create an orbital physics object in the root local space.
	/// The created object is uninitialized - zero position and velocity in the root space, not attached to the simulation.
	/// </summary>
	/// <param name="isDynamic">Whether the object should support dynamic integration (allows dynamic acceleration and moving between local spaces).</param>
	static ObjectNode Create(bool isDynamic = false);

	/// <summary>
	/// Destroy an orbital physics object.
	/// Children are re-parented to the destroyed object's parent and preserve their absolute states.
	/// </summary>
	static void Destroy(ObjectNode objNode);

	/// <summary>
	/// Removes a local space.
	/// Objects in the local space are moved to the next higher local space and preserve their absolute states.
	/// </summary>
	static void CollapseLocalSpace(LSpaceNode lspNode);

	static std::string ValidityToString(Validity v);

	/// <summary>
	/// Compute the tangential speed needed for an object to have a circular orbit around the local primary at the given orbit radius (measured in local space radii).
	/// Uses the primary of the given local space, not necessarily the local space itself: the orbit will not be circular in a non-influencing local space.
	/// </summary>
	static double CircularOrbitSpeed(LSpaceNode lspNode, float localRadius);

	/// <summary>
	/// Compute the velocity for a circular counter-clockwise orbit in the given local space at the given initial position.
	/// Uses the primary of the given local space, not necessarily the local space itself: the orbit will not be circular in a non-influencing local space.
	/// </summary>
	static Vector3d CircularOrbitVelocity(LSpaceNode lspNode, Vector3 const& localPosition);

	/// <summary> Compute the vector from one object to another, parameterised by the first object's local space radius. </summary>
	static Vector3 ComputeLocalSeparation(ObjectNode fromObject, ObjectNode toObject);

	/// <summary> Compute a position vector relative to a given local space, given its position in another local space. </summary>
	static Vector3 ComputeLocalPosition(LSpaceNode fromLsp, LSpaceNode toLsp, Vector3 toPosition);

	/// <summary> Compute the velocity of an object relative to a given local space, parameterised by that local space's radius. </summary>
	static Vector3d ComputeLocalVelocity(ObjectNode object, LSpaceNode lsp);

	/// <summary> Compute the velocity of an object relative to a given local space, parameterised by that local space's radius. </summary>
	static Vector3d ComputeLocalVelocity(Vector3d const objVelocity, LSpaceNode objLsp, LSpaceNode lsp);

	/// <summary>
	/// Solve for the position at which the missile object intercepts the target object, assuming constant thrust.
	/// All arguments in local units.
	/// </summary>
	/// <returns> Position vector of intercept relative to missile's local space. </returns>
	static void SolveMissileIntercept(ObjectNode missileObject, ObjectNode targetObject, double const acceleration,
		float const targetingTolerance, Vector3 & localIntercept, float & timeToIntercept, size_t const maxIterations = 5);

	/// <summary> Compute a missile's steering acceleration according to the proportional navigation equation (https://en.wikipedia.org/wiki/Proportional_navigation). </summary>
	/// <param name="targetRelativePosition">Position of target relative to missile.</param>
	/// <param name="targetRelativeVelocity">Velocity of target relative to missile.</param>
	/// <param name="missileVelocityDirection">Missile's current velocity direction (a unit vector).</param>
	/// <returns>The steering acceleration to apply to the missile in order to intercept the target.</returns>
	static Vector3d ComputeProportionalNavigationAcceleration(Vector3d const& targetRelativePosition, Vector3d const& targetRelativeVelocity,
		Vector3d const& missileVelocityDirection, double const proportionalityConstant = 4.0);

	/// <summary> Compute a missile's steering acceleration according to the proportional navigation equation (https://en.wikipedia.org/wiki/Proportional_navigation). </summary>
	/// <returns>The steering acceleration to apply to the missile in order to intercept the target.</returns>
	static Vector3d ComputeProportionalNavigationAcceleration(ObjectNode missileObject, ObjectNode targetObject, double const proportionalityConstant = 4.0);

	/// <summary>
	/// Solve for the vector that a missile object should accelerate along in order to intercept a target object, given a constant magnitude of engine thrust.
	/// Assumes the target object will not change its current orbit.
	/// NOTE: the solved value is a unit direction vector, not the actual thrust vector the missile object should apply.
	/// </summary>
	/// <param name="missileObject">The missile object.</param>
	/// <param name="targetObject">The target object.</param>
	/// <param name="localThrust">Magnitude of missile's constant engine thrust in localized units.</param>
	/// <param name="targetingTolerance">Tolerance used when solving for the point of intercept.</param>
	/// <param name="interceptVector">Storage for the solved intercept vector.</param>
	/// <param name="interceptPosition">Storage for the location of the solved intercept in the missile's local space.</param>
	/// <param name="timeToIntercept">Storage for the approximate time of flight to the point of intercept.</param>
	/// <param name="proportionalityConstant">Parameter for computing the missile's proportional navigation.</param>
	/// <param name="maxIterations">Maximum number of iterations to use when solving.</param>
	static void SolveMissileInterceptVector(ObjectNode missileObject, ObjectNode targetObject, double localAcceleration, float targetingTolerance,
			Vector3 &interceptVector, Vector3 &interceptPosition, float &timeToIntercept, float proportionalityConstant = 4.f, size_t maxIterations = 5);

private:
	/// <summary> Does the ID belong to a local space node. </summary>
	static bool IsLocalSpace(TNodeId const nodeId);

	/// <summary> Create a new object node in the given local space. </summary>
	static ObjectNode NewObjectNode(LSpaceNode parentNode);

	/// <summary> Remove an object node from the simulation. </summary>
	static void RemoveObjectNode(ObjectNode objNode);

	/// <summary> Resize all local spaces attached to an object node by a given factor. </summary>
	static void RescaleLocalSpaces(ObjectNode objNode, float const rescalingFactor);

	/// <summary> Moves object from its current local space to the next-larger local space, recomputing relative position to preserve absolute position. </summary>
	static void PromoteObjectNode(ObjectNode objNode);

	/// <summary> Moves object to a lower local space which is attached to another object in the same current local space. </summary>
	static void DemoteObjectNode(LSpaceNode newLspNode, ObjectNode objNode);

	/// <summary> Moves object to the next-lower local space attached to the same object as the current local space. </summary>
	static void DemoteObjectNode(ObjectNode objNode);

	static LSpaceNode NewLSpaceNode(ObjectNode parentNode, float const radius = kDefaultLSpaceRadius);
	static LSpaceNode NewSoiNode(ObjectNode parentNode, float const radiusOfInfluence);
	static void RemoveLSpaceNode(LSpaceNode lspNode);

	/// <summary> Get a new orbit section from storage and initialize it with the given local space node. </summary>
	/// <param name="lspNode">The local space which contains the orbital section.</param>
	/// <returns>The ID of the orbit section in storage.</returns>
	static TId NewOrbit(LSpaceNode lspNode);

	/// <summary> Delete an orbit (a linked list of orbit sections) starting with the given section. </summary>
	static void DeleteOrbit(TId& sectionId);

	/// <summary>
	/// Compute an orbit (a linked list of orbit sections) from the given initial position and velocity.
	/// The orbital path starts with the given section (initialized separately with the local space in which the orbital path begins).
	/// The path is limited to the given maximum number of orbit sections.
	/// A path can have multiple sections if the orbit path causes the orbiter (object) to exit a given local space either by exceeding the local space radius or overlapping an inner local space.
	/// </summary>
	/// <param name="firstSectionId">ID of the orbit section in which the orbit path begins.</param>
	/// <param name="localPosition">Initial position of the orbiting object relative to the local space.</param>
	/// <param name="localVelocity">Initial velocity of the orbiting object relative to the local space.</param>
	/// <param name="maxSections">Maximimum number of orbit sections to compute, should the orbit exit the initial local space.</param>
	static void ComputeOrbit(TId firstSectionId, Vector3 const& localPosition, Vector3d const& localVelocity, size_t maxSections = 1);

	/// <summary> Compute the true anomalies of an orbit section's local entry and escape points. </summary>
	static void ComputeTaLimits(OrbitSection& section);

	/// <summary> Compute an orbit section's elements. </summary>
	static void ComputeElements(OrbitSection& section, Vector3 const& localPosition, Vector3d const& localVelocity);

		static void CallParentLSpaceChangedCallback(ObjectNode objNode);
	static void CallChildLSpacesChangedCallback(ObjectNode objNode);

	/// <summary>
	/// Compute the radius of influence of an object node.
	/// If the computed radius is above the minimum supported local space radius, set the object's sphere of influence (its largest influencing local space)
	/// to the new radius, or add a sphere of influence if the object does not have one.
	/// </summary>
	static void ComputeInfluence(ObjectNode objNode);

	/// <summary> Compute the timestep to add to an object's update timer. </summary>
	static double ComputeObjDT(double const velocityMagnitude, double const minDT = kDefaultMinDT);

	/// <summary> Compute the timestep to add to a dynamic object's update timer. </summary>
	/// <param name="accelerationMagnitude">The magnitude of the object's current dynamic acceleration.</param>
	static double ComputeDynamicObjDT(double const velocityMagnitude, double const accelerationMagnitude, double const minDT = kDefaultMinDT);

	/// <summary> Compute gravitational parameter (GM/r in standard units) scaled to a local space with the given length unit. </summary>
	static double LocalGravitationalParameter(double const localPrimaryMass, double const localMetersPerUnitLength);

	/// <summary> Add the object node to the front of the update queue. </summary>
	static void UpdateQueuePushFront(ObjectNode objNode);

	/// <summary>
	/// Remove the given object from the update queue.
	/// Attempting to remove an object which is not in the queue will result in an array out-of-bounds error caused by executing 'm_Objects[Null]'.
	/// See UpdateQueueSafeRemove().
	/// </summary>
	static void UpdateQueueRemove(ObjectNode objNode);

	/// <summary>
	/// If the object is in the update queue, remove it.
	/// </summary>
	/// <returns>True if object was found and removed, otherwise false.</returns>
	static bool UpdateQueueSafeRemove(ObjectNode objNode);

	/// <summary>
	/// Sorts the first entry in the queue to its correct position.
	/// Assumes the first entry is the only entry which may be unsorted.
	/// </summary>
	static void UpdateQueueSortFront();

	/// <summary> Check that the object state (mass, position, velocity) is valid for simulation and, if so, compute its simulation data. </summary>
	/// <returns>The object's validity.</returns>
	static Validity TryPrepareObject(ObjectNode objNode);

	/// <returns>True if the object's orbit path is valid, otherwise false.</returns>
	static bool ValidMotion(ObjectNode objNode);

	/// <returns>True if the object's local position is valid, otherwise false.</returns>
	static bool ValidPosition(ObjectNode objNode);

	/// <returns>True if object's mass is valid, otherwise false.</returns>
	static bool ValidMass(ObjectNode objNode);

	/// <returns>True if object's local space is valid, otherwise false.</returns>
	static bool ValidSpace(ObjectNode objNode);

	/// <returns>True if object's parent is valid, otherwise false.</returns>
	static bool ValidParent(ObjectNode objNode);

	/// <summary> Run TryPrepareObject() on every ObjectNode in the subtree rooted in the given node, excluding the given node itself. </summary>
	static void TryPrepareSubtree(TNodeId rootNodeId);

	/// <summary> Approximate of object's delta true anomaly across the next integration step using the horizontal velocity component and delta time. </summary>
	/// <param name="posFromPrimary">Object's local position relative to the primary.</param>
	/// <param name="distFromPrimary">Object's local distance from the primary.</param>
	/// <param name="velFromPrimary">Object's local velocity relative to the primary.</param>
	/// <param name="objDT">Object's delta time - the integration timestep.</param>
	/// <returns>Approximation of object's delta true anomaly across the next integration step.</returns>
	static double ApproximateDeltaTrueAnomaly(Vector3d const& posFromPrimary, double distFromPrimary, Vector3d const& velFromPrimary, double objDT);

	/// <summary> Select the appropriate integration method for the object's next update. </summary>
	static enum class Motion::Integration SelectIntegrationMethod(double deltaTrueAnomaly, bool isDynamicallyAccelerating = false);

	/// <summary> Compute the object's integration parameters required for simulation. </summary>
	static void ComputeMotion(ObjectNode objNode);

	static Context*					m_pCtx;

	static const TNodeId			kRootObjId;
	static const TNodeId			kRootLspId;
};

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::IsLocalSpace(TNodeId const nodeId)
{
	return m_pCtx->m_Tree.Height(nodeId) % 2; /* 0 -> object, 1 -> local space */
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::TId OrbitalPhysics::NewOrbit(LSpaceNode lspNode)
{
	TId newFirstSectionId = m_pCtx->m_OrbitSections.New();

	m_pCtx->m_OrbitSections.Get(newFirstSectionId).LocalSpace = lspNode;

	return newFirstSectionId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline void OrbitalPhysics::SetContext(Context* pCtx)
{
	m_pCtx = pCtx;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline void OrbitalPhysics::CallParentLSpaceChangedCallback(ObjectNode objNode)
{
	if (m_pCtx->m_ParentLSpaceChangedCallback)
		m_pCtx->m_ParentLSpaceChangedCallback(objNode);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline void OrbitalPhysics::CallChildLSpacesChangedCallback(ObjectNode objNode)
{
	if (m_pCtx->m_ChildLSpacesChangedCallback)
		m_pCtx->m_ChildLSpacesChangedCallback(objNode);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline double OrbitalPhysics::ComputeObjDT(double const velocityMagnitude, double const minDT)
{
	if (0.0 < velocityMagnitude)
	{
		// If velocity (V) is greater than max update distance (MUD),
		// computed DT is less than 1; if DT is too small (resulting in too many updates per frame),
		// we take minDT instead and sacrifice accuracy to maintain framerate.
		// Otherwise, if V < MUD, then DT > 1 and is safe to use.
		return std::max(kMaxPositionStepd / velocityMagnitude, minDT);
	}

	return minDT;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline double OrbitalPhysics::ComputeDynamicObjDT(double const velocityMagnitude, double const accelerationMagnitude, double const minDT)
{
	if (0.0 < accelerationMagnitude)
		return std::min(ComputeObjDT(velocityMagnitude), std::max(kMaxVelocityStep / accelerationMagnitude, minDT));

	return ComputeObjDT(velocityMagnitude);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline double OrbitalPhysics::LocalGravitationalParameter(double const localPrimaryMass, double const localMetersPerUnitLength)
{
	return kGravitational * localPrimaryMass * pow(localMetersPerUnitLength, -3.0);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline void OrbitalPhysics::UpdateQueuePushFront(ObjectNode objNode)
{
	if (m_pCtx->m_UpdateQueueFront.IsNull())
	{
		m_pCtx->m_UpdateQueueFront = objNode;
		objNode.Motion().UpdateNext = ObjectNode::NNull();
	}
	else
	{
		objNode.Motion().UpdateNext = m_pCtx->m_UpdateQueueFront;
		m_pCtx->m_UpdateQueueFront = objNode;
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline double OrbitalPhysics::ApproximateDeltaTrueAnomaly(Vector3d const& posFromPrimary, double distFromPrimary,
	Vector3d const& velFromPrimary, double objDT)
{
	double vHorz = sqrt(velFromPrimary.SqrMagnitude() - pow(velFromPrimary.Dot(posFromPrimary) / distFromPrimary, 2.0));
	return objDT * vHorz / distFromPrimary;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline enum class OrbitalPhysics::Motion::Integration OrbitalPhysics::SelectIntegrationMethod(double deltaTrueAnomaly,
	bool isDynamicallyAccelerating)
{
	return (!isDynamicallyAccelerating && (kMinUpdateTrueAnomaly < deltaTrueAnomaly) ?
		Motion::Integration::Angular : Motion::Integration::Linear);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::GetRootObjectNode()
{
	return ObjectNode(kRootObjId);
}

// -------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::GetRootLSpaceNode()
{
	return LSpaceNode(kRootLspId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::Has(TNodeId nodeId)
{
	return m_pCtx->m_Tree.Has(nodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::Create(LSpaceNode lspNode, double mass, Vector3 const& position, bool isDynamic)
{
	return Create(lspNode, mass, position, CircularOrbitVelocity(lspNode, position), isDynamic);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::Create(LSpaceNode lspNode, bool isDynamic)
{
	return Create(lspNode, 0.0, Vector3(0.f), Vector3d(0.0), isDynamic);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::Create(bool isDynamic)
{
	return Create(LSpaceNode(kRootLspId), 0.0, Vector3(0.f), Vector3d(0.0), isDynamic);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline double OrbitalPhysics::CircularOrbitSpeed(LSpaceNode lspNode, float localRadius)
{
	/* ||V_circular|| = sqrt(mu / ||r||), where mu is the gravitational parameter of the orbit */
	return sqrt(lspNode.LSpace().Grav / (double)localRadius);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Vector3d OrbitalPhysics::ComputeLocalVelocity(ObjectNode object, LSpaceNode lsp)
{
	return ComputeLocalVelocity(object.GetState().Velocity, object.ParentLsp(), lsp);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::TNodeId OrbitalPhysics::ObjectNode::Id() const
{
	return m_NodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr OrbitalPhysics::ObjectNode OrbitalPhysics::ObjectNode::NNull()
{
	return ObjectNode();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::ObjectNode::IsNull() const
{
	return m_NodeId == OrbitalPhysics::NNull;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::ObjectNode::IsRoot() const
{
	return m_NodeId == kRootObjId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::ObjectNode::IsDynamic() const
{
	return m_pCtx->m_Dynamics.Has(m_NodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::ObjectNode::IsInfluencing() const
{
	return !this->Object().Influence.IsNull();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::ObjectNode::HasChildLSpace() const
{
	return m_pCtx->m_Tree[m_NodeId].FirstChild != IdNull;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Object const& OrbitalPhysics::ObjectNode::GetObj() const
{
	return this->Object();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::State const& OrbitalPhysics::ObjectNode::GetState() const
{
	return this->State();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Motion const& OrbitalPhysics::ObjectNode::GetMotion() const
{
	return this->Motion();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Dynamics const& OrbitalPhysics::ObjectNode::GetDynamics() const
{
	return this->Dynamics();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::ObjectNode::ParentLsp() const
{
	return LSpaceNode{ m_pCtx->m_Tree.GetParent(m_NodeId) };
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::ObjectNode::ParentObj() const
{
	return ObjectNode{ m_pCtx->m_Tree.GetGrandparent(m_NodeId) };
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::ObjectNode::PrimaryLsp() const
{
	return m_pCtx->m_LSpaces[m_pCtx->m_Tree.GetParent(m_NodeId)].Primary;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::ObjectNode::PrimaryObj() const
{
	return m_pCtx->m_LSpaces[m_pCtx->m_Tree.GetParent(m_NodeId)].Primary.ParentObj();
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::ObjectNode::FirstChildLSpace() const
{
	return LSpaceNode{ m_pCtx->m_Tree[m_NodeId].FirstChild };
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::ObjectNode::SphereOfInfluence() const
{
	return m_pCtx->m_Objects[m_NodeId].Influence;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline Vector3d OrbitalPhysics::ObjectNode::CircularOrbitVelocity() const
{
	return OrbitalPhysics::CircularOrbitVelocity(
		LSpaceNode{ m_pCtx->m_Tree[m_NodeId].Parent }, m_pCtx->m_States[m_NodeId].Position);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::ObjectNode::AddLocalSpace(float radius)
{
	return NewLSpaceNode(*this, radius);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::ObjectNode::operator==(ObjectNode const& rhs) const
{
	return this->m_NodeId == rhs.m_NodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::ObjectNode::operator!=(ObjectNode const& rhs) const
{
	return this->m_NodeId != rhs.m_NodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Node const& OrbitalPhysics::ObjectNode::Node() const
{
	return m_pCtx->m_Tree[m_NodeId];
}



// ---------------------------------------------------------------------------------------------------------------------------------

inline int OrbitalPhysics::ObjectNode::Height() const
{
	return m_pCtx->m_Tree.Height(m_NodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Object& OrbitalPhysics::ObjectNode::Object() const
{
	return m_pCtx->m_Objects[m_NodeId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::State& OrbitalPhysics::ObjectNode::State() const
{
	return m_pCtx->m_States[m_NodeId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Motion& OrbitalPhysics::ObjectNode::Motion() const
{
	return m_pCtx->m_Motions[m_NodeId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Dynamics& OrbitalPhysics::ObjectNode::Dynamics() const
{
	return m_pCtx->m_Dynamics[m_NodeId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::OrbitSection& OrbitalPhysics::ObjectNode::Orbit() const
{
	TId orbitId = m_pCtx->m_Motions[m_NodeId].Orbit;
	LV_ASSERT(orbitId != IdNull, "Object does not have an Orbit!");
	return m_pCtx->m_OrbitSections[orbitId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode::operator OrbitalPhysics::TNodeId() const
{
	return m_NodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::TNodeId OrbitalPhysics::LSpaceNode::Id() const
{
	return m_NodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline constexpr OrbitalPhysics::LSpaceNode OrbitalPhysics::LSpaceNode::NNull()
{
	return {};
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::IsNull() const
{
	return m_NodeId == OrbitalPhysics::NNull;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::IsRoot() const
{
	return m_NodeId == kRootLspId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::IsLargestLSpaceOnObject() const
{
	return m_pCtx->m_Tree[m_NodeId].PrevSibling == OrbitalPhysics::NNull;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::IsSmallestLSpaceOnObject() const
{
	return m_pCtx->m_Tree[m_NodeId].NextSibling == OrbitalPhysics::NNull;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::IsInfluencing() const
{
	return m_NodeId == m_pCtx->m_LSpaces[m_NodeId].Primary.m_NodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::IsSphereOfInfluence() const
{
	return m_NodeId == ParentObj().Object().Influence.m_NodeId;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LocalSpace const& OrbitalPhysics::LSpaceNode::GetLSpace() const
{
	return m_pCtx->m_LSpaces[m_NodeId];
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::LSpaceNode::ParentObj() const
{
	return ObjectNode{ m_pCtx->m_Tree.GetParent(m_NodeId) };
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::LSpaceNode::ParentLsp() const
{
	return LSpaceNode{ m_pCtx->m_Tree.GetGrandparent(m_NodeId) };
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::LSpaceNode::PrimaryLsp() const
{
	return m_pCtx->m_LSpaces[m_NodeId].Primary;
}

// ---------------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::ObjectNode OrbitalPhysics::LSpaceNode::PrimaryObj() const
{
	return m_pCtx->m_LSpaces[m_NodeId].Primary.ParentObj();
}

// -------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::LSpaceNode::OuterLSpace() const
{
	TNodeId const prevSibling = m_pCtx->m_Tree[m_NodeId].PrevSibling;

	return LSpaceNode(OrbitalPhysics::NNull == prevSibling ? m_pCtx->m_Tree.GetGrandparent(m_NodeId) : prevSibling);
}

// -------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode OrbitalPhysics::LSpaceNode::InnerLSpace() const
{
	return { m_pCtx->m_Tree[m_NodeId].NextSibling }; /* returns Null LSpace if no inner local space exists! */
}

// -------------------------------------------------------------------------------------------------------------------------

inline float OrbitalPhysics::LSpaceNode::InnerLSpaceLocalRadius() const
{
	return (IsSmallestLSpaceOnObject() ? 0.f : (InnerLSpace().LSpace().Radius / LSpace().Radius));
}


// -------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::operator==(LSpaceNode const& rhs) const
{
	return this->m_NodeId == rhs.m_NodeId;
}

// -------------------------------------------------------------------------------------------------------------------------

inline bool OrbitalPhysics::LSpaceNode::operator!=(LSpaceNode const& rhs) const
{
	return this->m_NodeId != rhs.m_NodeId;
}

// -------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::Node const& OrbitalPhysics::LSpaceNode::Node() const
{
	return m_pCtx->m_Tree[m_NodeId];
}

// -------------------------------------------------------------------------------------------------------------------------

inline int OrbitalPhysics::LSpaceNode::Height() const
{
	return m_pCtx->m_Tree.Height(m_NodeId);
}

// -------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LocalSpace& OrbitalPhysics::LSpaceNode::LSpace() const
{
	return m_pCtx->m_LSpaces[m_NodeId];
}

// -------------------------------------------------------------------------------------------------------------------------

inline OrbitalPhysics::LSpaceNode::operator OrbitalPhysics::TNodeId() const
{
	return m_NodeId;
}

// -------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------------------

inline float OrbitalPhysics::Elements::RadiusAt(float const trueAnomaly) const
{
	return P / (1.f + (E * cosf(trueAnomaly)));
}

// -------------------------------------------------------------------------------------------------------------------------

inline Vector3d OrbitalPhysics::Elements::VelocityAt(float const trueAnomaly) const
{
	return VConstant * static_cast<Vector3d>(((E + cosf(trueAnomaly)) * PerifocalY) - (sinf(trueAnomaly) * PerifocalX));
}

// -------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------------------

inline Vector3 OrbitalPhysics::OrbitSection::LocalPositionAt(float const trueAnomaly) const
{
	return Elements.PositionAt(trueAnomaly) - LocalSpace.LocalOffsetFromPrimary();
}

} // namespace Limnova

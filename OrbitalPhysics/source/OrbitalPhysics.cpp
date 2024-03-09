#include "OrbitalPhysics/OrbitalPhysics.h"

#include <algorithm>

namespace Limnova
{

OrbitalPhysics::Context*			OrbitalPhysics::m_pCtx		= nullptr;

constexpr OrbitalPhysics::TNodeId	OrbitalPhysics::kRootObjId	= 0;
constexpr OrbitalPhysics::TNodeId	OrbitalPhysics::kRootLspId	= 1;

// ---------------------------------------------------------------------------------------------------------------------------------

std::string OrbitalPhysics::ValidityToString(Validity v)
{
	switch (v)
	{
	case Validity::InvalidParent:		return "InvalidParent";
	case Validity::InvalidSpace:		return "InvalidSpace";
	case Validity::InvalidMass:			return "InvalidMass";
	case Validity::InvalidPosition:		return "InvalidPosition";
	case Validity::InvalidMotion:		return "InvalidMotion";
	case Validity::Valid:				return "Valid";
	}

	LV_ASSERT(false, "Unknown Validity ({})", static_cast<uint8_t>(v));

	return std::string();
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::ObjectNode OrbitalPhysics::NewObjectNode(LSpaceNode parentNode)
{
	TNodeId newNodeId = m_pCtx->m_Tree.New(parentNode.m_NodeId);

	m_pCtx->m_Objects.Add(newNodeId);
	m_pCtx->m_States.Add(newNodeId);
	m_pCtx->m_Motions.Add(newNodeId);

	return ObjectNode(newNodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::RemoveObjectNode(ObjectNode objNode)
{
	m_pCtx->m_Dynamics.TryRemove(objNode.m_NodeId);

	if (IdNull != objNode.Motion().Orbit)
		DeleteOrbit(objNode.Motion().Orbit);

	m_pCtx->m_Motions.Remove(objNode.m_NodeId);
	m_pCtx->m_States.Remove(objNode.m_NodeId);
	m_pCtx->m_Objects.Remove(objNode.m_NodeId);

	m_pCtx->m_Tree.Remove(objNode.m_NodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::RescaleLocalSpaces(ObjectNode objNode, float const rescalingFactor)
{
	double const parentLspMetersPerRadius = objNode.ParentLsp().LSpace().MetersPerRadius;

	std::vector<LSpaceNode> lspNodes;
	objNode.GetLocalSpaces(lspNodes);

	if (rescalingFactor > 1.f)
	{
		for (std::vector<LSpaceNode>::iterator lspNodeIt = lspNodes.begin(); lspNodes.cend() != lspNodeIt; ++lspNodeIt)
		{
			if (*lspNodeIt != objNode.SphereOfInfluence())
				lspNodeIt->SetRadius(lspNodeIt->LSpace().Radius * rescalingFactor);
		}
	}
	else
	{
		for (std::vector<LSpaceNode>::reverse_iterator lspNodeIt = lspNodes.rbegin(); lspNodeIt != lspNodes.rend(); ++lspNodeIt)
		{
			if (*lspNodeIt != objNode.SphereOfInfluence())
				lspNodeIt->SetRadius(lspNodeIt->LSpace().Radius * rescalingFactor);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::PromoteObjectNode(ObjectNode objNode)
{
	LSpaceNode oldLspNode = objNode.ParentLsp();

	LV_ASSERT(!oldLspNode.IsRoot(), "Cannot promote objects in the root local space!");

	LSpaceNode newLspNode = oldLspNode.OuterLSpace();

	float rescalingFactor;
	double rescalingFactord;

	State& state = objNode.State();

	if (oldLspNode.IsLargestLSpaceOnObject())
	{
		rescalingFactor = oldLspNode.LSpace().Radius;
		rescalingFactord = static_cast<double>(rescalingFactor);

		state.Position = (state.Position * rescalingFactor) + oldLspNode.ParentObj().State().Position;
		state.Velocity = (state.Velocity * rescalingFactord) + oldLspNode.ParentObj().State().Velocity;
	}
	else
	{
		rescalingFactord = static_cast<double>(oldLspNode.LSpace().Radius) / static_cast<double>(newLspNode.LSpace().Radius);
		rescalingFactor = static_cast<float>(rescalingFactord);

		state.Position *= rescalingFactor;
		state.Velocity *= rescalingFactord;
	}

	state.Acceleration *= rescalingFactord;

	if (objNode.IsDynamic())
		objNode.Dynamics().ContAcceleration *= rescalingFactord;

	m_pCtx->m_Tree.Move(objNode.m_NodeId, newLspNode.m_NodeId);

	RescaleLocalSpaces(objNode, rescalingFactor);
	TryPrepareObject(objNode);
	TryPrepareSubtree(objNode.m_NodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::DemoteObjectNode(LSpaceNode newLspNode, ObjectNode objNode)
{
	LV_ASSERT(newLspNode.ParentLsp().m_NodeId == objNode.ParentLsp().m_NodeId, "Local space is not in the same local space as the given object.");

	double const rescalingFactord = 1.0 / static_cast<double>(newLspNode.LSpace().Radius);
	const float rescalingFactor = static_cast<float>(rescalingFactord);

	State const& parentState = newLspNode.ParentObj().State();

	State & state = objNode.State();

	state.Position = (state.Position - parentState.Position) * rescalingFactor;
	state.Velocity = (state.Velocity - parentState.Velocity) * rescalingFactord;
	state.Acceleration *= rescalingFactord;

	if (objNode.IsDynamic())
		objNode.Dynamics().ContAcceleration *= rescalingFactord;

	m_pCtx->m_Tree.Move(objNode.m_NodeId, newLspNode.m_NodeId);

	RescaleLocalSpaces(objNode, rescalingFactor);
	TryPrepareObject(objNode);
	TryPrepareSubtree(objNode.m_NodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::DemoteObjectNode(ObjectNode objNode)
{
	LSpaceNode const lspNode = objNode.ParentLsp();
	LSpaceNode newLspNode(lspNode.Node().NextSibling);

	LV_ASSERT(!newLspNode.IsNull(), "There is no next-lower local space!");

	double rescalingFactord = static_cast<double>(lspNode.LSpace().Radius) / static_cast<double>(newLspNode.LSpace().Radius);
	float rescalingFactor = static_cast<float>(rescalingFactord);

	auto& state = objNode.State();

	state.Position *= rescalingFactor;
	state.Velocity *= rescalingFactord;
	state.Acceleration *= rescalingFactord;

	if (objNode.IsDynamic())
		objNode.Dynamics().ContAcceleration *= rescalingFactord;

	m_pCtx->m_Tree.Move(objNode.m_NodeId, newLspNode.m_NodeId);

	RescaleLocalSpaces(objNode, rescalingFactor);
	TryPrepareObject(objNode);
	TryPrepareSubtree(objNode.m_NodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::LSpaceNode OrbitalPhysics::NewLSpaceNode(ObjectNode parentNode, float const radius)
{
	TNodeId newLspNodeId(m_pCtx->m_Tree.New(parentNode.m_NodeId));
	m_pCtx->m_LSpaces.Add(newLspNodeId).Radius = 1.f;

	LSpaceNode newLspNode(newLspNodeId);
	newLspNode.SetRadius(radius);

	return newLspNode;
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::LSpaceNode OrbitalPhysics::NewSoiNode(ObjectNode parentNode, float const radiusOfInfluence)
{
	LV_ASSERT(parentNode.Object().Influence.IsNull(), "Object ({}) already has sphere of influence.", parentNode.m_NodeId);

	TNodeId newSoiNodeId(m_pCtx->m_Tree.New(parentNode.m_NodeId));
	m_pCtx->m_LSpaces.Add(newSoiNodeId).Radius = 1.f;

	LSpaceNode newSoiNode(newSoiNodeId);

	parentNode.Object().Influence = newSoiNode;

	// Use the implementation of the SetRadius function directly to bypass checks which prevent setting the radius of an SOI.
	newSoiNode.SetRadiusImpl(radiusOfInfluence);

	return newSoiNode;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::RemoveLSpaceNode(LSpaceNode lspNode)
{
	m_pCtx->m_LSpaces.Remove(lspNode.m_NodeId);
	m_pCtx->m_Tree.Remove(lspNode.m_NodeId);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::DeleteOrbit(TId& sectionId)
{
	while (sectionId != IdNull)
	{
		TId nextSection = m_pCtx->m_OrbitSections.Get(sectionId).Next;

		m_pCtx->m_OrbitSections.Erase(sectionId);

		sectionId = nextSection;
	}

	sectionId = IdNull;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ComputeOrbit(TId sectionId, Vector3 const& localPosition, Vector3d const& localVelocity, size_t const maxSections)
{
	for (size_t i = 0; i < maxSections; ++i)
	{
		OrbitSection& section = m_pCtx->m_OrbitSections.Get(sectionId);

		ComputeElements(section, localPosition, localVelocity);
		ComputeTaLimits(section);

		if (section.TaExit == PI2f)
			break;

		break; // TODO : add new section(s) and loop over
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ComputeTaLimits(OrbitSection& section)
{
	auto& elems = section.Elements;

	section.TaEntry = 0.f;
	section.TaExit = PI2f;
	if (false /*section.LocalSpace.IsInfluencing()*/) // TODO : fix
	{
		// Local space escape
		float apoapsisRadius = elems.SemiMajor * (1.f + elems.E);
		if (elems.Type == OrbitType::Hyperbola || apoapsisRadius > kLocalSpaceEscapeRadius)
		{
			section.TaExit = acosf((elems.P / kLocalSpaceEscapeRadius - 1.f) / elems.E);
			section.TaEntry = PI2f - section.TaExit;
		}
		// Inner space entry
		else if (!section.LocalSpace.IsSmallestLSpaceOnObject())
		{
			float periapsisRadius = elems.SemiMajor * (1.f - elems.E);
			float innerSpaceRelativeRadius = section.LocalSpace.InnerLSpace().LSpace().Radius / section.LocalSpace.LSpace().Radius;

			if (periapsisRadius < innerSpaceRelativeRadius)
			{
				section.TaEntry = acosf((elems.P / innerSpaceRelativeRadius - 1.f) / elems.E);
				section.TaExit = PI2f - section.TaEntry;
			}
		}
	}
	else
	{
		// TODO : get ta limits in primary space
		auto primarySpace = section.LocalSpace.PrimaryLsp();
		float primarySpaceRelativeScaling = primarySpace.LSpace().MetersPerRadius / section.LocalSpace.LSpace().MetersPerRadius;
		float escapeRadius = kLocalSpaceEscapeRadius * primarySpaceRelativeScaling;

		// COPY FROM ABOVE (+ edits)
		// Check for local space escape
		float apoapsisRadius = elems.SemiMajor * (1.f + elems.E);
		if (elems.Type == OrbitType::Hyperbola || apoapsisRadius > escapeRadius)
		{
			section.TaExit = acosf((elems.P / escapeRadius - 1.f) / elems.E);
			section.TaEntry = PI2f - section.TaExit;
		}
		// Check for inner space entry
		else if (!primarySpace.IsSmallestLSpaceOnObject())
		{
			float periapsisRadius = elems.SemiMajor * (1.f - elems.E);
			float innerSpaceRelativeRadius = primarySpace.InnerLSpace().LSpace().Radius / primarySpace.LSpace().Radius * primarySpaceRelativeScaling;

			if (periapsisRadius < innerSpaceRelativeRadius)
			{
				section.TaEntry = acosf((elems.P / innerSpaceRelativeRadius - 1.f) / elems.E);
				section.TaExit = PI2f - section.TaEntry;
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ComputeElements(OrbitSection& section, Vector3 const& localPosition, Vector3d const& localVelocity)
{
	Elements& elems = section.Elements;
	LocalSpace& lsp = section.LocalSpace.LSpace();

	Vector3 positionFromPrimary = localPosition + section.LocalSpace.LocalOffsetFromPrimary();
	Vector3d velocityFromPrimary = localVelocity + section.LocalSpace.LocalVelocityFromPrimary();

	Vector3d Hvec = Vector3d(positionFromPrimary).Cross(velocityFromPrimary);
	double H2 = Hvec.SqrMagnitude();
	elems.H = sqrt(H2);
	if (elems.H == 0)
	{
		/* handle position or velocity being zero */
		elems = Elements();
		return;
	}
	elems.PerifocalNormal = (Vector3)(Hvec / elems.H);

	/* Loss of precision due to casting is acceptable: semi-latus rectum is on the order of 1 in all common cases, due to distance parameterisation */
	elems.P = (float)(H2 / lsp.Grav);
	elems.VConstant = lsp.Grav / elems.H;
	elems.MConstant = pow(lsp.Grav, 2.0) / pow(elems.H, 3.0);

	/* Loss of precision due to casting is acceptable: result of vector division (V x H / Grav) is on the order of 1 */
	Vector3 posDir = positionFromPrimary.Normalized();
	Vector3 Evec = (Vector3)(velocityFromPrimary.Cross(Hvec) / lsp.Grav) - posDir;
	float e2 = Evec.SqrMagnitude();
	elems.E = sqrtf(e2);

	float e2term;
	if (elems.E < kEccentricityEpsilon)
	{
		// Circular
		elems.E = 0.f;
		elems.Type = OrbitType::Circle;

		elems.PerifocalX = abs(elems.PerifocalNormal.Dot(kReferenceY)) > kParallelDotProductLimit
			? kReferenceX : kReferenceY.Cross(elems.PerifocalNormal);
		elems.PerifocalY = elems.PerifocalNormal.Cross(elems.PerifocalX);

		e2term = 1.f;
	}
	else
	{
		elems.PerifocalX = Evec / elems.E;
		elems.PerifocalY = elems.PerifocalNormal.Cross(elems.PerifocalX);

		if (elems.E < 1.f) {
			// Elliptical
			elems.Type = OrbitType::Ellipse;
			e2term = 1.f - e2;
		}
		else {
			// Hyperbolic
			elems.Type = OrbitType::Hyperbola;
			e2term = e2 - 1.f;
		}
		e2term += kEps; /* guarantees e2term > 0 */
	}

	// Dimensions
	elems.SemiMajor = elems.P / e2term;
	elems.SemiMinor = elems.SemiMajor * sqrtf(e2term);

	elems.C = elems.P / (1.f + elems.E);
	elems.C += (elems.Type == OrbitType::Hyperbola) ? elems.SemiMajor : -elems.SemiMajor; /* different center positions for elliptical and hyperbolic */

	elems.T = PI2 * (double)(elems.SemiMajor * elems.SemiMinor) / elems.H;

	// Frame orientation
	elems.I = acosf(elems.PerifocalNormal.Dot(kReferenceNormal));
	elems.N = abs(elems.PerifocalNormal.Dot(kReferenceNormal)) > kParallelDotProductLimit
		? elems.PerifocalX : kReferenceNormal.Cross(elems.PerifocalNormal).Normalized();
	elems.Omega = acosf(elems.N.Dot(kReferenceX));
	if (elems.N.Dot(kReferenceY) < 0.f) {
		elems.Omega = PI2f - elems.Omega;
	}
	elems.ArgPeriapsis = AngleBetweenUnitVectorsf(elems.N, elems.PerifocalX);
	if (elems.N.Dot(elems.PerifocalY) > 0.f) {
		elems.ArgPeriapsis = PI2f - elems.ArgPeriapsis;
	}
	elems.PerifocalOrientation =
		Quaternion(elems.PerifocalNormal, elems.ArgPeriapsis)
		* Quaternion(elems.N, elems.I)
		* Quaternion(kReferenceNormal, elems.Omega);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ComputeInfluence(ObjectNode objNode)
{
	LV_ASSERT(!objNode.IsRoot(), "Cannot compute influence of root object.");

	Object& obj = objNode.Object();

	/* Radius of influence = a(m / M)^0.4
		* Semi-major axis must be in the order of 1,
		* so the order of ROI is determined by (m / M)^0.4 */
	float massFactor = static_cast<float>(pow(objNode.State().Mass / objNode.PrimaryObj().State().Mass, 0.4));
	float radiusOfInfluence = objNode.GetOrbit().Elements.SemiMajor * massFactor;
	radiusOfInfluence = std::min(radiusOfInfluence, kMaxLSpaceRadius + kEps * kMaxLSpaceRadius); /* restrict size while still causing InvalidMotion */

	if (!objNode.IsDynamic() && (kMinLSpaceRadius < radiusOfInfluence))
	{
		if (obj.Influence.IsNull())
		{
			NewSoiNode(objNode, radiusOfInfluence);
		}
		else
		{
			LV_ASSERT(obj.Influence.LSpace().Primary.m_NodeId == obj.Influence.m_NodeId, "Sphere of influence should still be its own Primary.");

			obj.Influence.SetRadiusImpl(radiusOfInfluence);
		}

		LV_ASSERT(!obj.Influence.IsNull() && m_pCtx->m_LSpaces.Has(obj.Influence.m_NodeId), "Failed to create sphere of influence!");
	}
	else if (!obj.Influence.IsNull())
	{
		// Object was previously influencing - remove old sphere of influence
		CollapseLocalSpace(obj.Influence);
		obj.Influence = LSpaceNode();
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::UpdateQueueRemove(ObjectNode objNode)
{
	LV_ASSERT(!m_pCtx->m_UpdateQueueFront.IsNull(), "Update queue is empty.");

	if (m_pCtx->m_UpdateQueueFront == objNode)
	{
		m_pCtx->m_UpdateQueueFront = objNode.Motion().UpdateNext;
		objNode.Motion().UpdateNext = ObjectNode::NNull();
		return;
	}

	ObjectNode queueItem = m_pCtx->m_UpdateQueueFront, queueNext = m_pCtx->m_UpdateQueueFront.Motion().UpdateNext;

	while (queueNext != objNode)
	{
		LV_ASSERT(!queueNext.IsNull(), "Could not find the given object in the update queue.");

		queueItem = queueNext;
		queueNext = queueNext.Motion().UpdateNext;
	}

	queueItem.Motion().UpdateNext = objNode.Motion().UpdateNext;
	objNode.Motion().UpdateNext = ObjectNode::NNull();
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool OrbitalPhysics::UpdateQueueSafeRemove(ObjectNode objNode)
{
	if (m_pCtx->m_UpdateQueueFront.IsNull()) return false;

	if (m_pCtx->m_UpdateQueueFront == objNode)
	{
		m_pCtx->m_UpdateQueueFront = objNode.Motion().UpdateNext;
		objNode.Motion().UpdateNext = ObjectNode::NNull();
		return true;
	}

	ObjectNode queueItem = m_pCtx->m_UpdateQueueFront, queueNext = m_pCtx->m_UpdateQueueFront.Motion().UpdateNext;

	while (!queueNext.IsNull())
	{
		if (queueNext == objNode)
		{
			queueItem.Motion().UpdateNext = objNode.Motion().UpdateNext;
			objNode.Motion().UpdateNext = ObjectNode::NNull();
			return true;
		}

		queueItem = queueNext;
		queueNext = queueNext.Motion().UpdateNext;
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::UpdateQueueSortFront()
{
	LV_ASSERT(!m_pCtx->m_UpdateQueueFront.IsNull(), "Queue is empty.");

	ObjectNode objNode = m_pCtx->m_UpdateQueueFront;
	Motion& motion = objNode.Motion();

	ObjectNode queueItem = motion.UpdateNext;

	if (queueItem.IsNull() || (motion.UpdateTimer < queueItem.Motion().UpdateTimer)) return;

	m_pCtx->m_UpdateQueueFront = queueItem;

	ObjectNode queueNext = queueItem.Motion().UpdateNext;
	while (!queueNext.IsNull())
	{
		if (motion.UpdateTimer < queueNext.Motion().UpdateTimer) break;

		queueItem = queueNext;
		queueNext = queueNext.Motion().UpdateNext;
	}

	queueItem.Motion().UpdateNext = objNode;
	motion.UpdateNext = queueNext;
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::Validity OrbitalPhysics::TryPrepareObject(ObjectNode objNode)
{
	UpdateQueueSafeRemove(objNode);

	Object& object = objNode.Object();

	object.Validity = Validity::Valid;

	if (!ValidParent(objNode))
		object.Validity = Validity::InvalidParent;
	else if (!ValidSpace(objNode))
		object.Validity = Validity::InvalidSpace;
	else if (!ValidMass(objNode))
		object.Validity = Validity::InvalidMass;
	else if (!ValidPosition(objNode))
		object.Validity = Validity::InvalidPosition;

	if (objNode.IsRoot() || (object.Validity != Validity::Valid))
		return object.Validity; // Root object does not have motion or dynamically computed influence so we return the root here regardless of validity

	// By this point, state is known to be valid: prepare Motion and Influence
	ComputeMotion(objNode);
	ComputeInfluence(objNode);

	if (!ValidMotion(objNode))
		object.Validity = Validity::InvalidMotion;
	else
		UpdateQueuePushFront(objNode); // All tests passed: object can safely be simulated

	return object.Validity;
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool OrbitalPhysics::ValidMotion(ObjectNode objNode)
{
	if (!objNode.IsDynamic())
	{
		OrbitSection const& orbit = objNode.GetOrbit();

		if (orbit.TaExit < PI2)
		{
			LV_WARN("Object {0} has invalid motion: non-dynamic objects cannot exit their local space.", objNode.m_NodeId);
			return false;
		}

		if (objNode.IsInfluencing())
		{
			float const roi = objNode.SphereOfInfluence().LSpace().Radius;

			if (roi > kMaxLSpaceRadius)
			{
				LV_WARN("Object {0} has invalid motion: sphere of influence is too wide. Reduce orbit radius or object mass.", objNode.m_NodeId);
				return false;
			}

			if ((kLocalSpaceEscapeRadius < (orbit.Elements.RadiusAt(PIf) + roi)) ||
				(objNode.ParentLsp().InnerLSpaceLocalRadius() < (orbit.Elements.RadiusAt(0.f) - roi)))
			{
				LV_WARN("Object {0} has invalid motion: sphere of influence is crossing local space boundaries.", objNode.m_NodeId);
				return false;
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool OrbitalPhysics::ValidPosition(ObjectNode objNode)
{
	static constexpr float kEscapeDistance2 = (kLocalSpaceEscapeRadius * kLocalSpaceEscapeRadius) - kEps;

	if (objNode.IsRoot()) return true;

	LSpaceNode parentLsp = objNode.ParentLsp();

	float const innerSpaceRadius = (parentLsp.IsSmallestLSpaceOnObject() ? 0.f : (parentLsp.InnerLSpace().LSpace().Radius / parentLsp.LSpace().Radius));
	float const posMag2 = objNode.State().Position.SqrMagnitude();
	float const posFromPrimaryMag2 = objNode.LocalPositionFromPrimary().SqrMagnitude();

	if (posFromPrimaryMag2 < kEps)
	{
		LV_WARN("Object {} has invalid position: distance from primary object {} is zero.", objNode.m_NodeId, objNode.PrimaryObj().m_NodeId);
		return false;
	}
	if ((kEscapeDistance2 < posMag2) || (posMag2 < ((innerSpaceRadius * innerSpaceRadius) + kEps)))
	{
		LV_WARN("Object {} has invalid position: object is outside local space boundaries.", objNode.m_NodeId);
		return false;
	}

	std::vector<ObjectNode> siblings;
	parentLsp.GetLocalObjects(siblings);

	for (ObjectNode sibNode : siblings)
	{
		if (sibNode == objNode) continue;

		float const separation = sqrtf((objNode.State().Position - sibNode.State().Position).SqrMagnitude());
		float const minSeparation = kEps + (sibNode.HasChildLSpace() ? sibNode.FirstChildLSpace().LSpace().Radius : 0.f);

		if (separation < minSeparation)
		{
			LV_WARN("Object {} has invalid position: overlapping with another object's {} local space!", objNode.m_NodeId, sibNode.m_NodeId);
			return false;
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool OrbitalPhysics::ValidMass(ObjectNode objNode)
{
	static constexpr double kMaxCOG = 1e-4; /* Maximum offset for shared centre of gravity with a separation distance of 1 */

	static const float kMaxDynamicMassRatio = powf(kMinLSpaceRadius, 2.5f);

	State& state = objNode.State();

	if (state.Mass <= 0.0)
	{
		LV_WARN("Object {0} has invalid mass: mass is not non-negative.", objNode.m_NodeId);
		return false;
	}

	if (objNode.IsRoot()) return true;

	double const primaryMass = objNode.PrimaryObj().State().Mass;
	double massRatio = state.Mass / (state.Mass + primaryMass);

	if (kMaxCOG < massRatio)
	{
		LV_WARN("Object {0} has invalid mass: ratio with primary {1} mass ({2}) is too high "
			"(ratio is m / (m + M) = {3}, must be less than {4})!",
			objNode.m_NodeId, objNode.PrimaryObj().m_NodeId, primaryMass, massRatio, kMaxCOG);
		return false;
	}

	massRatio = state.Mass / primaryMass;

	if (objNode.IsDynamic() && (kMaxDynamicMassRatio < static_cast<float>(massRatio)))
	{
		LV_WARN("Object {0} has invalid mass: ratio with primary {1} mass ({2}) is too high for a dynamic object "
			"(ratio is m/M = {3}, must be less than {4} for dynamic objects)!",
			objNode.m_NodeId, objNode.PrimaryObj().m_NodeId, primaryMass, massRatio, kMaxDynamicMassRatio);
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool OrbitalPhysics::ValidSpace(ObjectNode objNode)
{
	if (objNode.IsRoot()) return true;

	if (!objNode.IsDynamic() && !objNode.ParentLsp().IsInfluencing())
	{
		LV_WARN("Object {0} invalid local space {1}: non-dynamic object cannot belong to a non-influencing space.", objNode.m_NodeId, objNode.ParentLsp().m_NodeId);
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::TryPrepareSubtree(TNodeId rootNodeId)
{
	std::vector<TNodeId> tree;
	m_pCtx->m_Tree.GetSubtree(rootNodeId, tree);

	for (TNodeId nodeId : tree)
	{
		if (IsLocalSpace(nodeId))
		{
			LSpaceNode subLspNode(nodeId);

			if (!subLspNode.IsRoot() && !subLspNode.IsSphereOfInfluence())
				subLspNode.SetRadius(subLspNode.LSpace().Radius); /* recomputes MetersPerRadius and Grav */
		}
		else
		{
			// TODO : preserve orbit shapes ?

			ObjectNode subObjNode(nodeId);

			TryPrepareObject(subObjNode);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool OrbitalPhysics::ValidParent(ObjectNode objNode)
{
	if (objNode.IsRoot())
		return (Validity::InvalidParent != objNode.Object().Validity); /* see SetRootSpaceScaling() */

	if (objNode.ParentObj().Object().Validity != Validity::Valid)
	{
		LV_WARN("Object {0} invalid parent {1}: parent Validity must be Valid (was {2}).",
			objNode.m_NodeId, objNode.ParentObj().m_NodeId, ValidityToString(objNode.ParentObj().Object().Validity));
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ComputeMotion(ObjectNode objNode)
{
	LV_ASSERT(!objNode.IsRoot(), "Attempted to compute motion of root object.");

	State& state = objNode.State();
	Motion& motion = objNode.Motion();

	if (IdNull != motion.Orbit)
		DeleteOrbit(motion.Orbit);

	motion.PrevDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()));

	double approxDTrueAnomaly, posMag2;
	Vector3d posDir;
	bool isDynamicallyAccelerating;
	{
		// Approximate delta true anomaly (dTa) as the angle change in position direction caused by velocity across delta time (dT)
		Vector3d const posFromPrimary(objNode.LocalPositionFromPrimary());
		posMag2 = posFromPrimary.SqrMagnitude();

		double const r = sqrt(posMag2);
		posDir = posFromPrimary / r;

		double const approxDTrueAnomaly =
			ApproximateDeltaTrueAnomaly(posFromPrimary, r, objNode.LocalVelocityFromPrimary(), motion.PrevDT);

		isDynamicallyAccelerating = objNode.IsDynamic() && !objNode.Dynamics().ContAcceleration.IsZero();

		motion.Integration = SelectIntegrationMethod(approxDTrueAnomaly, isDynamicallyAccelerating);
	}

	switch (motion.Integration)
	{
	case Motion::Integration::Angular:
	{
		// Angular integration
		motion.Integration = Motion::Integration::Angular;

		OrbitSection const& orbit = objNode.GetOrbit(); /* creates Orbit */

		motion.TrueAnomaly = orbit.Elements.TrueAnomalyOf(Vector3(posDir));
		motion.DeltaTrueAnomaly = (motion.PrevDT * orbit.Elements.H) / posMag2;

		break;
	}
	case Motion::Integration::Linear:
	{
		// Linear integration
		motion.Integration = Motion::Integration::Linear;

		state.Acceleration = -posDir * objNode.ParentLsp().LSpace().Grav / posMag2;

		if (isDynamicallyAccelerating)
			state.Acceleration += objNode.Dynamics().ContAcceleration;

		break;
	}
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::OnUpdate(double dT)
{
	if (m_pCtx->m_UpdateQueueFront.IsNull()) return;

	// Subtract elapsed time from all object timers
	ObjectNode objNode = m_pCtx->m_UpdateQueueFront;
	do
	{
		objNode.Motion().UpdateTimer -= dT;
		objNode = objNode.Motion().UpdateNext;
	}
	while (!objNode.IsNull());


	double minObjDT = dT / kMaxObjectUpdates;

	// Update all objects with timers less than 0
	ObjectNode& updateNode = m_pCtx->m_UpdateQueueFront;
	while (updateNode.Motion().UpdateTimer < 0.0)
	{
		LSpaceNode lspNode = updateNode.ParentLsp();
		LocalSpace& lsp = lspNode.LSpace();
		Object& obj = updateNode.Object();
		State& state = updateNode.State();
		Motion& motion = updateNode.Motion();
		bool const isDynamic = updateNode.IsDynamic();

		double& objDT = motion.PrevDT; // Reference for readability

		// Motion integration
		switch (motion.Integration)
		{
		case Motion::Integration::Angular:
		{
			/* Integrate true anomaly:
			* dTrueAnomaly / dT = h / r^2
			* */
			OrbitSection const& orbit = updateNode.Orbit();
			Elements const& elems = orbit.Elements;

			motion.TrueAnomaly += motion.DeltaTrueAnomaly;
			motion.TrueAnomaly = Wrapf(motion.TrueAnomaly, PI2f);

			// Compute new state
			float const sinT = sinf(static_cast<float>(motion.TrueAnomaly));
			float const cosT = cosf(static_cast<float>(motion.TrueAnomaly));

			float const r = elems.P / (1.f + (elems.E * cosT)); /* orbit equation: r = h^2 / mu * 1 / (1 + e * cos(trueAnomaly)) */

			/* state according to elements (local distance scaling, relative to primary) */
			state.Position = r * (cosT * elems.PerifocalX + sinT * elems.PerifocalY);
			state.Velocity = elems.VConstant * (Vector3d)((elems.E + cosT) * elems.PerifocalY - sinT * elems.PerifocalX);

			/* state relative to local space */
			LSpaceNode parentLspNode = updateNode.ParentLsp();
			state.Position -= parentLspNode.LocalOffsetFromPrimary();
			state.Velocity -= parentLspNode.LocalVelocityFromPrimary();

			objDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()), minObjDT);
			motion.DeltaTrueAnomaly = (objDT * elems.H) / static_cast<double>(r * r);

			// Re-select integration method
			motion.Integration = SelectIntegrationMethod(motion.DeltaTrueAnomaly);

			if (motion.Integration == Motion::Integration::Linear)
			{
				// Prepare Linear integration
				Vector3d const positionFromPrimary(updateNode.LocalPositionFromPrimary());

				double const posMag2 = positionFromPrimary.SqrMagnitude();
				Vector3d const posDir = positionFromPrimary / sqrt(posMag2);

				state.Acceleration = -posDir * lsp.Grav / posMag2;

				if (isDynamic)
					state.Acceleration += updateNode.Dynamics().ContAcceleration;
			}

			break;
		}
		case Motion::Integration::Linear:
		{
			/* Velocity verlet :
			* p1 = p0 + v0 * dT + 0.5 * a0 * dT^2
			* a1 = (-rDirection) * G * M / r^2 + dynamicAcceleration
			* v1 = v0 + 0.5 * (a0 + a1) * dT
			* */
			state.Position += Vector3(state.Velocity * objDT) + (0.5f * Vector3(state.Acceleration * objDT * objDT));

			Vector3d const positionFromPrimary(updateNode.LocalPositionFromPrimary());

			double const r2 = positionFromPrimary.SqrMagnitude();
			double const r = sqrt(r2);

			Vector3d newAcceleration = -positionFromPrimary * lsp.Grav / (r2 * r);

			bool isDynamicallyAccelerating = false;
			if (isDynamic)
			{
				newAcceleration += updateNode.Dynamics().ContAcceleration;
				isDynamicallyAccelerating = !updateNode.Dynamics().ContAcceleration.IsZero();
			}

			state.Velocity += 0.5 * (state.Acceleration + newAcceleration) * objDT;
			state.Acceleration = newAcceleration;

			objDT = ComputeObjDT(sqrt(state.Velocity.SqrMagnitude()), minObjDT);

			if (isDynamicallyAccelerating && (IdNull != motion.Orbit))
			{
				// Dynamic acceleration invalidates orbit:
				// Orbit was requested by the user in the previous frame so we need to delete the stored (now invalid) data.
				DeleteOrbit(motion.Orbit);

				// We do not need to compute the orbit of a linearly integrated object until it is requested (See ObjectNode::GetOrbit()),
				// so we don't need to recompute here.
			}

			// Re-select integration method
			double const approxDTrueAnomaly = ApproximateDeltaTrueAnomaly(positionFromPrimary, r, updateNode.LocalVelocityFromPrimary(), objDT);

			motion.Integration = SelectIntegrationMethod(approxDTrueAnomaly, isDynamicallyAccelerating);

			if (motion.Integration == Motion::Integration::Angular)
			{
				// Prepare Angular integration
				motion.DeltaTrueAnomaly = (motion.PrevDT * updateNode.GetOrbit().Elements.H) / r2; /* GetOrbit() creates or updates orbit */
			}

			break;
		}
		case Motion::Integration::Dynamic:
		{
			static constexpr double kMaxUpdateDistanced2 = kMaxPositionStepd * kMaxPositionStepd;

			Dynamics& dynamics = updateNode.Dynamics();

			if (IdNull != motion.Orbit)
			{
				// Dynamic acceleration invalidates orbit:
				// Orbit was requested by the user in the previous frame so we need to delete the stored (now invalid) data.
				DeleteOrbit(motion.Orbit);

				// We do not need to compute the orbit of a linearly integrated object until it is requested (See ObjectNode::GetOrbit()),
				// so we don't need to recompute here.
			}

			dynamics.DeltaPosition += (state.Velocity * objDT) + (0.5 * state.Acceleration * objDT * objDT);

			Vector3d const positionFromPrimary = Vector3d(updateNode.LocalPositionFromPrimary()) + dynamics.DeltaPosition;
			double const r2 = positionFromPrimary.SqrMagnitude();
			double const r = sqrt(r2);

			Vector3d const newAcceleration = dynamics.ContAcceleration - (positionFromPrimary * lsp.Grav / (r2 * r));

			state.Velocity += 0.5 * (state.Acceleration + newAcceleration) * objDT;
			state.Acceleration = newAcceleration;

			bool positionUpdated = false;

			double const deltaPosMag2 = dynamics.DeltaPosition.SqrMagnitude();
			if (kMaxUpdateDistanced2 < deltaPosMag2)
			{
				Vector3 const deltaPosition(dynamics.DeltaPosition);

				state.Position += deltaPosition;
				dynamics.DeltaPosition -= Vector3d(deltaPosition); // Accumulate rounded-off delta

				positionUpdated = true;
			}

			if (dynamics.ContAcceleration.IsZero())
			{
				double const v = sqrt(state.Velocity.SqrMagnitude());

				objDT = ComputeObjDT(v, minObjDT);

				if (positionUpdated)
				{
					// Switch to Angular or Linear integration
					double const approxDTrueAnomaly = ApproximateDeltaTrueAnomaly(positionFromPrimary, r, updateNode.LocalVelocityFromPrimary(), objDT);

					motion.Integration = SelectIntegrationMethod(approxDTrueAnomaly, false);

					if (motion.Integration == Motion::Integration::Angular)
						motion.DeltaTrueAnomaly = (motion.PrevDT * updateNode.GetOrbit().Elements.H) / r2; /* GetOrbit() creates or updates orbit */
				}
				else
				{
					// Prepare the next integration step so that it jumps to the next position update.
					objDT = std::max(minObjDT, objDT - ((kMaxPositionStepd - sqrt(deltaPosMag2)) / v));
				}
			}
			else
			{
				objDT = ComputeDynamicObjDT(sqrt(state.Velocity.SqrMagnitude()), sqrt(state.Acceleration.SqrMagnitude()), minObjDT);
			}
		}
		}

		// Test for orbit events
		if (isDynamic)
		{
			bool lspChanged = false;

			// Local escape
			float const r = sqrtf(state.Position.SqrMagnitude());
			if (kLocalSpaceEscapeRadius < r)
			{
				LV_ASSERT(!updateNode.ParentLsp().IsRoot(), "Object has escaped root local space.");
				lspChanged = true;
				PromoteObjectNode(updateNode);
			}
			// Inner space entry
			else if (!lspNode.IsSmallestLSpaceOnObject() && (r < lspNode.InnerLSpace().LSpace().Radius / lsp.Radius))
			{
				lspChanged = true;
				DemoteObjectNode(updateNode);
			}
			// Local subspace entry
			else
			{
				std::vector<ObjectNode> objs;
				lspNode.GetLocalObjects(objs);

				for (ObjectNode objNode : objs)
				{
					if (objNode == updateNode) continue; /* skip self */
					if (!objNode.HasChildLSpace()) continue; /* skip objs without subspaces */

					LSpaceNode subspaceNode = objNode.FirstChildLSpace();

					float const s = sqrtf((state.Position - objNode.State().Position).SqrMagnitude());

					if (s < subspaceNode.LSpace().Radius)
					{
						lspChanged = true;
						DemoteObjectNode(subspaceNode, updateNode);
					}
				}
			}

			if (lspChanged)
			{
				LV_ASSERT(obj.Validity == Validity::Valid, "Invalid dynamics after event.");
				CallParentLSpaceChangedCallback(updateNode);
			}
		}

		motion.UpdateTimer += objDT;
		UpdateQueueSortFront();
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::SetRootSpaceScaling(double meters)
{
	meters = std::max(meters, 1.0); /* minimum root scaling of 1 meter */

	LocalSpace& rootLsp = LSpaceNode(kRootLspId).LSpace();
	rootLsp.MetersPerRadius = meters;
	rootLsp.Grav = LocalGravitationalParameter(ObjectNode(kRootObjId).State().Mass, meters);

	ObjectNode rootObjNode(kRootObjId);

	Object& rootObj = rootObjNode.Object();
	rootObj.Validity = ValidMass(rootObjNode) ? Validity::Valid : Validity::InvalidMass;

	TryPrepareSubtree(kRootLspId);
}

OrbitalPhysics::ObjectNode OrbitalPhysics::Create(LSpaceNode lspNode, double mass, Vector3 const& position, Vector3d const& velocity, bool isDynamic)
{
	LV_ASSERT(!lspNode.IsNull(), "Local space node is uninitialized.");

	ObjectNode newObjNode = NewObjectNode(lspNode);

	State& state = newObjNode.State();
	state.Mass = mass;
	state.Position = position;
	state.Velocity = velocity;

	if (isDynamic)
		m_pCtx->m_Dynamics.Add(newObjNode.m_NodeId);

	Validity validity = TryPrepareObject(newObjNode);
	LV_LOG("Created OrbitalPhysics object {0} with validity '{1}'", newObjNode.m_NodeId, ValidityToString(validity));

	return newObjNode;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::Destroy(ObjectNode objNode)
{
	LV_ASSERT(!objNode.IsNull(), "Invalid object node.");

	// Move children into parent local space
	LSpaceNode parentLsp = objNode.ParentLsp();
	State& state = objNode.State();

	std::vector<LSpaceNode> lspaces;
	for (size_t i = 0; i < objNode.GetLocalSpaces(lspaces); ++i)
	{
		float const rescalingFactor = lspaces[i].LSpace().Radius;

		std::vector<ObjectNode> localObjs;
		lspaces[i].GetLocalObjects(localObjs);

		for (ObjectNode localObjNode : localObjs)
		{
			State& childState = localObjNode.State();
			childState.Position = (childState.Position * rescalingFactor) + state.Position;
			childState.Velocity = (childState.Velocity * static_cast<double>(rescalingFactor)) + state.Velocity;

			m_pCtx->m_Tree.Move(localObjNode.m_NodeId, parentLsp.m_NodeId);

			TryPrepareObject(localObjNode);
			TryPrepareSubtree(localObjNode.m_NodeId);
		}
	}

	UpdateQueueSafeRemove(objNode);
	RemoveObjectNode(objNode);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::CollapseLocalSpace(LSpaceNode lspNode)
{
	std::vector<ObjectNode> localObjs;
	lspNode.GetLocalObjects(localObjs);

	for (ObjectNode objNode : localObjs)
		PromoteObjectNode(objNode);

	LV_ASSERT(m_pCtx->m_Tree[lspNode.m_NodeId].FirstChild == OrbitalPhysics::NNull, "Failed to remove children.");

	ObjectNode parentObjNode = lspNode.ParentObj();

	RemoveLSpaceNode(lspNode);

	CallChildLSpacesChangedCallback(parentObjNode);

	for (ObjectNode objNode : localObjs)
		CallParentLSpaceChangedCallback(objNode);
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3d OrbitalPhysics::CircularOrbitVelocity(LSpaceNode lspNode, Vector3 const& localPosition)
{
	// Keep the orbital plane as flat (close to the reference plane) as possible:
	// derive velocity direction as the cross product of the reference normal and normalized position.
	Vector3d vDir;
	Vector3 const positionFromPrimary = localPosition + lspNode.LocalOffsetFromPrimary();
	float rMag = sqrtf(positionFromPrimary.SqrMagnitude());

	if (rMag == 0) return Vector3d::Zero();

	Vector3 const rDir = positionFromPrimary / rMag;
	float const rDotNormal = rDir.Dot(kReferenceNormal);

	if (abs(rDotNormal) > kParallelDotProductLimit)
	{
		/* Handle cases where the normal and position are parallel:
			* counter-clockwise around the reference Y-axis, whether above or below the plane */
		vDir = (rDotNormal > 0.f ? -kReferenceXd : kReferenceXd);
	}
	else
	{
		vDir = kReferenceNormald.Cross(rDir).Normalized();
	}

	return vDir * CircularOrbitSpeed(lspNode, rMag);
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3 OrbitalPhysics::ComputeLocalSeparation(ObjectNode fromObject, ObjectNode toObject)
{
	float const fromToRadiusRatio = static_cast<float>(
		toObject.ParentLsp().LSpace().MetersPerRadius / fromObject.ParentLsp().LSpace().MetersPerRadius);

	float fromLspLocalRadius = 1.f, toLspNonlocalRadius = 1.f;

	ObjectNode toParent = toObject.ParentObj(), fromParent = fromObject.ParentObj();
	Vector3 localFromOffset(fromObject.State().Position), nonlocalToOffset(toObject.State().Position);

	int heightDifference = toParent.Height() - fromParent.Height();
	while (heightDifference < 0)
	{
		fromLspLocalRadius /= fromObject.ParentLsp().LSpace().Radius;
		fromObject = fromParent;
		fromParent = fromObject.ParentObj();
		localFromOffset += fromObject.State().Position * fromLspLocalRadius;

		heightDifference += 2;
	}
	while (heightDifference > 0)
	{
		toLspNonlocalRadius /= toObject.ParentLsp().LSpace().Radius;
		toObject = toParent;
		toParent = toObject.ParentObj();
		nonlocalToOffset += toObject.State().Position * toLspNonlocalRadius;

		heightDifference -= 2;
	}

	while (fromParent != toParent)
	{
		fromLspLocalRadius /= fromObject.ParentLsp().LSpace().Radius;
		fromObject = fromParent;
		fromParent = fromObject.ParentObj();
		localFromOffset += fromObject.State().Position * fromLspLocalRadius;

		toLspNonlocalRadius /= toObject.ParentLsp().LSpace().Radius;
		toObject = toParent;
		toParent = toObject.ParentObj();
		nonlocalToOffset += toObject.State().Position * toLspNonlocalRadius;
	}

	Vector3 const localToOffset = nonlocalToOffset * fromToRadiusRatio;

	return localToOffset - localFromOffset;
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3 OrbitalPhysics::ComputeLocalPosition(LSpaceNode fromLsp, LSpaceNode toLsp, Vector3 toPosition)
{
	float fromToRadiusRatio = static_cast<float>(
		fromLsp.LSpace().MetersPerRadius / toLsp.LSpace().MetersPerRadius);

	float fromLspLocalRadius = 1.f, toLspNonlocalRadius = 1.f;

	Vector3 localFromOffset(0.f), nonlocalToOffset(toPosition);

	int heightDifference = toLsp.Height() - fromLsp.Height();
	while (heightDifference < 0)
	{
		fromLspLocalRadius /= fromLsp.LSpace().Radius;

		ObjectNode fromLspParent = fromLsp.ParentObj();
		localFromOffset += fromLspParent.State().Position * fromLspLocalRadius;

		fromLsp = fromLspParent.ParentLsp();
		heightDifference += 2;
	}
	while (heightDifference > 0)
	{
		toLspNonlocalRadius /= toLsp.LSpace().Radius;

		ObjectNode toLspParent = toLsp.ParentObj();
		nonlocalToOffset += toLspParent.State().Position * toLspNonlocalRadius;

		toLsp = toLspParent.ParentLsp();
		heightDifference -= 2;
	}

	while (fromLsp != toLsp)
	{
		fromLspLocalRadius /= fromLsp.LSpace().Radius;
		ObjectNode fromLspParent = fromLsp.ParentObj();
		localFromOffset += fromLspParent.State().Position * fromLspLocalRadius;
		fromLsp = fromLspParent.ParentLsp();

		toLspNonlocalRadius /= toLsp.LSpace().Radius;
		ObjectNode toLspParent = toLsp.ParentObj();
		nonlocalToOffset += toLspParent.State().Position * toLspNonlocalRadius;
		toLsp = toLspParent.ParentLsp();
	}

	Vector3 localToOffset = nonlocalToOffset * fromToRadiusRatio;

	return localToOffset - localFromOffset;
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3d OrbitalPhysics::ComputeLocalVelocity(Vector3d const objVelocity, LSpaceNode objLsp, LSpaceNode lsp)
{
	double const radiusRatio = objLsp.LSpace().MetersPerRadius / lsp.LSpace().MetersPerRadius;

	float objectLspNonlocalRadius = 1.f, lspLocalRadius = 1.f;

	Vector3d objNonlocalVelocity = objVelocity, lspLocalVelocity(0.0);

	int heightDifference = lsp.Height() - objLsp.Height();
	while (heightDifference < 0)
	{
		objectLspNonlocalRadius /= objLsp.LSpace().Radius;

		ObjectNode objLspParent = objLsp.ParentObj();
		objNonlocalVelocity += objLspParent.State().Velocity * static_cast<double>(objectLspNonlocalRadius);

		objLsp = objLspParent.ParentLsp();
		heightDifference += 2;
	}
	while (heightDifference > 0)
	{
		lspLocalRadius /= lsp.LSpace().Radius;

		ObjectNode lspParent = lsp.ParentObj();
		lspLocalVelocity += lspParent.State().Velocity * static_cast<double>(lspLocalRadius);

		lsp = lspParent.ParentLsp();
		heightDifference -= 2;
	}

	while (lsp != objLsp)
	{
		objectLspNonlocalRadius /= objLsp.LSpace().Radius;
		ObjectNode objLspParent = objLsp.ParentObj();
		objNonlocalVelocity += objLspParent.State().Velocity * static_cast<double>(objectLspNonlocalRadius);
		objLsp = objLspParent.ParentLsp();

		lspLocalRadius /= lsp.LSpace().Radius;
		ObjectNode lspParent = lsp.ParentObj();
		lspLocalVelocity += lspParent.State().Velocity * static_cast<double>(lspLocalRadius);
		lsp = lspParent.ParentLsp();
	}

	Vector3d objLocalVelocity = objNonlocalVelocity * radiusRatio;

	return objLocalVelocity - lspLocalVelocity;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::SolveMissileIntercept(ObjectNode missileObject, ObjectNode targetObject, double const acceleration,
	float const targetingTolerance, Vector3 & localIntercept, float & timeToIntercept, size_t const maxIterations)
{
	Vector3 const missilePosition = missileObject.GetState().Position;
	Vector3d const missileVelocity = missileObject.GetState().Velocity;
	LSpaceNode missileLsp = missileObject.ParentLsp(), targetLsp = targetObject.ParentLsp();
	Elements const& targetOrbitElements = targetObject.GetOrbit().Elements;
	float const targetTrueAnomaly = targetObject.GetMotion().TrueAnomaly;
	float trueAnomalyAtIntercept = targetTrueAnomaly;

	Vector3 separationVector = ComputeLocalSeparation(missileObject, targetObject);

	if (separationVector.IsZero() || (0.0 >= acceleration))
		return;

	float const targetingToleranceSqrd = targetingTolerance * targetingTolerance;
	float targetingDeltaSqrd; // variable to minimise
	size_t iteration = 0;
	do
	{
		float const separation = sqrtf(separationVector.SqrMagnitude());

		Vector3d const targetVelocity = targetOrbitElements.VelocityAt(trueAnomalyAtIntercept);
		Vector3d const initialRelativeVelocity = missileVelocity - ComputeLocalVelocity(targetVelocity, targetLsp, missileLsp);
		float const initialApproachSpeed = static_cast<float>(initialRelativeVelocity.Dot(Vector3d(separationVector.Normalized())));

		// Solve for time to target with constant acceleration
		static constexpr size_t kMaxNewtonIterations = 5;

		typedef std::function <float(float const)> F;
		F func = [=](float const t) -> float
		{
			return (0.5f * acceleration * t * t) + (initialApproachSpeed * t) - separation;
		};
		F func_1d = [=](float const t) -> float
		{
			return (acceleration * t) + initialApproachSpeed;
		};

		// Initial guess from suvat: v = sqrt(u^2 + 2as) --> t = 2s / (u + v)
		float const initialGuess = 2.f * separation /
			(initialApproachSpeed + sqrtf((initialApproachSpeed * initialApproachSpeed) + (2.f * acceleration * separation))); // very rough ballpark estimate

		float const timeTolerance = 0.01 * initialGuess; // 1% of initial guess

		timeToIntercept = SolveNetwon<float>(func, func_1d, initialGuess, timeTolerance, kMaxNewtonIterations); // low iteration count - favour speed over accuracy

		// Solve for target's actual position at solved time of intercept
		trueAnomalyAtIntercept = targetOrbitElements.SolveFinalTrueAnomaly(targetTrueAnomaly, timeToIntercept);
		localIntercept = ComputeLocalPosition(missileLsp, targetLsp, targetOrbitElements.PositionAt(trueAnomalyAtIntercept));

		// Compute targeting delta - the change in solved target position at estimated time of intercept
		Vector3 const newSeparationVector = localIntercept - missilePosition;

		targetingDeltaSqrd = (newSeparationVector - separationVector).SqrMagnitude();

		separationVector = newSeparationVector;

		++iteration;
	}
	while ((maxIterations > iteration) && (targetingToleranceSqrd < targetingDeltaSqrd));
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3d OrbitalPhysics::ComputeProportionalNavigationAcceleration(ObjectNode missileObject, ObjectNode targetObject, double const proportionalityConstant)
{
	State const& missileState = missileObject.GetState();

	Vector3 targetRelativePosition = ComputeLocalSeparation(missileObject, targetObject);
	Vector3d targetRelativeVelocity = ComputeLocalVelocity(targetObject, missileObject.ParentLsp()) - missileState.Velocity;
	Vector3d missileVelocityDirection = missileState.Velocity.Normalized();

	return ComputeProportionalNavigationAcceleration(targetRelativePosition, targetRelativeVelocity, missileVelocityDirection, proportionalityConstant);
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3d OrbitalPhysics::ComputeProportionalNavigationAcceleration(Vector3d const& targetRelativePosition, Vector3d const& targetRelativeVelocity,
	Vector3d const& missileVelocityDirection, double const proportionalityConstant)
{
	Vector3d targetRotationVector = targetRelativePosition.Cross(targetRelativeVelocity) / targetRelativePosition.SqrMagnitude();

	double targetRelativeVelocityMagnitude = sqrt(targetRelativeVelocity.SqrMagnitude());

	return -proportionalityConstant * targetRelativeVelocityMagnitude * missileVelocityDirection.Cross(targetRotationVector);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::SolveMissileInterceptVector(ObjectNode missileObject, ObjectNode targetObject, double localAcceleration, float targetingTolerance,
			Vector3 &interceptVector, Vector3 &interceptPosition, float &timeToIntercept, float proportionalityConstant, size_t maxIterations)
{
	SolveMissileIntercept(missileObject, targetObject, localAcceleration, targetingTolerance, interceptPosition, timeToIntercept, maxIterations);

	Vector3 const relativeIntercept = interceptPosition - missileObject.GetState().Position;

	Vector3 const proportionalNavigationAcceleration =
		Vector3(ComputeProportionalNavigationAcceleration(missileObject, targetObject, static_cast<double>(proportionalityConstant)));

	float const proportionalNavigationBias =
		std::clamp(sqrtf(proportionalNavigationAcceleration.SqrMagnitude()) / static_cast<float>(localAcceleration), 0.0f, 1.0f);

	interceptVector = ((1.0f - proportionalNavigationBias) * relativeIntercept.Normalized()) +
		(proportionalNavigationBias * proportionalNavigationAcceleration.Normalized());
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

constexpr OrbitalPhysics::ObjectNode::ObjectNode() :
	m_NodeId(OrbitalPhysics::NNull)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::ObjectNode::ObjectNode(ObjectNode const& rhs) :
	m_NodeId(rhs.m_NodeId)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::ObjectNode::ObjectNode(TNodeId const nodeId) :
	m_NodeId(nodeId)
{
	if (nodeId != OrbitalPhysics::NNull) {
		LV_ASSERT(m_pCtx->m_Tree.Has(nodeId), "Node ID ({}) not recognised.", nodeId);
		LV_ASSERT(m_pCtx->m_Tree.Height(nodeId) % 2 == 0, "Class is for object nodes only!");
		LV_ASSERT(m_pCtx->m_Objects.Has(nodeId), "Object node ({}) is missing Object attribute.", nodeId);
		LV_ASSERT(m_pCtx->m_States.Has(nodeId), "Object node ({}) is missing State attribute.", nodeId);
		LV_ASSERT(nodeId == kRootObjId || m_pCtx->m_Motions.Has(nodeId), "Object node ({}) is missing Motion attribute.", nodeId);
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::OrbitSection const& OrbitalPhysics::ObjectNode::GetOrbit(size_t const maxSections) const
{
	auto& state = m_pCtx->m_States[m_NodeId];
	auto& motion = m_pCtx->m_Motions[m_NodeId];

	if (motion.Orbit == IdNull)
	{
		motion.Orbit = NewOrbit(ParentLsp());
		ComputeOrbit(motion.Orbit, state.Position, state.Velocity, maxSections);
		motion.TrueAnomaly = Orbit().Elements.TrueAnomalyOf(state.Position.Normalized());
	}
	else if (motion.Integration == Motion::Integration::Linear)
	{
		motion.TrueAnomaly = Orbit().Elements.TrueAnomalyOf(state.Position.Normalized());
	}

	return m_pCtx->m_OrbitSections[motion.Orbit];
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3 OrbitalPhysics::ObjectNode::LocalPositionFromPrimary() const
{
	return m_pCtx->m_States[m_NodeId].Position +
		LSpaceNode(m_pCtx->m_Tree[m_NodeId].Parent).LocalOffsetFromPrimary();
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3d OrbitalPhysics::ObjectNode::LocalVelocityFromPrimary() const
{
	return m_pCtx->m_States[m_NodeId].Velocity +
		LSpaceNode(m_pCtx->m_Tree[m_NodeId].Parent).LocalVelocityFromPrimary();
}

// ---------------------------------------------------------------------------------------------------------------------------------

size_t OrbitalPhysics::ObjectNode::GetLocalSpaces(std::vector<LSpaceNode>& lspNodes) const
{
	size_t numChildren = 0;

	for (TNodeId child = m_pCtx->m_Tree[m_NodeId].FirstChild; OrbitalPhysics::NNull != child; child = m_pCtx->m_Tree[child].NextSibling)
	{
		lspNodes.emplace_back(child);
		numChildren++;
	}

	return numChildren;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ObjectNode::SetLocalSpace(LSpaceNode const newLspNode) const
{
	LV_ASSERT(!IsRoot() && !IsNull(), "Null or root object node ({}).", m_NodeId);
	LV_ASSERT(!newLspNode.IsNull(), "Invalid local space node ({}).", newLspNode.m_NodeId);

	m_pCtx->m_Tree.Move(m_NodeId, newLspNode.m_NodeId);

	TryPrepareObject(*this);
	TryPrepareSubtree(*this);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ObjectNode::SetMass(double const mass) const
{
	LV_ASSERT(!IsNull(), "Null object node ({}).", m_NodeId);

	m_pCtx->m_States[m_NodeId].Mass = mass;

	if (IsRoot())
	{
		auto& rootLsp = Object().Influence.LSpace();
		rootLsp.Grav = LocalGravitationalParameter(mass, rootLsp.MetersPerRadius);
		// TODO : exclude from release builds ?
	}

	TryPrepareObject(*this);
	TryPrepareSubtree(*this);
}

void OrbitalPhysics::ObjectNode::SetPosition(Vector3 const& position) const
{
	LV_ASSERT(!IsNull() && !IsRoot(), "Root or null object node ({}).", m_NodeId);

	m_pCtx->m_States[m_NodeId].Position = position;

	TryPrepareObject(*this);
	TryPrepareSubtree(*this);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ObjectNode::SetVelocity(Vector3d const& velocity) const
{
	LV_ASSERT(!IsNull() && !IsRoot(), "Root or null object node ({}).", m_NodeId);

	m_pCtx->m_States[m_NodeId].Velocity = velocity;

	TryPrepareObject(*this);
	TryPrepareSubtree(*this);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ObjectNode::SetDynamic(bool const isDynamic) const
{
	LV_ASSERT(!IsRoot(), "Root object node ({}).", m_NodeId);

	if (isDynamic)
		m_pCtx->m_Dynamics.GetOrAdd(m_NodeId);
	else
		m_pCtx->m_Dynamics.TryRemove(m_NodeId);

	TryPrepareObject(*this);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ObjectNode::SetContinuousAcceleration(Vector3d const& acceleration) const
{
	LV_ASSERT(IsDynamic(), "Object node ({}) does not have dynamic integration enabled.", m_NodeId);

	Dynamics().ContAcceleration = acceleration / ParentLsp().LSpace().MetersPerRadius;

	if (acceleration.IsZero())
		return;

	auto& motion = Motion();
	motion.Integration = Motion::Integration::Dynamic;
	motion.PrevDT -= motion.UpdateTimer;
	motion.UpdateTimer = 0.0;
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::ObjectNode::SetContinuousThrust(Vector3d const& thrust) const
{
	LV_ASSERT(IsDynamic(), "Cannot set dynamic acceleration on non-dynamic objects!");

	Vector3d acceleration = thrust / State().Mass;

	SetContinuousAcceleration(acceleration);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

constexpr OrbitalPhysics::LSpaceNode::LSpaceNode() :
	m_NodeId(OrbitalPhysics::NNull)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::LSpaceNode::LSpaceNode(LSpaceNode const& rhs) :
	m_NodeId(rhs.m_NodeId)
{
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::LSpaceNode::LSpaceNode(TNodeId const nodeId) :
	m_NodeId(nodeId)
{
	if (nodeId != OrbitalPhysics::NNull)
	{
		LV_ASSERT(m_pCtx->m_Tree.Has(nodeId), "Node ID ({}) not recognised.", nodeId);
		LV_ASSERT(m_pCtx->m_Tree.Height(nodeId) % 2 == 1, "ID belongs to an object node.");
		LV_ASSERT(m_pCtx->m_LSpaces.Has(nodeId), "Local space node ({}) is missing LocalSpace attribute.", nodeId);
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

size_t OrbitalPhysics::LSpaceNode::GetLocalObjects(std::vector<ObjectNode>& objNodes) const
{
	size_t numChildren = 0;

	for (TNodeId child = m_pCtx->m_Tree[m_NodeId].FirstChild; OrbitalPhysics::NNull != child; child = m_pCtx->m_Tree[child].NextSibling)
	{
		numChildren++;
		objNodes.emplace_back(child);
	}

	return numChildren;
}

// ---------------------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------------

Vector3 OrbitalPhysics::LSpaceNode::LocalOffsetFromPrimary() const
{
	return LocalOffsetFromPrimaryRecurs(m_NodeId, m_pCtx->m_LSpaces[m_NodeId].Primary.m_NodeId);
}

// -------------------------------------------------------------------------------------------------------------------------

Vector3d OrbitalPhysics::LSpaceNode::LocalVelocityFromPrimary() const
{
	return LocalVelocityFromPrimaryRecurs(m_NodeId, m_pCtx->m_LSpaces[m_NodeId].Primary.m_NodeId);
}

// -------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::LSpaceNode::SetRadius(float const radius) const
{
	LV_ASSERT(!IsSphereOfInfluence(), "Cannot set radius of sphere of influence.");

	SetRadiusImpl(radius);
}

// ---------------------------------------------------------------------------------------------------------------------------------

bool OrbitalPhysics::LSpaceNode::TrySetRadius(float const radius) const
{
	if (IsSphereOfInfluence())
	{
		LV_LOG("Attempted to set radius ({}) of sphere of influence (local space {} on object {}).",
			m_NodeId, ParentObj().m_NodeId, radius);

		return false;
	}
	else if ((kMaxLSpaceRadius < radius) || (kMinLSpaceRadius > radius))
	{
		LV_LOG("Attempted to set invalid radius {} (local space {} on object {}) - out of range [{}, {}].",
			radius, m_NodeId, ParentObj().m_NodeId, kMinLSpaceRadius, kMaxLSpaceRadius);

		return false;
	}

	SetRadiusImpl(radius);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3 OrbitalPhysics::LSpaceNode::LocalOffsetFromPrimaryRecurs(TNodeId const lspId, TNodeId const primaryLspId) const
{
	LV_ASSERT(Has(lspId) && (1 == (m_pCtx->m_Tree.Height(lspId) % 2)), "ID {} does not belong to a local space node.", lspId);
	LV_ASSERT(Has(primaryLspId) && (1 == (m_pCtx->m_Tree.Height(primaryLspId) % 2)), "ID {} does not belong to a local space node.", primaryLspId);

	if (lspId == primaryLspId)
		return Vector3::Zero();

	TNodeId lspParentObjId = m_pCtx->m_Tree[lspId].Parent;

	return ((m_pCtx->m_States[lspParentObjId].Position + LocalOffsetFromPrimaryRecurs(m_pCtx->m_Tree[lspParentObjId].Parent, primaryLspId)) /
		m_pCtx->m_LSpaces[lspId].Radius);
}

// ---------------------------------------------------------------------------------------------------------------------------------

Vector3d OrbitalPhysics::LSpaceNode::LocalVelocityFromPrimaryRecurs(TNodeId const lspId, TNodeId const primaryLspId) const
{
	LV_ASSERT(Has(lspId) && (1 == (m_pCtx->m_Tree.Height(lspId) % 2)), "ID {} does not belong to a local space node.", lspId);
	LV_ASSERT(Has(primaryLspId) && (1 == (m_pCtx->m_Tree.Height(primaryLspId) % 2)), "ID {} does not belong to a local space node.", primaryLspId);

	if (lspId == primaryLspId)
		return Vector3d::Zero();

	TNodeId lspParentObjId = m_pCtx->m_Tree[lspId].Parent;

	return ((m_pCtx->m_States[lspParentObjId].Velocity + LocalVelocityFromPrimaryRecurs(m_pCtx->m_Tree[lspParentObjId].Parent, primaryLspId)) /
		m_pCtx->m_LSpaces[lspId].Radius);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void OrbitalPhysics::LSpaceNode::SetRadiusImpl(float const radius) const
{
	OrbitalPhysics::Node const& node = m_pCtx->m_Tree[m_NodeId];
	LocalSpace& lsp = m_pCtx->m_LSpaces[m_NodeId];

	LV_ASSERT(m_NodeId != kRootLspId, "This function cannot set the radius of the root local space. See OrbitalPhysics::SetRootSpaceScaling()");
	LV_ASSERT(!(radius > kMaxLSpaceRadius) && !(radius < kMinLSpaceRadius), "Given radius ({}) is out of range: [{}, {}].",
		radius, kMinLSpaceRadius, kMaxLSpaceRadius);

	const float rescaleFactor = lsp.Radius / radius;

	const bool isSoi = IsSphereOfInfluence();
	const bool isInfluencing = (!ParentObj().Object().Influence.IsNull() &&
		!(radius > ParentObj().Object().Influence.LSpace().Radius));

	// Update local space attribute
	lsp.Radius = radius;
	lsp.MetersPerRadius = static_cast<double>(radius) * ((Height() == 1) ? GetRootLSpaceNode().LSpace().MetersPerRadius :
		m_pCtx->m_LSpaces[m_pCtx->m_Tree.GetGrandparent(m_NodeId)].MetersPerRadius);

	LV_ASSERT(lsp.MetersPerRadius > 1e-50, "Absolute scale is too small!");

	if (isSoi || isInfluencing)
		lsp.Primary = *this; /* an influencing space is its own Primary space */
	else
		lsp.Primary = ParentObj().PrimaryLsp(); /* a non-influencing space's Primary is that of its parent object */

	lsp.Grav = LocalGravitationalParameter(PrimaryObj().State().Mass, lsp.MetersPerRadius);

	// Move child objects to next-higher space if necessary.
	std::vector<ObjectNode> childObjs;
	GetLocalObjects(childObjs);

	LSpaceNode prevLspNode(node.PrevSibling);
	const bool promoteAll = (!prevLspNode.IsNull() && (radius > prevLspNode.LSpace().Radius));

	for (ObjectNode objNode : childObjs)
	{
		objNode.State().Position *= rescaleFactor;
		objNode.State().Velocity *= rescaleFactor;

		if (promoteAll || (kLocalSpaceEscapeRadius < sqrtf(objNode.State().Position.SqrMagnitude())))
		{
			PromoteObjectNode(objNode); /* "promoting" still works because we haven't yet re-sorted the local space amongst its siblings */
		}
		else
		{
			TryPrepareObject(objNode);
			TryPrepareSubtree(objNode.m_NodeId);
		}
	}

	// Resort the local space in its sibling linked-list
	if (rescaleFactor < 1.f)
	{
		// Radius increased: sort node left-wards
		while (!prevLspNode.IsNull())
		{
			if (!(radius > prevLspNode.LSpace().Radius))
				break;

			if (isSoi)
				prevLspNode.LSpace().Primary = prevLspNode; /* if resorting the sphere of influence, any smaller local spaces are now influencing */

			m_pCtx->m_Tree.SwapWithPrevSibling(m_NodeId);

			prevLspNode = LSpaceNode(node.PrevSibling);
		}
	}
	else
	{
		// Radius decreased: sort node right-wards
		LSpaceNode nextLspNode(node.NextSibling), parentPrimaryLspNode = ParentObj().PrimaryLsp();
		while (!nextLspNode.IsNull())
		{
			if (!(radius < nextLspNode.LSpace().Radius))
				break;

			if (isSoi)
				nextLspNode.LSpace().Primary = parentPrimaryLspNode; /* if resorting the sphere of influence, any larger local spaces are no longer influencing */

			m_pCtx->m_Tree.SwapWithNextSibling(m_NodeId);

			nextLspNode = LSpaceNode(node.NextSibling);
		}
	}

	// Local space ordering potentially changed
	CallChildLSpacesChangedCallback(ParentObj()); /* local space itself has been fully changed at this point - call relevant callback before changing local objects */

	// Child objects potentially moved
	for (ObjectNode objNode : childObjs)
	{
		if (*this != objNode.ParentLsp())
			CallParentLSpaceChangedCallback(objNode);
	}

	// Adopt any child objects from the new next-higher local space
	LSpaceNode nextHigherSpace = OuterLSpace();

	childObjs.clear();
	nextHigherSpace.GetLocalObjects(childObjs);

	const bool nextHigherIsSibling = nextHigherSpace.m_NodeId == node.PrevSibling;

	const float radiusInPrev = lsp.Radius / nextHigherSpace.LSpace().Radius;

	Vector3 const& lspPos = ParentObj().State().Position;

	for (ObjectNode objNode : childObjs)
	{
		if (objNode.m_NodeId == m_pCtx->m_Tree[m_NodeId].Parent)
			continue; /* skip parent object */

		if (nextHigherIsSibling && (radiusInPrev > sqrtf(objNode.State().Position.SqrMagnitude())))
		{
			DemoteObjectNode(objNode);

			CallParentLSpaceChangedCallback(objNode);
		}
		else if (!nextHigherIsSibling && (lsp.Radius > sqrtf((objNode.State().Position - lspPos).SqrMagnitude())))
		{
			DemoteObjectNode(*this, objNode);

			CallParentLSpaceChangedCallback(objNode);
		}
	}

	// Finally, update orbits with all local space changes
	if (isSoi)
		TryPrepareSubtree(ParentObj().m_NodeId); /* if SOI changed, update all sibling spaces */
	else
		TryPrepareSubtree(m_NodeId); /* otherwise, update only the objects in this local space */
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

Vector3 OrbitalPhysics::Elements::PositionAt(float const trueAnomaly) const
{
	Vector3 const directionAtTrueAnomaly = (cosf(trueAnomaly) * PerifocalX) + (sinf(trueAnomaly) * PerifocalY);
	return RadiusAt(trueAnomaly) * directionAtTrueAnomaly;
}

// ---------------------------------------------------------------------------------------------------------------------------------

float OrbitalPhysics::Elements::TrueAnomalyOf(Vector3 const& positionDirection) const
{
	LV_ASSERT((10.f * kEps) > abs(positionDirection.SqrMagnitude() - 1.f), "Direction vector must have unit length (was {0}, must be 1).",
		sqrtf(abs(positionDirection.SqrMagnitude())));

	float trueAnomaly = AngleBetweenUnitVectorsf(PerifocalX, positionDirection);

	if (positionDirection.Dot(PerifocalY) < 0.f)
		trueAnomaly = PI2f - trueAnomaly;

	return trueAnomaly;
}

// ---------------------------------------------------------------------------------------------------------------------------------

float OrbitalPhysics::Elements::ComputeTimeSincePeriapsis(float const trueAnomaly) const
{
	float meanAnomaly;

	if (E < 1.f)        // Elliptical
	{
		float eccentricAnomaly = 2.f * atanf(sqrtf((1.f - E) / (1.f + E)) * tanf(0.5f * trueAnomaly));

		if (eccentricAnomaly < 0.f)
			eccentricAnomaly += PI2f;

		meanAnomaly = eccentricAnomaly - (E * sinf(eccentricAnomaly));
	}
	else if (E > 1.f)   // Hyperbolic
	{
		float eccentricAnomaly = 2.f * atanhf(sqrtf((E - 1.f) / (E + 1.f)) * tanf(0.5f * trueAnomaly));

		meanAnomaly = E * sinhf(eccentricAnomaly) - eccentricAnomaly;
	}
	else                // Parabolic
	{
		meanAnomaly = (0.5f * tanf(0.5f * trueAnomaly)) + ((1.f / 6.f) * powf(tanf(0.5f * trueAnomaly), 3.f));
	}

	return meanAnomaly * static_cast<float>(T) * OverPI2f;
}

// ---------------------------------------------------------------------------------------------------------------------------------

float OrbitalPhysics::Elements::SolveTrueAnomaly(float const timeSincePeriapsis, float const tolerance, size_t const nMaxIterations) const
{
	float trueAnomaly;

	if (E < 1.f)        // Elliptical
	{
		float meanAnomaly = PI2f * timeSincePeriapsis / static_cast<float>(T);

		typedef std::function<float(float)> F;

		F f = [=](float const eccentricAnomaly) -> float { return eccentricAnomaly - E * sinf(eccentricAnomaly) - meanAnomaly; };
		F f_1d = [=](float const eccentricAnomaly) -> float { return 1.f - E * cosf(eccentricAnomaly); };

		float const eccentricAnomalyInitialGuess = meanAnomaly; // this relationship is true in circular orbits so it's a good place to start

		float const eccentricAnomaly = SolveNetwon<float>(f, f_1d, eccentricAnomalyInitialGuess, tolerance, nMaxIterations);

		trueAnomaly = 2.f * atanf(tanf(0.5f * eccentricAnomaly) / sqrtf((1.f - E) / (1.f + E)));
	}
	else if (E > 1.f)   // Hyperbolic
	{
		float meanAnomaly = MConstant * powf((E * E) - 1.f, 1.5f) * timeSincePeriapsis;

		typedef std::function<float(float)> F;

		F f = [=](float const eccentricAnomaly) -> float { return E * sinhf(eccentricAnomaly) - eccentricAnomaly - meanAnomaly; };
		F f_1d = [=](float const eccentricAnomaly) -> float { return E * coshf(eccentricAnomaly) - 1.f; };

		float const mLog10 = log10f(meanAnomaly);
		float const eccentricAnomalyInitialGuess = std::max(1.f, 2.f * mLog10);
		float const eccentricAnomaly = SolveNetwon<float>(f, f_1d, eccentricAnomalyInitialGuess, tolerance, nMaxIterations);

		trueAnomaly = 2.f * atanf(tanhf(0.5f * eccentricAnomaly) / sqrtf((E - 1.f) / (E + 1.f)));
	}
	else                // Parabolic
	{
		float const meanAnomaly = MConstant * timeSincePeriapsis;

		float const meanAnomalyFactor = cbrtf(3.f * meanAnomaly + sqrtf(1.f + (9.f * meanAnomaly * meanAnomaly)));

		trueAnomaly = 2.f * atanf(meanAnomalyFactor - (1.f / meanAnomalyFactor));
	}

	if (trueAnomaly < 0.f)
		trueAnomaly += PI2f;

	return trueAnomaly;
}

// ---------------------------------------------------------------------------------------------------------------------------------

float OrbitalPhysics::Elements::SolveFinalTrueAnomaly(float const initialTrueAnomaly, float const timeSeparation) const
{
	float initialTimeSincePeriapsis = ComputeTimeSincePeriapsis(initialTrueAnomaly);

	float finalTimeSincePeriapsis = Wrapf(initialTimeSincePeriapsis + timeSeparation, T);

	return SolveTrueAnomaly(finalTimeSincePeriapsis);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::Context::Context()
{
	m_Tree.New(); /* kRootObjId (0) */
	m_Tree.New(); /* kRootLspId (1) */

	LV_ASSERT(m_Tree.Has(kRootObjId), "Context failed to create root object node.");
	LV_ASSERT(m_Tree.Has(kRootLspId), "Context failed to create root local space node.");

	Object& rootObj = m_Objects.Add(kRootObjId);
	rootObj.Validity = Validity::InvalidParent; /* use InvalidParent to signify that root SCALING has not been set, instead of making a unique enum value */
	rootObj.Influence.m_NodeId = kRootLspId;
	m_States.Add(kRootObjId);

	LocalSpace& rootLsp = m_LSpaces.Add(kRootLspId);
	rootLsp.Radius = 1.f;
	rootLsp.MetersPerRadius = 1.0;
	rootLsp.Primary.m_NodeId = kRootLspId; /* an influencing lsp is its own primary */
}

// ---------------------------------------------------------------------------------------------------------------------------------

OrbitalPhysics::Context::~Context()
{
	// Estimating required memory allocation for converting vectors to static arrays
	LV_LOG("OrbitalPhysics final tree size: {0} ({1} objects, {2} local spaces)", m_Tree.Size(), m_Objects.Size(), m_LSpaces.Size());
}

// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------

}

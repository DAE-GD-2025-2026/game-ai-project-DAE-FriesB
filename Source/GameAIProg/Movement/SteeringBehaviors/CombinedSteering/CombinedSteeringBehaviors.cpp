#include "CombinedSteeringBehaviors.h"
#include <algorithm>
#include "../SteeringAgent.h"

BlendedSteering::BlendedSteering(const std::vector<WeightedBehavior>& WeightedBehaviors)
	: WeightedBehaviors(WeightedBehaviors)
{
};

//****************
//BLENDED STEERING
SteeringOutput BlendedSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput BlendedSteering = {};
	// TODO: Calculate the weighted average steeringbehavior
	float sum = 0.f;
	for (auto& weightedbehavior : WeightedBehaviors)
	{
		sum += weightedbehavior.Weight;
	}
	if (sum == 0.f)
	{
		return BlendedSteering;
	}

	float weightDenom = 1.f / sum;
	for (auto& weightedBehavior : WeightedBehaviors)
	{
		if (weightedBehavior.Weight <= 0.f)
		{
			continue;
		}

		SteeringOutput partialSteering = weightedBehavior.pBehavior->CalculateSteering(DeltaT, Agent);
		BlendedSteering.AngularVelocity += partialSteering.AngularVelocity * weightDenom * weightedBehavior.Weight;
		BlendedSteering.LinearVelocity += partialSteering.LinearVelocity * weightDenom * weightedBehavior.Weight;
	}
	// TODO: Add debug drawing
	if (Agent.GetDebugRenderingEnabled())
	{
		DrawDebugDirectionalArrow(Agent.GetWorld(), FVector{Agent.GetPosition(), 0},
		                          FVector{Agent.GetPosition() + BlendedSteering.LinearVelocity, 0}, 3.f, FColor::Green);
	}

	return BlendedSteering;
}

float* BlendedSteering::GetWeight(ISteeringBehavior* const SteeringBehavior)
{
	auto it = find_if(WeightedBehaviors.begin(),
	                  WeightedBehaviors.end(),
	                  [SteeringBehavior](const WeightedBehavior& Elem)
	                  {
		                  return Elem.pBehavior == SteeringBehavior;
	                  }
	);

	if (it != WeightedBehaviors.end())
		return &it->Weight;

	return nullptr;
}

//*****************
//PRIORITY STEERING
SteeringOutput PrioritySteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering = {};

	for (ISteeringBehavior* const pBehavior : m_PriorityBehaviors)
	{
		Steering = pBehavior->CalculateSteering(DeltaT, Agent);

		if (Steering.IsValid)
			break;
	}

	//If non of the behavior return a valid output, last behavior is returned
	return Steering;
}

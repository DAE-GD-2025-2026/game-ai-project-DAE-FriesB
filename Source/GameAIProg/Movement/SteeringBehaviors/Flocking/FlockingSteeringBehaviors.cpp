#include "FlockingSteeringBehaviors.h"
#include "Flock.h"
#include "../SteeringAgent.h"
#include "../SteeringHelpers.h"
#include <numeric>

//*******************
//COHESION (FLOCKING)
SteeringOutput Cohesion::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	Target = FTargetData{pFlock->GetAverageNeighborPos()};
	return Seek::CalculateSteering(deltaT, pAgent);
}

//*********************
//SEPARATION (FLOCKING)
SteeringOutput Separation::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	
	//inversely proportional
	SteeringOutput Output{};
	const int NrOfNeighbors = pFlock->GetNrOfNeighbors();
	
	if (NrOfNeighbors == 0)
	{
		Output.LinearVelocity = pAgent.GetLinearVelocity();	
		return Output; 
	}
	

	const auto& Neighbors = pFlock->GetNeighbors();
	const FVector2D AgentPos = pAgent.GetPosition();
	
	FVector2D OutputVelocity{FVector2D::ZeroVector};
	
	for (int idx{0}; idx < NrOfNeighbors; ++idx)
	{
		const FVector2D ToAgent = AgentPos - Neighbors[idx]->GetPosition();
		
		//Inverse proportional to distance
        FVector2D PushForce = ToAgent;
        PushForce /= PushForce.SquaredLength();
		
        OutputVelocity += PushForce;
	}
	
	OutputVelocity.Normalize();
	Output.LinearVelocity = OutputVelocity;

	return Output;
}

//*************************
//VELOCITY MATCH (FLOCKING)
SteeringOutput VelocityMatch::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput output{};
	if (pFlock->GetNrOfNeighbors() == 0)
	{
		output.LinearVelocity = pAgent.GetLinearVelocity();
	}
	else
	{
		output.LinearVelocity = pFlock->GetAverageNeighborVelocity();
	}
	output.LinearVelocity.Normalize();
	return output;
}

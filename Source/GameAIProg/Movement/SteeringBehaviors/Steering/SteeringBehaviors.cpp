#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

//SEEK
//*******
// TODO: Do the Week01 assignment :^)
SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Output{};
	FVector2D Offset = Target.Position - Agent.GetPosition();
	Offset.Normalize();
	Output.LinearVelocity = Offset;
	if (Agent.GetDebugRenderingEnabled())
	{
		DrawDebugDirectionalArrow(Agent.GetWorld(), 
			                      FVector{Agent.GetPosition(), 0}, 
			                      FVector{Agent.GetPosition()+Offset*100, 0}, 
			                      1.f,
		                          FColor::Green);
	}
	return Output;
}

//WANDER (base> SEEK)
//*******
SteeringOutput Wander::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	// ANGLE ON CIRCLE IMPLEMENTATION
	// ------------------------------
	//Calculate WanderOffset
	FVector2D offset = Agent.GetLinearVelocity();
	offset.Normalize();
	offset *= m_OffsetDistance;

	//Change the WanderAngle slightly for next frame
	m_WanderAngle += UKismetMathLibrary::RandomFloat() * m_MaxAngleChange - (m_MaxAngleChange * .5f);
	//RAND[-angleChange/2,angleChange/2]

	//WanderCircle Offset (Polar to Cartesian Coordinates)
	FVector2D circleOffset = {cos(m_WanderAngle) * m_Radius, sin(m_WanderAngle) * m_Radius};

	//Set target as Seek::Target
	Target = FTargetData{Agent.GetPosition() + offset + circleOffset};

	//DEBUG RENDERING
	if (Agent.GetDebugRenderingEnabled())
	{
		auto pos = Agent.GetActorLocation();

		FTransform circlePos{
			FVector::UpVector.ToOrientationRotator(),
			Agent.GetActorLocation() + FVector{offset, 0.0f}
		};

		circlePos.AddToTranslation(FVector{offset, 0});
		DrawDebugLine(Agent.GetWorld(), pos, pos + FVector{offset, 0}, FColor::Red);
		DrawDebugCircle(Agent.GetWorld(), circlePos.ToMatrixNoScale(), m_Radius, 8, FColor::Blue);
		// DrawDebugSphere(Agent.GetWorld(), pos + FVector{offset, 0}, m_Radius, 8, FColor::Blue);
		DrawDebugPoint(Agent.GetWorld(), pos + FVector{offset, 0} + FVector{circleOffset, 0}, 10.f, FColor::Green);
	}

	//// OFFSET POINT IMPLEMENTATION
	//// ---------------------------
	//// Add a random offset to the current wander target (in circle space)
	//const float halfJitter = m_MaxJitterDistance / 2;
	//const FVector2D randomOffset = FVector2D{ RandomFloat(-halfJitter, halfJitter), RandomFloat(-halfJitter, halfJitter) };
	//m_WanderTarget += randomOffset;

	//// Project the point back onto the circle
	//m_WanderTarget.Normalize();
	//m_WanderTarget *= m_Radius;

	//// Displace the point on the circle in front of the agent
	//FVector2D offset = Agent.GetLinearVelocity();
	//offset.Normalize();
	//offset *= m_OffsetDistance;

	//m_Target = TargetData(Agent.GetPosition() + offset + m_WanderTarget);

	// TODO implement debug render
	////DEBUG RENDERING
	//if (Agent.GetDebugRenderingEnabled())
	//{
	//	auto pos = Agent.GetPosition();
	//	DEBUGRENDERER2D->DrawSegment(pos, pos + offset, { 0,0,1 ,0.5f});
	//	DEBUGRENDERER2D->DrawCircle(pos + offset, m_Radius, { 0,0,1 ,0.5f }, -0.7f);
	//	DEBUGRENDERER2D->DrawSolidCircle(pos + offset + m_WanderTarget, 0.1f, { 0,0 }, { 0, 1, 0 ,0.5f }, -0.75f);
	//}

	//Return Seek Output (with our wander target)
	return Seek::CalculateSteering(DeltaT, Agent);
}

//PURSUIT (base> SEEK)
//*******
SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	const float distanceToTarget = FVector2D::Distance(Agent.GetPosition(), Target.Position);
	const float timeToTarget = distanceToTarget / Agent.GetMaxLinearSpeed();
	const FVector2D targetOffset = Target.LinearVelocity * timeToTarget;
	Target = FTargetData{Target.Position + targetOffset};

	//DEBUG RENDERING
	if (Agent.GetDebugRenderingEnabled())
		DrawDebugPoint(Agent.GetWorld(), FVector{Target.Position, 90.f}, 10.f, FColor::Yellow);

	return Seek::CalculateSteering(DeltaT, Agent);
}

//FLEE
//****
SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering = {};
	steering.LinearVelocity = Agent.GetPosition() - Target.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= Agent.GetMaxLinearSpeed(); //Rescale to Max Speed

	//DEBUG RENDERING
	if (Agent.GetDebugRenderingEnabled())
	{
		DrawDebugDirectionalArrow(
			Agent.GetWorld(),
			Agent.GetActorLocation(),
			Agent.GetActorLocation() + FVector{steering.LinearVelocity, 0} * (Agent.GetMaxLinearSpeed() * DeltaT),
			30.f, FColor::Blue);
	}

	return steering;
}

//EVADE (base> FLEE)
//*****
Evade::Evade(float evasionRadius /* = 15.f */)
	: m_EvasionRadius(evasionRadius)
{
}

SteeringOutput Evade::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	const float distanceToTarget = FVector2D::Distance(Agent.GetPosition(), Target.Position);

	if (distanceToTarget > m_EvasionRadius)
	{
		SteeringOutput steering;
		steering.IsValid = false;
		return steering;
	}

	const float lookAheadTime = distanceToTarget / (Target.LinearVelocity.Length() + Agent.GetLinearVelocity().
		Length());
	Target = FTargetData{Target.Position + Target.LinearVelocity * lookAheadTime};

	//DEBUG RENDERING
	if (Agent.GetDebugRenderingEnabled())
		DrawDebugPoint(Agent.GetWorld(), FVector{Target.Position, 90.f}, 10.f, FColor::Yellow);

	return Flee::CalculateSteering(DeltaT, Agent);
}

//ARRIVE
//******
SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering = {};

	steering.LinearVelocity = Agent.GetPosition() - Target.Position; //Total_Velocity to Target
	const float distance = steering.LinearVelocity.Normalize() - m_TargetRadius;
	//Total_Distance to Target + Normalize Total_Velocity 

	if (distance < m_SlowRadius) //Inside SlowRadius
		steering.LinearVelocity *= Agent.GetMaxLinearSpeed() * (distance / (m_SlowRadius + m_TargetRadius));
		//Slow Downs
	else
		steering.LinearVelocity *= Agent.GetMaxLinearSpeed(); //Move to target at max speed

	//DEBUG RENDERING
	if (Agent.GetDebugRenderingEnabled())
	{
		const FVector2D pos = Agent.GetPosition();
		FTransform circlePos{FVector::UpVector.ToOrientationRotator(), Agent.GetActorLocation()};
		DrawDebugCircle(Agent.GetWorld(), circlePos.ToMatrixNoScale(), m_SlowRadius, 8, FColor::Green);
		DrawDebugCircle(Agent.GetWorld(), circlePos.ToMatrixNoScale(), m_TargetRadius, 8, FColor::Red);

		DrawDebugDirectionalArrow(
			Agent.GetWorld(),
			Agent.GetActorLocation(),
			Agent.GetActorLocation() + FVector{steering.LinearVelocity, 0} * (Agent.GetMaxLinearSpeed() * DeltaT),
			30.f, FColor::Blue);
	}

	return steering;
}
//FACE
//*******

SteeringOutput Face::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput output{};
	FVector2D offset = Target.Position - Agent.GetPosition();
	float targetAngle = atan2(offset.Y, offset.X)*180.f/PI;

	auto controller = Agent.GetController();
	float currentAngle = controller->GetControlRotation().Yaw;
	
	
	
	float deltaAngle = targetAngle - currentAngle;
	if (deltaAngle < -180.f)
	{
		deltaAngle += 360.0f;
	}
	currentAngle += deltaAngle;
	
	controller->SetControlRotation(FRotator{0,currentAngle,0});
	
	DrawDebugDirectionalArrow(
			Agent.GetWorld(),
			Agent.GetActorLocation(),
			Agent.GetActorLocation() + Agent.GetActorForwardVector()*100.f,
			30.f, FColor::Blue);
	DrawDebugDirectionalArrow(
			Agent.GetWorld(),
			Agent.GetActorLocation(),
			Agent.GetActorLocation() + FVector{offset*100.f,0},
			30.f, FColor::Red);
	DrawDebugLine(
		Agent.GetWorld(), 
		Agent.GetActorLocation(), 
		FVector{Target.Position,0}, 
		FColor::Yellow);
	return output;
}

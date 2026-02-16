#pragma once

#include <Movement/SteeringBehaviors/SteeringHelpers.h>
#include "Kismet/KismetMathLibrary.h"

class ASteeringAgent;

// SteeringBehavior base, all steering behaviors should derive from this.
class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	// Override to implement your own behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) = 0;

	void SetTarget(const FTargetData& NewTarget) { Target = NewTarget; }
	
	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
	T* As()
	{ return static_cast<T*>(this); }

protected:
	FTargetData Target;
};

// Your own SteeringBehaviors should follow here...
class Seek: public ISteeringBehavior
{
public:
	Seek() = default;
	virtual ~Seek() = default;
	
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
	
	
};

class Wander : public Seek
{
public:
	Wander() = default;
	virtual ~Wander() override = default;

	//Wander Behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) override;

	void SetWanderOffset(float offset) { m_OffsetDistance = offset; }
	void SetWanderRadius(float radius) { m_Radius = radius; }
	void SetMaxAngleChange(float rad) { m_MaxAngleChange = rad; }

protected:
	float m_OffsetDistance = 60.f; //Offset (Agent Direction)
	float m_Radius = 40.f; //WanderRadius
	float m_MaxAngleChange = UKismetMathLibrary::DegreesToRadians(45.f); //Max WanderAngle change per frame
	float m_WanderAngle = 0.f; //Internal

	FVector2D m_WanderTarget;
	float m_MaxJitterDistance = 1.f;
};

class Pursuit : public Seek
{
public:
	Pursuit() = default;
	virtual ~Pursuit() override = default;

	//Pursuit Behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) override;
};

class Flee : public ISteeringBehavior
{
public:
	Flee() = default;
	virtual ~Flee() override = default;

	//Seek Behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) override;
};

class Evade : public Flee
{
public:
	Evade(float evasionRadius = 500.f);
	virtual ~Evade() override = default;

	//Evade Behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) override;

	void SetEvasionRadius(float evasionRadius) { m_EvasionRadius = evasionRadius; }
private:
	float m_EvasionRadius; // only evades agent within this range
};

class Arrive : public ISteeringBehavior
{
public:
	Arrive() = default;
	virtual ~Arrive() override = default;

	//Arrive Behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) override;

	//Arrive Functions
	void SetSlowRadius(float radius) { m_SlowRadius = radius; }
	void SetTargetRadius(float radius) { m_TargetRadius = radius; }

protected:

	float m_SlowRadius = 300.f;
	float m_TargetRadius = 50.f;
};
#include "Flock.h"

#include "FlockingSteeringBehaviors.h"
#include "Shared/ImGuiHelpers.h"

Flock::Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize,
	float WorldSize,
	ASteeringAgent* const pAgentToEvade,
	bool bTrimWorld)
	: pWorld{pWorld}
	  , FlockSize{FlockSize}
	  , pAgentToEvade{pAgentToEvade}
{
	Agents.SetNum(FlockSize);

	// TODO: initialize the flock and the memory pool
	pSeparationBehavior = std::make_unique<Separation>(this);
	pCohesionBehavior = std::make_unique<Cohesion>(this);
	pVelMatchBehavior = std::make_unique<VelocityMatch>(this);
	pSeekBehavior = std::make_unique<Seek>();
	pWanderBehavior = std::make_unique<Wander>();

	pBlendedSteering = std::make_unique<BlendedSteering>(
		std::vector<BlendedSteering::WeightedBehavior>{
			{pSeparationBehavior.get(), .2f},
			{pCohesionBehavior.get(), .2f},
			{pVelMatchBehavior.get(), .2f},
			{pSeekBehavior.get(), .2f},
			{pWanderBehavior.get(), .2f}
		}
	);

	ISteeringBehavior* pBoidSteering = pBlendedSteering.get();
	if (pAgentToEvade)
	{
		pEvadeBehavior = std::make_unique<Evade>();
		FSteeringParams evadeTarget{pAgentToEvade->GetPosition()};
		pEvadeBehavior->SetTarget(evadeTarget);
		pPrioritySteering = std::make_unique<PrioritySteering>(
			std::vector<ISteeringBehavior*>{
				pEvadeBehavior.get(),
				pBlendedSteering.get()
			});
		pBoidSteering = pPrioritySteering.get();
	}
	
#ifdef GAMEAI_USE_SPACE_PARTITIONING
	OldPositions.SetNum(FlockSize);
	pPartitionedSpace = std::make_unique<CellSpace>(pWorld, WorldSize*2, WorldSize*2, NrOfCellsX, NrOfCellsX, FlockSize);
#else
	Neighbors.SetNum(FlockSize);
#endif

	
	
	//Spawn agents
	for (int i = 0; i < FlockSize; ++i)
	{
		FVector2D randomPos{
			FMath::RandRange(-WorldSize / 2, WorldSize / 2), FMath::RandRange(-WorldSize / 2, WorldSize / 2)
		};
		FRotator randomRot{0.f, FMath::RandRange(0.f, 360.f), 0.f};
		
		FActorSpawnParameters params{};
		params.bNoFail = true;
		params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		
		if (ASteeringAgent* NewAgent{pWorld->SpawnActor<ASteeringAgent>(AgentClass, FVector{randomPos,90}, randomRot,params )})
		{
			NewAgent->SetActorTickEnabled(false);
			NewAgent->SetSteeringBehavior(pBoidSteering);
			NewAgent->SetDebugRenderingEnabled(false);
			

			Agents[i] = NewAgent;
			
		}
		else
		{
			--i;
		}
	}
	

}

Flock::~Flock()
{
	// TODO: Cleanup any additional data
	for (ASteeringAgent* pAgent : Agents)
	{
		pAgent->SetSteeringBehavior(nullptr);
	}
	Agents.Empty();
	
#ifndef GAMEAI_USE_SPACE_PARTITIONING
	Neighbors.Empty();
#endif
}

void Flock::Tick(float DeltaTime)
{
	// TODO: update the flock
	// TODO: for every agent:
	// TODO: register the neighbors for this agent (-> fill the memory pool with the neighbors for the currently evaluated agent)
	// TODO: update the agent (-> the steeringbehaviors use the neighbors in the memory pool)
	// TODO: trim the agent to the world
	if (pAgentToEvade)
	{
		FSteeringParams evadeTarget{pAgentToEvade->GetPosition()};
		pEvadeBehavior->SetTarget(evadeTarget);
	}

	const int size = Agents.Num();
	for (int i = 0; i < size; ++i)
	{
		ASteeringAgent* pAgent = Agents[i];
#ifdef GAMEAI_USE_SPACE_PARTITIONING
		pPartitionedSpace->RegisterNeighbors(*pAgent, NeighborhoodRadius);
		pPartitionedSpace->UpdateAgentCell(*pAgent, OldPositions[i]);
		
		OldPositions[i]= pAgent->GetPosition();
#else
		RegisterNeighbors(pAgent);
#endif
		pAgent->Tick(DeltaTime);
	}
}

void Flock::RenderDebug()
{
	// TODO: Render all the agents in the flock
#ifdef GAMEAI_USE_SPACE_PARTITIONING
	if (DebugRenderPartitions)
		pPartitionedSpace->RenderCells();
#endif // USE_SPACE_PARTITIONING
	if (DebugRenderNeighborhood)
		RenderNeighborhood();
	// #StudentEntry end
}

void Flock::ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize)
{
#ifdef PLATFORM_WINDOWS
#pragma region UI
	//UI
	{
		//Setup
		bool bWindowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &bWindowActive,
		             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: place target");
		ImGui::Text("RMB: move cam.");
		ImGui::Text("Scrollwheel: zoom cam.");
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Flocking");
		ImGui::Spacing();

		// TODO: implement ImGUI checkboxes for debug rendering here
		if (ImGui::Checkbox("Render first agent debug", &DebugRenderSteering))
		{
			Agents[0]->SetDebugRenderingEnabled(DebugRenderSteering);
		}

		ImGui::Checkbox("Render first agent neighborhood", &DebugRenderNeighborhood);
		ImGui::Spacing();
		bool bCurrentDebugRenderPartitions = DebugRenderPartitions;
		if (ImGui::Checkbox("Debug render partitions", &bCurrentDebugRenderPartitions))
		{
			DebugRenderPartitions = bCurrentDebugRenderPartitions;
		}
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Text("Behavior Weights");
		ImGui::Spacing();

		// TODO: implement ImGUI sliders for steering behavior weights here

		ImGui::SliderFloat("Separation", &pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Cohesion", &pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("VelocityMatch", &pBlendedSteering->GetWeightedBehaviorsRef()[2].Weight, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Seek", &pBlendedSteering->GetWeightedBehaviorsRef()[3].Weight, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Wander", &pBlendedSteering->GetWeightedBehaviorsRef()[4].Weight, 0.0f, 1.0f, "%.2f");


		//End
		ImGui::End();
	}
#pragma endregion
#endif
}

void Flock::RenderNeighborhood()
{
	if (!DebugRenderNeighborhood)return;

	FTransform circlePos{FVector::UpVector.ToOrientationRotator(), Agents[0]->GetActorLocation()};
	DrawDebugCircle(pWorld, circlePos.ToMatrixNoScale(), NeighborhoodRadius, 32, FColor::Green);
}

#ifndef GAMEAI_USE_SPACE_PARTITIONING
void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
	if (FlockSize <= 0)return;
	NrOfNeighbors = 0;

	auto agentPos = pAgent->GetPosition();
	for (int i = 0; i < FlockSize; ++i)
	{
		ASteeringAgent* const pOther = Agents[i];

		if (pOther == pAgent)continue; //skip self
		
		//const float distanceSq = (agentPos - pOther->GetPosition()).SquaredLength();
		const float distanceSq = (agentPos - pOther->GetPosition()).SquaredLength();

		if ( distanceSq > NeighborhoodRadius*NeighborhoodRadius )
			continue;

		Neighbors[NrOfNeighbors++] = pOther;
	}
}
#endif

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;

	int numNeighbors = GetNrOfNeighbors();
	if (numNeighbors == 0)return avgPosition;
	float neighborsDenom = 1.f / numNeighbors;
    
	for (int idx = 0; idx < numNeighbors; ++idx)
	{
		avgPosition += GetNeighbors()[idx]->GetPosition() * neighborsDenom;
	}

	return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;

	int numNeighbors = GetNrOfNeighbors();
	if (numNeighbors == 0)return avgVelocity;

	float neighborsDenom = 1.f / numNeighbors;

	for (int idx = 0; idx < numNeighbors; ++idx)
	{
		avgVelocity += GetNeighbors()[idx]->GetLinearVelocity() * neighborsDenom;
	}


	return avgVelocity;
}

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{
	pSeekBehavior->SetTarget(Target);
}

#include "Level_CombinedSteering.h"

#include "imgui.h"


// Sets default values
ALevel_CombinedSteering::ALevel_CombinedSteering()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_CombinedSteering::BeginPlay()
{
	Super::BeginPlay();
	DrunkAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{0, 0, 90}, FRotator::ZeroRotator);

	//Create blended steering
	pDrunkSeek = new Seek();
	pDrunkWander = new Wander();
	pDrunkWanderSteering = new BlendedSteering({
		{pDrunkSeek, 0.5f},
		{pDrunkWander, 0.5f}
	});
	pDrunkSeek->SetTarget(MouseTarget);
	DrunkAgent->SetSteeringBehavior(pDrunkWanderSteering);
	
	//evading agent
	
	pEvadingWander = new Wander();
	pEvade = new Evade();
	pPrioritySteering = new	PrioritySteering({pEvade,pEvadingWander});
	EvadingAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{0,100,90}, FRotator::ZeroRotator);
	EvadingAgent->SetSteeringBehavior(pPrioritySteering);
	
}

void ALevel_CombinedSteering::BeginDestroy()
{
	Super::BeginDestroy();
	
	delete pDrunkSeek;
	delete pDrunkWander;
	delete pDrunkWanderSteering;
	
	delete pEvade;
	delete pEvadingWander;
	delete pPrioritySteering;
	
}

// Called every frame
void ALevel_CombinedSteering::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	

#pragma region UI
	//UI
	{
		//Setup
		bool windowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Game AI", &windowActive,
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
		ImGui::Spacing();

		ImGui::Text("Blended steering params");

		ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",
		                                         pDrunkWanderSteering->GetWeightedBehaviorsRef()[0].Weight, 0.f, 1.f,
		                                         [this](float val)
		                                         {
			                                         pDrunkWanderSteering->GetWeightedBehaviorsRef()[0].Weight = val;
		                                         });
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",
											 pDrunkWanderSteering->GetWeightedBehaviorsRef()[1].Weight, 0.f, 1.f,
											 [this](float val)
											 {
												 pDrunkWanderSteering->GetWeightedBehaviorsRef()[1].Weight = val;
											 });
		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::Checkbox("Debug Rendering", &CanDebugRender))
		{
			// TODO: Handle the debug rendering of your agents here :)
		}
		ImGui::Checkbox("Trim World", &TrimWorld->bShouldTrimWorld);
		if (TrimWorld->bShouldTrimWorld)
		{
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Trim Size",
			                                         TrimWorld->GetTrimWorldSize(), 1000.f, 3000.f,
			                                         [this](float InVal) { TrimWorld->SetTrimWorldSize(InVal); });
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Behavior Weights");
		ImGui::Spacing();


		// ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",
		// 	pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight, 0.f, 1.f,
		// 	[this](float InVal) { pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight = InVal; }, "%.2f");
		//
		// ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",
		// pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight, 0.f, 1.f,
		// [this](float InVal) { pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight = InVal; }, "%.2f");

		//End
		ImGui::End();
	}
#pragma endregion

	DrunkAgent->Tick(DeltaTime);
	
	pDrunkSeek->SetTarget(FTargetData{MouseTarget});
	// Combined Steering Update
	// TODO: implement handling mouse click input for seek
	// TODO: implement Make sure to also evade the wanderer
	FTargetData EvadeTarget{};
	EvadeTarget.LinearVelocity = DrunkAgent->GetLinearVelocity();
	EvadeTarget.Position = DrunkAgent->GetPosition();
	
	pEvade->SetTarget(EvadeTarget);
}

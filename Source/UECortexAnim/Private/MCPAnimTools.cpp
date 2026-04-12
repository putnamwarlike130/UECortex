#include "MCPAnimTools.h"
#include "MCPToolRegistry.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationStateGraph.h"
#include "AnimationStateGraphSchema.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "AnimStateEntryNode.h"
#include "Animation/BlendSpace.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"
#include "Dom/JsonObject.h"

// ---------------------------------------------------------------------------
// NOTE: HTTP handlers are called from the game thread tick (FHttpServerModule::ProcessRequests).
// Do NOT use AsyncTask(GameThread) + FEvent — deadlocks. Call UE APIs directly.
// ---------------------------------------------------------------------------

void FMCPAnimTools::RegisterTools(TArray<FMCPToolDef>& OutTools)
{
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_list_skeletons");
		T.Description = TEXT("List Skeleton assets in the project.");
		T.Params = { { TEXT("filter"), TEXT("string"), TEXT("Optional substring filter on asset name"), false } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimListSkeletons(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_list_anim_blueprints");
		T.Description = TEXT("List Animation Blueprint assets in the project.");
		T.Params = {
			{ TEXT("filter"),        TEXT("string"), TEXT("Optional substring filter on asset name"), false },
			{ TEXT("skeleton_path"), TEXT("string"), TEXT("Optional: only list ABPs targeting this skeleton"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimListAnimBlueprints(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_list_sequences");
		T.Description = TEXT("List Animation Sequence assets in the project.");
		T.Params = {
			{ TEXT("filter"),        TEXT("string"), TEXT("Optional substring filter on asset name"), false },
			{ TEXT("skeleton_path"), TEXT("string"), TEXT("Optional: only list sequences for this skeleton"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimListSequences(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_list_skeletal_meshes");
		T.Description = TEXT("List Skeletal Mesh assets in the project.");
		T.Params = { { TEXT("filter"), TEXT("string"), TEXT("Optional substring filter on asset name"), false } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimListSkeletalMeshes(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_get_skeleton_bones");
		T.Description = TEXT("Get the bone hierarchy from a Skeleton asset.");
		T.Params = { { TEXT("skeleton_path"), TEXT("string"), TEXT("Content path to the Skeleton asset"), true } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimGetSkeletonBones(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_create_anim_blueprint");
		T.Description = TEXT("Create a new Animation Blueprint targeting a given Skeleton.");
		T.Params = {
			{ TEXT("path"),          TEXT("string"), TEXT("Content path for the new asset, e.g. /Game/Animations/ABP_Character"), true },
			{ TEXT("skeleton_path"), TEXT("string"), TEXT("Content path to the target Skeleton asset"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimCreateAnimBlueprint(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_get_anim_graph");
		T.Description = TEXT("Inspect the graphs and state machines inside an Animation Blueprint.");
		T.Params = { { TEXT("anim_bp_path"), TEXT("string"), TEXT("Content path to the Animation Blueprint"), true } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimGetAnimGraph(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_add_state_machine");
		T.Description = TEXT("Add a State Machine node to an Animation Blueprint's anim graph.");
		T.Params = {
			{ TEXT("anim_bp_path"), TEXT("string"), TEXT("Content path to the Animation Blueprint"), true },
			{ TEXT("sm_name"),      TEXT("string"), TEXT("Name for the state machine"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimAddStateMachine(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_add_state");
		T.Description = TEXT("Add a state to a State Machine inside an Animation Blueprint.");
		T.Params = {
			{ TEXT("anim_bp_path"), TEXT("string"), TEXT("Content path to the Animation Blueprint"), true },
			{ TEXT("sm_name"),      TEXT("string"), TEXT("Name of the state machine to add the state to"), true },
			{ TEXT("state_name"),   TEXT("string"), TEXT("Name of the new state"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimAddState(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_set_state_animation");
		T.Description = TEXT(
			"Assign an animation asset to a state inside a State Machine. "
			"Creates a Sequence Player (for AnimSequence/AnimMontage) or Blend Space Player "
			"node inside the state's graph and wires it to the Output Pose. "
			"Call after anim_add_state.");
		T.Params = {
			{ TEXT("anim_bp_path"),      TEXT("string"), TEXT("Content path to the Animation Blueprint"), true },
			{ TEXT("sm_name"),           TEXT("string"), TEXT("Name of the State Machine containing the state"), true },
			{ TEXT("state_name"),        TEXT("string"), TEXT("Name of the state to assign the animation to"), true },
			{ TEXT("animation_asset"),   TEXT("string"), TEXT("Content path to the AnimSequence or BlendSpace asset"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimSetStateAnimation(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_add_transition");
		T.Description = TEXT(
			"Add a transition between two states in a State Machine. "
			"The transition's rule graph (BoundGraph) is created automatically. "
			"Use anim_set_transition_condition to add a condition, or leave empty to always transition.");
		T.Params = {
			{ TEXT("anim_bp_path"),  TEXT("string"), TEXT("Content path to the Animation Blueprint"), true },
			{ TEXT("sm_name"),       TEXT("string"), TEXT("Name of the State Machine"), true },
			{ TEXT("from_state"),    TEXT("string"), TEXT("Name of the source state"), true },
			{ TEXT("to_state"),      TEXT("string"), TEXT("Name of the destination state"), true },
			{ TEXT("bidirectional"), TEXT("string"), TEXT("'true' to create the reverse transition as well (default: false)"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimAddTransition(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_set_transition_condition");
		T.Description = TEXT(
			"Set a condition on a transition rule. Supports: "
			"'variable_true' (bool var is true), 'variable_false' (bool var is false), "
			"'variable_greater_than' (float var > value), 'variable_less_than' (float var < value). "
			"The variable must exist on the Animation Blueprint.");
		T.Params = {
			{ TEXT("anim_bp_path"),      TEXT("string"), TEXT("Content path to the Animation Blueprint"), true },
			{ TEXT("sm_name"),           TEXT("string"), TEXT("Name of the State Machine"), true },
			{ TEXT("from_state"),        TEXT("string"), TEXT("Name of the source state"), true },
			{ TEXT("to_state"),          TEXT("string"), TEXT("Name of the destination state"), true },
			{ TEXT("condition_type"),    TEXT("string"), TEXT("One of: variable_true, variable_false, variable_greater_than, variable_less_than"), true },
			{ TEXT("variable"),          TEXT("string"), TEXT("Name of the AnimInstance variable to read"), true },
			{ TEXT("value"),             TEXT("string"), TEXT("Threshold value for variable_greater_than / variable_less_than (as a number string)"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimSetTransitionCondition(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_compile_anim_blueprint");
		T.Description = TEXT("Save an Animation Blueprint and mark it for recompile.");
		T.Params = { { TEXT("anim_bp_path"), TEXT("string"), TEXT("Content path to the Animation Blueprint"), true } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimCompileAnimBlueprint(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("skeleton_fix_broken_references");
		T.Description = TEXT("Reassign the Skeleton on a SkeletalMesh asset — equivalent to the editor's 'Assign Skeleton' dialog. Use when a mesh shows None skeleton after cross-project asset copies.");
		T.Params = {
			{ TEXT("mesh_path"),     TEXT("string"), TEXT("Content path to the SkeletalMesh asset"), true },
			{ TEXT("skeleton_path"), TEXT("string"), TEXT("Content path to the target Skeleton asset"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimFixSkeletonReference(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_set_entry_state");
		T.Description = TEXT("Connect the Entry node to a named state in a State Machine. The Entry node determines which state plays first. anim_add_state auto-connects Entry to the first state added, but call this to change it later.");
		T.Params = {
			{ TEXT("anim_bp_path"), TEXT("string"), TEXT("Content path to the Animation Blueprint"), true },
			{ TEXT("sm_name"),      TEXT("string"), TEXT("Name of the state machine"), true },
			{ TEXT("state_name"),   TEXT("string"), TEXT("Name of the state to set as the entry state"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimSetEntryState(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("anim_create_locomotion_setup");
		T.Description = TEXT(
			"Build a full 5-state locomotion state machine (Idle/WalkRun/Jump/Fall/Land) on an existing Animation Blueprint in one call. "
			"Adds Speed (float) and IsInAir (bool) variables, creates LocomotionSM, assigns animations, wires all transitions and conditions, and compiles. "
			"The EventGraph update logic (velocity -> Speed, IsFalling -> IsInAir) must be wired manually after.");
		T.Params = {
			{ TEXT("anim_bp_path"),      TEXT("string"), TEXT("Content path to an existing Animation Blueprint"), true },
			{ TEXT("idle"),              TEXT("string"), TEXT("Content path to the Idle AnimSequence"), true },
			{ TEXT("walk_run"),          TEXT("string"), TEXT("Content path to the WalkRun BlendSpace or AnimSequence"), true },
			{ TEXT("jump"),              TEXT("string"), TEXT("Content path to the Jump AnimSequence"), true },
			{ TEXT("fall"),              TEXT("string"), TEXT("Content path to the Fall AnimSequence"), true },
			{ TEXT("land"),              TEXT("string"), TEXT("Content path to the Land AnimSequence"), true },
			{ TEXT("speed_threshold"),   TEXT("string"), TEXT("Speed value where Idle<->WalkRun transition fires (default: 10.0)"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimCreateLocomotionSetup(P); };
		OutTools.Add(T);
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static IAssetRegistry& GetAR()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
}

static UAnimBlueprint* LoadAnimBlueprint(const FString& Path)
{
	return Cast<UAnimBlueprint>(StaticLoadObject(UAnimBlueprint::StaticClass(), nullptr, *Path));
}

// ---------------------------------------------------------------------------
// anim_list_skeletons
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimListSkeletons(const TSharedPtr<FJsonObject>& Params)
{
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	TArray<FAssetData> Assets;
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Skeleton")));
	ARFilter.bRecursivePaths = true;
	GetAR().GetAssets(ARFilter, Assets);

	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FAssetData& A : Assets)
	{
		FString Name = A.AssetName.ToString();
		if (!Filter.IsEmpty() && !Name.Contains(Filter, ESearchCase::IgnoreCase)) continue;
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Name);
		Obj->SetStringField(TEXT("path"), A.GetSoftObjectPath().ToString());
		Arr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetArrayField(TEXT("skeletons"), Arr);
	Response->SetNumberField(TEXT("count"), Arr.Num());
	return FMCPToolResult::Success(FString::Printf(TEXT("Found %d skeletons"), Arr.Num()), Response);
}

// ---------------------------------------------------------------------------
// anim_list_anim_blueprints
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimListAnimBlueprints(const TSharedPtr<FJsonObject>& Params)
{
	FString Filter, SkeletonPath;
	Params->TryGetStringField(TEXT("filter"), Filter);
	Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath);

	TArray<FAssetData> Assets;
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("AnimBlueprint")));
	ARFilter.bRecursivePaths = true;
	GetAR().GetAssets(ARFilter, Assets);

	USkeleton* FilterSkeleton = nullptr;
	if (!SkeletonPath.IsEmpty())
		FilterSkeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));

	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FAssetData& A : Assets)
	{
		FString Name = A.AssetName.ToString();
		if (!Filter.IsEmpty() && !Name.Contains(Filter, ESearchCase::IgnoreCase)) continue;

		if (FilterSkeleton)
		{
			UAnimBlueprint* ABP = Cast<UAnimBlueprint>(A.GetAsset());
			if (!ABP || ABP->TargetSkeleton != FilterSkeleton) continue;
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Name);
		Obj->SetStringField(TEXT("path"), A.GetSoftObjectPath().ToString());
		Arr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetArrayField(TEXT("anim_blueprints"), Arr);
	Response->SetNumberField(TEXT("count"), Arr.Num());
	return FMCPToolResult::Success(FString::Printf(TEXT("Found %d animation blueprints"), Arr.Num()), Response);
}

// ---------------------------------------------------------------------------
// anim_list_sequences
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimListSequences(const TSharedPtr<FJsonObject>& Params)
{
	FString Filter, SkeletonPath;
	Params->TryGetStringField(TEXT("filter"), Filter);
	Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath);

	TArray<FAssetData> Assets;
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("AnimSequence")));
	ARFilter.bRecursivePaths = true;
	GetAR().GetAssets(ARFilter, Assets);

	USkeleton* FilterSkeleton = nullptr;
	if (!SkeletonPath.IsEmpty())
		FilterSkeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));

	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FAssetData& A : Assets)
	{
		FString Name = A.AssetName.ToString();
		if (!Filter.IsEmpty() && !Name.Contains(Filter, ESearchCase::IgnoreCase)) continue;

		if (FilterSkeleton)
		{
			UAnimSequence* Seq = Cast<UAnimSequence>(A.GetAsset());
			if (!Seq || Seq->GetSkeleton() != FilterSkeleton) continue;
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Name);
		Obj->SetStringField(TEXT("path"), A.GetSoftObjectPath().ToString());

		// Include duration if loaded
		if (UAnimSequence* Seq = Cast<UAnimSequence>(A.GetAsset()))
			Obj->SetNumberField(TEXT("duration_seconds"), Seq->GetPlayLength());

		Arr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetArrayField(TEXT("sequences"), Arr);
	Response->SetNumberField(TEXT("count"), Arr.Num());
	return FMCPToolResult::Success(FString::Printf(TEXT("Found %d animation sequences"), Arr.Num()), Response);
}

// ---------------------------------------------------------------------------
// anim_list_skeletal_meshes
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimListSkeletalMeshes(const TSharedPtr<FJsonObject>& Params)
{
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	TArray<FAssetData> Assets;
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SkeletalMesh")));
	ARFilter.bRecursivePaths = true;
	GetAR().GetAssets(ARFilter, Assets);

	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FAssetData& A : Assets)
	{
		FString Name = A.AssetName.ToString();
		if (!Filter.IsEmpty() && !Name.Contains(Filter, ESearchCase::IgnoreCase)) continue;
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Name);
		Obj->SetStringField(TEXT("path"), A.GetSoftObjectPath().ToString());
		Arr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetArrayField(TEXT("skeletal_meshes"), Arr);
	Response->SetNumberField(TEXT("count"), Arr.Num());
	return FMCPToolResult::Success(FString::Printf(TEXT("Found %d skeletal meshes"), Arr.Num()), Response);
}

// ---------------------------------------------------------------------------
// anim_get_skeleton_bones
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimGetSkeletonBones(const TSharedPtr<FJsonObject>& Params)
{
	FString SkeletonPath;
	if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
		return FMCPToolResult::Error(TEXT("Missing required param: skeleton_path"));

	USkeleton* Skeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));
	if (!Skeleton)
		return FMCPToolResult::Error(FString::Printf(TEXT("Skeleton not found: %s"), *SkeletonPath));

	const FReferenceSkeleton& RefSkel = Skeleton->GetReferenceSkeleton();
	int32 BoneCount = RefSkel.GetNum();

	TArray<TSharedPtr<FJsonValue>> Bones;
	for (int32 i = 0; i < BoneCount; i++)
	{
		TSharedPtr<FJsonObject> Bone = MakeShared<FJsonObject>();
		Bone->SetStringField(TEXT("name"), RefSkel.GetBoneName(i).ToString());
		Bone->SetNumberField(TEXT("index"), i);
		int32 ParentIdx = RefSkel.GetParentIndex(i);
		if (ParentIdx >= 0)
			Bone->SetStringField(TEXT("parent"), RefSkel.GetBoneName(ParentIdx).ToString());
		else
			Bone->SetStringField(TEXT("parent"), TEXT(""));
		Bones.Add(MakeShared<FJsonValueObject>(Bone));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("skeleton"), SkeletonPath);
	Response->SetNumberField(TEXT("bone_count"), BoneCount);
	Response->SetArrayField(TEXT("bones"), Bones);
	return FMCPToolResult::Success(FString::Printf(TEXT("Skeleton '%s': %d bones"), *SkeletonPath, BoneCount), Response);
}

// ---------------------------------------------------------------------------
// anim_create_anim_blueprint
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimCreateAnimBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	FString Path, SkeletonPath;
	if (!Params->TryGetStringField(TEXT("path"), Path))
		return FMCPToolResult::Error(TEXT("Missing required param: path"));
	if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
		return FMCPToolResult::Error(TEXT("Missing required param: skeleton_path"));

	USkeleton* Skeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));
	if (!Skeleton)
		return FMCPToolResult::Error(FString::Printf(TEXT("Skeleton not found: %s"), *SkeletonPath));

	int32 LastSlash;
	if (!Path.FindLastChar(TEXT('/'), LastSlash))
		return FMCPToolResult::Error(TEXT("Invalid path"));
	FString AssetName = Path.Mid(LastSlash + 1);

	UPackage* Package = CreatePackage(*Path);

	UAnimBlueprint* AnimBP = NewObject<UAnimBlueprint>(
		Package, UAnimBlueprint::StaticClass(), FName(*AssetName),
		RF_Public | RF_Standalone | RF_Transactional);
	AnimBP->TargetSkeleton = Skeleton;
	AnimBP->ParentClass = UAnimInstance::StaticClass();
	AnimBP->BlueprintType = BPTYPE_Normal;

	// Create the main anim graph
	UEdGraph* AnimGraph = FBlueprintEditorUtils::CreateNewGraph(
		AnimBP,
		TEXT("AnimGraph"),
		UAnimationGraph::StaticClass(),
		UAnimationGraphSchema::StaticClass());
	AnimBP->FunctionGraphs.Add(AnimGraph);

	// Explicitly create the Output Pose (AnimGraphNode_Root) node — CreateNewGraph doesn't
	// fire CreateDefaultNodesForGraph automatically in the programmatic path.
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();
	Schema->CreateDefaultNodesForGraph(*AnimGraph);

	// Create the EventGraph (Ubergraph) — without it there is no EventGraph tab in the editor
	// and no place to wire BlueprintUpdateAnimation logic (speed, IsInAir, etc.)
	UEdGraph* EventGraph = FBlueprintEditorUtils::CreateNewGraph(
		AnimBP,
		FName(TEXT("EventGraph")),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass());
	AnimBP->UbergraphPages.Add(EventGraph);

	FAssetRegistryModule::AssetCreated(AnimBP);
	Package->MarkPackageDirty();

	return FMCPToolResult::Success(FString::Printf(TEXT("Created Animation Blueprint '%s' targeting '%s'"), *Path, *SkeletonPath));
}

// ---------------------------------------------------------------------------
// anim_get_anim_graph
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimGetAnimGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"), AnimBPPath))
		return FMCPToolResult::Error(TEXT("Missing required param: anim_bp_path"));

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Animation Blueprint not found: %s"), *AnimBPPath));

	TArray<TSharedPtr<FJsonValue>> GraphArr;

	// Inspect all graphs — look for UAnimationGraph and state machines within
	TArray<UEdGraph*> AllGraphs;
	AnimBP->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph) continue;
		TSharedPtr<FJsonObject> GObj = MakeShared<FJsonObject>();
		GObj->SetStringField(TEXT("name"), Graph->GetName());
		GObj->SetStringField(TEXT("class"), Graph->GetClass()->GetName());

		TArray<TSharedPtr<FJsonValue>> NodeArr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;
			TSharedPtr<FJsonObject> NObj = MakeShared<FJsonObject>();
			NObj->SetStringField(TEXT("name"), Node->GetName());
			NObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
			NObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

			// If it's a state machine node, list its states
			if (UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node))
			{
				UAnimationStateMachineGraph* SMGraph = Cast<UAnimationStateMachineGraph>(SMNode->EditorStateMachineGraph);
				if (SMGraph)
				{
					TArray<TSharedPtr<FJsonValue>> StateArr;
					for (UEdGraphNode* SMNode2 : SMGraph->Nodes)
					{
						if (UAnimStateNode* StateNode = Cast<UAnimStateNode>(SMNode2))
						{
							TSharedPtr<FJsonObject> SObj = MakeShared<FJsonObject>();
							SObj->SetStringField(TEXT("name"), StateNode->GetStateName());
							StateArr.Add(MakeShared<FJsonValueObject>(SObj));
						}
					}
					NObj->SetArrayField(TEXT("states"), StateArr);
					NObj->SetNumberField(TEXT("state_count"), StateArr.Num());
				}
			}

			NodeArr.Add(MakeShared<FJsonValueObject>(NObj));
		}
		GObj->SetArrayField(TEXT("nodes"), NodeArr);
		GObj->SetNumberField(TEXT("node_count"), NodeArr.Num());
		GraphArr.Add(MakeShared<FJsonValueObject>(GObj));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("anim_bp"), AnimBPPath);
	Response->SetArrayField(TEXT("graphs"), GraphArr);
	Response->SetNumberField(TEXT("graph_count"), GraphArr.Num());
	return FMCPToolResult::Success(
		FString::Printf(TEXT("Animation Blueprint '%s': %d graphs"), *AnimBPPath, GraphArr.Num()), Response);
}

// ---------------------------------------------------------------------------
// anim_add_state_machine
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimAddStateMachine(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath, SMName;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"), AnimBPPath))
		return FMCPToolResult::Error(TEXT("Missing required param: anim_bp_path"));
	if (!Params->TryGetStringField(TEXT("sm_name"), SMName))
		return FMCPToolResult::Error(TEXT("Missing required param: sm_name"));

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Animation Blueprint not found: %s"), *AnimBPPath));

	// Find the main anim graph (UAnimationGraph)
	UAnimationGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : AnimBP->FunctionGraphs)
	{
		if (UAnimationGraph* AG = Cast<UAnimationGraph>(Graph))
		{
			AnimGraph = AG;
			break;
		}
	}
	if (!AnimGraph)
		return FMCPToolResult::Error(TEXT("No AnimationGraph found in this Animation Blueprint. Try anim_create_anim_blueprint."));

	// Use FGraphNodeCreator — PostPlacedNewNode creates EditorStateMachineGraph AND its entry
	// state node automatically. Raw NewObject skips this, leaving the SM graph without the
	// required entry node which crashes the AnimBP compiler.
	FGraphNodeCreator<UAnimGraphNode_StateMachine> NodeCreator(*AnimGraph);
	UAnimGraphNode_StateMachine* SMNode = NodeCreator.CreateNode(false);
	SMNode->NodePosX = -300;
	SMNode->NodePosY = 0;
	NodeCreator.Finalize();

	UAnimationStateMachineGraph* SMGraph = Cast<UAnimationStateMachineGraph>(SMNode->EditorStateMachineGraph);
	if (!SMGraph)
		return FMCPToolResult::Error(TEXT("State machine graph not created by PostPlacedNewNode"));
	SMGraph->Rename(*SMName, nullptr, REN_DontCreateRedirectors);

	// Auto-wire: connect SM output pose pin to the Output Pose (AnimGraphNode_Root) node
	UAnimGraphNode_Root* RootNode = nullptr;
	for (UEdGraphNode* N : AnimGraph->Nodes)
	{
		if (UAnimGraphNode_Root* R = Cast<UAnimGraphNode_Root>(N))
		{
			RootNode = R;
			break;
		}
	}
	if (RootNode)
	{
		// Only allocate pins if they don't exist yet — CreateDefaultNodesForGraph already did this
		// on the first call. Re-allocating breaks any existing connections to Output Pose.
		if (RootNode->Pins.Num() == 0) RootNode->AllocateDefaultPins();
		const UAnimationGraphSchema* Schema = CastChecked<UAnimationGraphSchema>(AnimGraph->GetSchema());
		UEdGraphPin* SMOutPin = nullptr;
		UEdGraphPin* RootInPin = nullptr;
		for (UEdGraphPin* Pin : SMNode->Pins)
			if (Pin->Direction == EGPD_Output) { SMOutPin = Pin; break; }
		for (UEdGraphPin* Pin : RootNode->Pins)
			if (Pin->Direction == EGPD_Input) { RootInPin = Pin; break; }
		if (SMOutPin && RootInPin)
			Schema->TryCreateConnection(SMOutPin, RootInPin);
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	return FMCPToolResult::Success(
		FString::Printf(TEXT("Added state machine '%s' to '%s'"), *SMName, *AnimBPPath));
}

// ---------------------------------------------------------------------------
// anim_add_state
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimAddState(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath, SMName, StateName;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"), AnimBPPath))
		return FMCPToolResult::Error(TEXT("Missing required param: anim_bp_path"));
	if (!Params->TryGetStringField(TEXT("sm_name"), SMName))
		return FMCPToolResult::Error(TEXT("Missing required param: sm_name"));
	if (!Params->TryGetStringField(TEXT("state_name"), StateName))
		return FMCPToolResult::Error(TEXT("Missing required param: state_name"));

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Animation Blueprint not found: %s"), *AnimBPPath));

	// Find the state machine node by name
	UAnimGraphNode_StateMachine* TargetSM = nullptr;
	TArray<UEdGraph*> AllGraphs;
	AnimBP->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node))
			{
				if (SMNode->EditorStateMachineGraph &&
					SMNode->EditorStateMachineGraph->GetName() == SMName)
				{
					TargetSM = SMNode;
					break;
				}
			}
		}
		if (TargetSM) break;
	}

	if (!TargetSM)
		return FMCPToolResult::Error(FString::Printf(TEXT("State machine '%s' not found"), *SMName));

	UAnimationStateMachineGraph* SMGraph = Cast<UAnimationStateMachineGraph>(TargetSM->EditorStateMachineGraph);
	if (!SMGraph)
		return FMCPToolResult::Error(TEXT("State machine graph is invalid"));

	// Count existing states to compute a spread-out position
	int32 ExistingStateCount = 0;
	for (UEdGraphNode* N : SMGraph->Nodes)
		if (Cast<UAnimStateNode>(N)) ExistingStateCount++;

	// Use FGraphNodeCreator — properly calls AllocateDefaultPins + PostPlacedNewNode.
	// PostPlacedNewNode creates BoundGraph automatically and initialises the compiler's
	// internal state arrays. Raw NewObject + AddNode bypasses all of this and causes
	// "Array index out of bounds" crashes in the AnimBP compiler.
	FGraphNodeCreator<UAnimStateNode> NodeCreator(*SMGraph);
	UAnimStateNode* StateNode = NodeCreator.CreateNode(false);
	StateNode->NodePosX = 200 + ExistingStateCount * 300;
	StateNode->NodePosY = 0;
	NodeCreator.Finalize();
	StateNode->OnRenameNode(StateName);

	// Auto-connect Entry → first state added
	if (ExistingStateCount == 0)
	{
		UAnimStateEntryNode* EntryNode = nullptr;
		for (UEdGraphNode* N : SMGraph->Nodes)
			if (UAnimStateEntryNode* EN = Cast<UAnimStateEntryNode>(N)) { EntryNode = EN; break; }
		if (EntryNode)
		{
			const UEdGraphSchema* SMSchema = SMGraph->GetSchema();
			UEdGraphPin* EntryOut = nullptr; UEdGraphPin* StateIn = nullptr;
			for (UEdGraphPin* P : EntryNode->Pins) if (P->Direction == EGPD_Output) { EntryOut = P; break; }
			for (UEdGraphPin* P : StateNode->Pins)  if (P->Direction == EGPD_Input)  { StateIn  = P; break; }
			if (EntryOut && StateIn) SMSchema->TryCreateConnection(EntryOut, StateIn);
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	return FMCPToolResult::Success(
		FString::Printf(TEXT("Added state '%s' to state machine '%s'"), *StateName, *SMName));
}

// ---------------------------------------------------------------------------
// anim_compile_anim_blueprint
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimCompileAnimBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"), AnimBPPath))
		return FMCPToolResult::Error(TEXT("Missing required param: anim_bp_path"));

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Animation Blueprint not found: %s"), *AnimBPPath));

	// Pre-flight: find the Output Pose (AnimGraphNode_Root) node in the main AnimGraph
	UAnimGraphNode_Root* RootNode = nullptr;
	for (UEdGraph* Graph : AnimBP->FunctionGraphs)
	{
		if (Cast<UAnimationGraph>(Graph))
		{
			for (UEdGraphNode* N : Graph->Nodes)
			{
				if (UAnimGraphNode_Root* R = Cast<UAnimGraphNode_Root>(N))
				{
					RootNode = R;
					break;
				}
			}
			break;
		}
	}
	if (!RootNode)
		return FMCPToolResult::Error(TEXT("AnimGraph has no Output Pose node. This may be a Child AnimBP — open and edit the parent instead."));

	bool bHasConnection = false;
	for (UEdGraphPin* Pin : RootNode->Pins)
	{
		if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
		{
			bHasConnection = true;
			break;
		}
	}
	if (!bHasConnection)
		return FMCPToolResult::Error(TEXT("AnimGraph Output Pose has no incoming connections. Use anim_add_state_machine first, or connect a node to Output Pose before compiling."));

	// Actually compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(AnimBP, EBlueprintCompileOptions::None);

	UPackage* Package = AnimBP->GetOutermost();
	if (Package)
	{
		Package->MarkPackageDirty();
		FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);
	}
	return FMCPToolResult::Success(FString::Printf(TEXT("Compiled and saved '%s'"), *AnimBPPath));
}

// ---------------------------------------------------------------------------
// skeleton_fix_broken_references
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimFixSkeletonReference(const TSharedPtr<FJsonObject>& Params)
{
	FString MeshPath, SkeletonPath;
	if (!Params->TryGetStringField(TEXT("mesh_path"), MeshPath))
		return FMCPToolResult::Error(TEXT("Missing required param: mesh_path"));
	if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
		return FMCPToolResult::Error(TEXT("Missing required param: skeleton_path"));

	USkeletalMesh* Mesh = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *MeshPath));
	if (!Mesh)
		return FMCPToolResult::Error(FString::Printf(TEXT("SkeletalMesh not found: %s"), *MeshPath));

	USkeleton* Skeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));
	if (!Skeleton)
		return FMCPToolResult::Error(FString::Printf(TEXT("Skeleton not found: %s"), *SkeletonPath));

	Mesh->SetSkeleton(Skeleton);
	Mesh->GetOutermost()->MarkPackageDirty();
	FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);

	return FMCPToolResult::Success(FString::Printf(
		TEXT("Assigned skeleton '%s' to mesh '%s'"), *SkeletonPath, *MeshPath));
}

// ---------------------------------------------------------------------------
// anim_set_state_animation
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimSetStateAnimation(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath, SMName, StateName, AssetPath;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"),    AnimBPPath))  return FMCPToolResult::Error(TEXT("Missing: anim_bp_path"));
	if (!Params->TryGetStringField(TEXT("sm_name"),         SMName))      return FMCPToolResult::Error(TEXT("Missing: sm_name"));
	if (!Params->TryGetStringField(TEXT("state_name"),      StateName))   return FMCPToolResult::Error(TEXT("Missing: state_name"));
	if (!Params->TryGetStringField(TEXT("animation_asset"), AssetPath))   return FMCPToolResult::Error(TEXT("Missing: animation_asset"));

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPPath));

	// Find the state machine node by name
	UAnimGraphNode_StateMachine* TargetSM = nullptr;
	TArray<UEdGraph*> AllGraphs;
	AnimBP->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node))
			{
				if (SMNode->EditorStateMachineGraph &&
					SMNode->EditorStateMachineGraph->GetName() == SMName)
				{
					TargetSM = SMNode;
					break;
				}
			}
		}
		if (TargetSM) break;
	}
	if (!TargetSM)
		return FMCPToolResult::Error(FString::Printf(TEXT("State machine '%s' not found"), *SMName));

	UAnimationStateMachineGraph* SMGraph = Cast<UAnimationStateMachineGraph>(TargetSM->EditorStateMachineGraph);
	if (!SMGraph)
		return FMCPToolResult::Error(TEXT("State machine graph is invalid"));

	// Find the state node by name
	UAnimStateNode* StateNode = nullptr;
	for (UEdGraphNode* Node : SMGraph->Nodes)
	{
		if (UAnimStateNode* SN = Cast<UAnimStateNode>(Node))
		{
			if (SN->GetStateName() == StateName)
			{
				StateNode = SN;
				break;
			}
		}
	}
	if (!StateNode)
		return FMCPToolResult::Error(FString::Printf(TEXT("State '%s' not found in state machine '%s'"), *StateName, *SMName));

	UAnimationStateGraph* StateGraph = Cast<UAnimationStateGraph>(StateNode->BoundGraph);
	if (!StateGraph)
		return FMCPToolResult::Error(FString::Printf(
			TEXT("State '%s' has no bound graph — was it created with anim_add_state?"), *StateName));

	// Find the StateResult node (Output Pose equivalent inside state graphs)
	UAnimGraphNode_StateResult* ResultNode = nullptr;
	for (UEdGraphNode* Node : StateGraph->Nodes)
	{
		if (UAnimGraphNode_StateResult* RN = Cast<UAnimGraphNode_StateResult>(Node))
		{
			ResultNode = RN;
			break;
		}
	}
	if (!ResultNode)
		return FMCPToolResult::Error(FString::Printf(
			TEXT("State graph for '%s' has no StateResult node — internal graph corruption"), *StateName));

	// Load the animation asset and determine the player node type
	UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!Asset)
		return FMCPToolResult::Error(FString::Printf(TEXT("Animation asset not found: %s"), *AssetPath));

	const UAnimationGraphSchema* Schema = CastChecked<UAnimationGraphSchema>(StateGraph->GetSchema());
	UEdGraphNode* PlayerNode = nullptr;

	if (UBlendSpace* BS = Cast<UBlendSpace>(Asset))
	{
		// BlendSpace — use BlendSpacePlayer
		FGraphNodeCreator<UAnimGraphNode_BlendSpacePlayer> NodeCreator(*StateGraph);
		UAnimGraphNode_BlendSpacePlayer* BSNode = NodeCreator.CreateNode(false);
		BSNode->Node.SetBlendSpace(BS);
		BSNode->NodePosX = ResultNode->NodePosX - 300;
		BSNode->NodePosY = ResultNode->NodePosY;
		NodeCreator.Finalize();
		PlayerNode = BSNode;
	}
	else if (UAnimSequenceBase* Seq = Cast<UAnimSequenceBase>(Asset))
	{
		// AnimSequence / AnimComposite / etc — use SequencePlayer
		FGraphNodeCreator<UAnimGraphNode_SequencePlayer> NodeCreator(*StateGraph);
		UAnimGraphNode_SequencePlayer* SeqNode = NodeCreator.CreateNode(false);
		SeqNode->Node.SetSequence(Seq);
		SeqNode->NodePosX = ResultNode->NodePosX - 300;
		SeqNode->NodePosY = ResultNode->NodePosY;
		NodeCreator.Finalize();
		PlayerNode = SeqNode;
	}
	else
	{
		return FMCPToolResult::Error(FString::Printf(
			TEXT("Asset '%s' is not a supported type. Expected AnimSequence or BlendSpace (got %s)"),
			*AssetPath, *Asset->GetClass()->GetName()));
	}

	// Wire: player output pose → StateResult input pose
	UEdGraphPin* OutPin = nullptr;
	UEdGraphPin* InPin  = nullptr;
	for (UEdGraphPin* Pin : PlayerNode->Pins)
		if (Pin->Direction == EGPD_Output) { OutPin = Pin; break; }
	for (UEdGraphPin* Pin : ResultNode->Pins)
		if (Pin->Direction == EGPD_Input) { InPin = Pin; break; }

	if (!OutPin || !InPin)
		return FMCPToolResult::Error(TEXT("Could not find pose pins for connection — check node types"));

	Schema->TryCreateConnection(OutPin, InPin);

	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	return FMCPToolResult::Success(FString::Printf(
		TEXT("Assigned '%s' to state '%s' in '%s/%s'"), *AssetPath, *StateName, *AnimBPPath, *SMName));
}

// ---------------------------------------------------------------------------
// Shared helpers for transition tools
// ---------------------------------------------------------------------------

static UAnimationStateMachineGraph* FindSMGraph(UAnimBlueprint* AnimBP, const FString& SMName)
{
	TArray<UEdGraph*> AllGraphs;
	AnimBP->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node))
			{
				if (SMNode->EditorStateMachineGraph &&
					SMNode->EditorStateMachineGraph->GetName() == SMName)
					return Cast<UAnimationStateMachineGraph>(SMNode->EditorStateMachineGraph);
			}
		}
	}
	return nullptr;
}

static UAnimStateNode* FindStateNode(UAnimationStateMachineGraph* SMGraph, const FString& StateName)
{
	for (UEdGraphNode* Node : SMGraph->Nodes)
	{
		if (UAnimStateNode* SN = Cast<UAnimStateNode>(Node))
			if (SN->GetStateName() == StateName)
				return SN;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// anim_add_transition
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimAddTransition(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath, SMName, FromState, ToState;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"), AnimBPPath)) return FMCPToolResult::Error(TEXT("Missing: anim_bp_path"));
	if (!Params->TryGetStringField(TEXT("sm_name"),      SMName))     return FMCPToolResult::Error(TEXT("Missing: sm_name"));
	if (!Params->TryGetStringField(TEXT("from_state"),   FromState))  return FMCPToolResult::Error(TEXT("Missing: from_state"));
	if (!Params->TryGetStringField(TEXT("to_state"),     ToState))    return FMCPToolResult::Error(TEXT("Missing: to_state"));

	FString BidirectionalStr;
	Params->TryGetStringField(TEXT("bidirectional"), BidirectionalStr);
	bool bBidirectional = BidirectionalStr.Equals(TEXT("true"), ESearchCase::IgnoreCase);

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPPath));

	UAnimationStateMachineGraph* SMGraph = FindSMGraph(AnimBP, SMName);
	if (!SMGraph)
		return FMCPToolResult::Error(FString::Printf(TEXT("State machine '%s' not found"), *SMName));

	UAnimStateNode* FromNode = FindStateNode(SMGraph, FromState);
	if (!FromNode)
		return FMCPToolResult::Error(FString::Printf(TEXT("Source state '%s' not found in '%s'"), *FromState, *SMName));

	UAnimStateNode* ToNode = FindStateNode(SMGraph, ToState);
	if (!ToNode)
		return FMCPToolResult::Error(FString::Printf(TEXT("Destination state '%s' not found in '%s'"), *ToState, *SMName));

	// Create the transition — PostPlacedNewNode creates BoundGraph (rule graph) automatically
	FGraphNodeCreator<UAnimStateTransitionNode> NodeCreator(*SMGraph);
	UAnimStateTransitionNode* TransitionNode = NodeCreator.CreateNode(false);
	TransitionNode->NodePosX = (FromNode->NodePosX + ToNode->NodePosX) / 2;
	TransitionNode->NodePosY = (FromNode->NodePosY + ToNode->NodePosY) / 2;
	TransitionNode->Bidirectional = bBidirectional;
	NodeCreator.Finalize();

	// Wire the transition between the two state nodes
	TransitionNode->CreateConnections(FromNode, ToNode);

	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);

	FString Msg = FString::Printf(TEXT("Added transition '%s' → '%s' in state machine '%s'"),
		*FromState, *ToState, *SMName);
	if (bBidirectional) Msg += TEXT(" (bidirectional)");
	return FMCPToolResult::Success(Msg);
}

// ---------------------------------------------------------------------------
// anim_set_transition_condition
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimSetTransitionCondition(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath, SMName, FromState, ToState, ConditionType, VarName, ValueStr;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"),   AnimBPPath))     return FMCPToolResult::Error(TEXT("Missing: anim_bp_path"));
	if (!Params->TryGetStringField(TEXT("sm_name"),        SMName))         return FMCPToolResult::Error(TEXT("Missing: sm_name"));
	if (!Params->TryGetStringField(TEXT("from_state"),     FromState))      return FMCPToolResult::Error(TEXT("Missing: from_state"));
	if (!Params->TryGetStringField(TEXT("to_state"),       ToState))        return FMCPToolResult::Error(TEXT("Missing: to_state"));
	if (!Params->TryGetStringField(TEXT("condition_type"), ConditionType))  return FMCPToolResult::Error(TEXT("Missing: condition_type"));
	// variable + value are only required for non-always_true conditions — read them after the type check
	Params->TryGetStringField(TEXT("variable"), VarName);
	Params->TryGetStringField(TEXT("value"), ValueStr);

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPPath));

	UAnimationStateMachineGraph* SMGraph = FindSMGraph(AnimBP, SMName);
	if (!SMGraph)
		return FMCPToolResult::Error(FString::Printf(TEXT("State machine '%s' not found"), *SMName));

	// Find the transition node between the two states
	UAnimStateTransitionNode* Transition = nullptr;
	for (UEdGraphNode* Node : SMGraph->Nodes)
	{
		if (UAnimStateTransitionNode* TN = Cast<UAnimStateTransitionNode>(Node))
		{
			UAnimStateNodeBase* Prev = TN->GetPreviousState();
			UAnimStateNodeBase* Next = TN->GetNextState();
			if (Prev && Next &&
				Prev->GetStateName() == FromState &&
				Next->GetStateName() == ToState)
			{
				Transition = TN;
				break;
			}
		}
	}
	if (!Transition)
		return FMCPToolResult::Error(FString::Printf(
			TEXT("No transition found from '%s' to '%s'. Use anim_add_transition first."), *FromState, *ToState));

	UEdGraph* BoundGraph = Transition->BoundGraph;
	if (!BoundGraph)
		return FMCPToolResult::Error(TEXT("Transition has no rule graph — was it created with anim_add_transition?"));

	// Find the TransitionResult node (the sink that the condition must connect to)
	UAnimGraphNode_TransitionResult* ResultNode = nullptr;
	for (UEdGraphNode* Node : BoundGraph->Nodes)
	{
		if (UAnimGraphNode_TransitionResult* RN = Cast<UAnimGraphNode_TransitionResult>(Node))
		{
			ResultNode = RN;
			break;
		}
	}
	if (!ResultNode)
		return FMCPToolResult::Error(TEXT("Transition rule graph has no TransitionResult node — internal graph corruption"));

	// Find the bCanEnterTransition input pin on the result node
	UEdGraphPin* ResultPin = nullptr;
	for (UEdGraphPin* Pin : ResultNode->Pins)
	{
		if (Pin->Direction == EGPD_Input && Pin->PinName == TEXT("bCanEnterTransition"))
		{
			ResultPin = Pin;
			break;
		}
	}
	if (!ResultPin)
	{
		// Fallback: first input pin
		for (UEdGraphPin* Pin : ResultNode->Pins)
			if (Pin->Direction == EGPD_Input) { ResultPin = Pin; break; }
	}
	if (!ResultPin)
		return FMCPToolResult::Error(TEXT("TransitionResult has no input pin"));

	// always_true: unconditional transition — bCanEnterTransition defaults to true when unconnected.
	// Set it explicitly and return early; no variable getter needed.
	if (ConditionType.Equals(TEXT("always_true"), ESearchCase::IgnoreCase))
	{
		ResultPin->DefaultValue = TEXT("true");
		FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
		return FMCPToolResult::Success(FString::Printf(
			TEXT("Set transition condition '%s -> %s': always_true (fires unconditionally)"), *FromState, *ToState));
	}

	if (VarName.IsEmpty())
		return FMCPToolResult::Error(TEXT("Missing: variable (required for all condition types except always_true)"));

	// Validate the variable exists — check compiled GeneratedClass first, then uncompiled NewVariables
	// (Variables added with blueprint_add_variable don't appear in GeneratedClass until the BP is compiled)
	FProperty* VarProp = AnimBP->GeneratedClass
		? FindFProperty<FProperty>(AnimBP->GeneratedClass, FName(*VarName))
		: nullptr;
	if (!VarProp)
	{
		bool bFoundInNewVars = AnimBP->NewVariables.ContainsByPredicate(
			[&VarName](const FBPVariableDescription& Var)
			{ return Var.VarName.ToString().Equals(VarName, ESearchCase::IgnoreCase); });
		if (!bFoundInNewVars)
			return FMCPToolResult::Error(FString::Printf(
				TEXT("Variable '%s' not found on AnimBP. Add it with blueprint_add_variable first."), *VarName));
	}

	const UEdGraphSchema* GraphSchema = BoundGraph->GetSchema();
	bool bIsFloat = ConditionType.Equals(TEXT("variable_greater_than"), ESearchCase::IgnoreCase) ||
	                ConditionType.Equals(TEXT("variable_less_than"),     ESearchCase::IgnoreCase);

	// Create the variable getter node
	FGraphNodeCreator<UK2Node_VariableGet> GetterCreator(*BoundGraph);
	UK2Node_VariableGet* GetterNode = GetterCreator.CreateNode(false);
	GetterNode->VariableReference.SetSelfMember(FName(*VarName));
	GetterNode->NodePosX = ResultNode->NodePosX - (bIsFloat ? 600 : 300);
	GetterNode->NodePosY = ResultNode->NodePosY;
	GetterCreator.Finalize();

	UEdGraphPin* GetterValuePin = GetterNode->GetValuePin();
	if (!GetterValuePin)
		return FMCPToolResult::Error(FString::Printf(
			TEXT("Variable getter for '%s' produced no value pin — check variable type"), *VarName));

	if (ConditionType.Equals(TEXT("variable_true"), ESearchCase::IgnoreCase))
	{
		// Bool var is true — direct wire
		GraphSchema->TryCreateConnection(GetterValuePin, ResultPin);
	}
	else if (ConditionType.Equals(TEXT("variable_false"), ESearchCase::IgnoreCase))
	{
		// Bool var is false — invert with NOT
		FGraphNodeCreator<UK2Node_CallFunction> NotCreator(*BoundGraph);
		UK2Node_CallFunction* NotNode = NotCreator.CreateNode(false);
		NotNode->FunctionReference.SetExternalMember(
			GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool),
			UKismetMathLibrary::StaticClass());
		NotNode->NodePosX = ResultNode->NodePosX - 150;
		NotNode->NodePosY = ResultNode->NodePosY;
		NotCreator.Finalize();

		UEdGraphPin* NotInPin  = NotNode->FindPin(TEXT("a"));
		UEdGraphPin* NotOutPin = NotNode->FindPin(TEXT("ReturnValue"));
		if (NotInPin)  GraphSchema->TryCreateConnection(GetterValuePin, NotInPin);
		if (NotOutPin) GraphSchema->TryCreateConnection(NotOutPin, ResultPin);
	}
	else if (bIsFloat)
	{
		// Float var compared to threshold
		bool bGreater = ConditionType.Equals(TEXT("variable_greater_than"), ESearchCase::IgnoreCase);
		FName FuncName = bGreater
			? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble)
			: GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble);

		FGraphNodeCreator<UK2Node_CallFunction> CmpCreator(*BoundGraph);
		UK2Node_CallFunction* CmpNode = CmpCreator.CreateNode(false);
		CmpNode->FunctionReference.SetExternalMember(FuncName, UKismetMathLibrary::StaticClass());
		CmpNode->NodePosX = ResultNode->NodePosX - 300;
		CmpNode->NodePosY = ResultNode->NodePosY;
		CmpCreator.Finalize();

		UEdGraphPin* APin        = CmpNode->FindPin(TEXT("A"));
		UEdGraphPin* BPin        = CmpNode->FindPin(TEXT("B"));
		UEdGraphPin* CmpOutPin   = CmpNode->FindPin(TEXT("ReturnValue"));

		if (APin) GraphSchema->TryCreateConnection(GetterValuePin, APin);
		if (BPin && !ValueStr.IsEmpty()) BPin->DefaultValue = ValueStr;
		if (CmpOutPin) GraphSchema->TryCreateConnection(CmpOutPin, ResultPin);
	}
	else
	{
		return FMCPToolResult::Error(FString::Printf(
			TEXT("Unknown condition_type '%s'. Valid: variable_true, variable_false, variable_greater_than, variable_less_than"),
			*ConditionType));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	FString ExtraPart = bIsFloat ? FString::Printf(TEXT(", %s"), *ValueStr) : FString();
	return FMCPToolResult::Success(FString::Printf(
		TEXT("Set transition condition '%s -> %s': %s(%s%s)"),
		*FromState, *ToState, *ConditionType, *VarName, *ExtraPart));
}

// ---------------------------------------------------------------------------
// anim_set_entry_state
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimSetEntryState(const TSharedPtr<FJsonObject>& Params)
{
	FString AnimBPPath, SMName, StateName;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"), AnimBPPath)) return FMCPToolResult::Error(TEXT("Missing: anim_bp_path"));
	if (!Params->TryGetStringField(TEXT("sm_name"),      SMName))     return FMCPToolResult::Error(TEXT("Missing: sm_name"));
	if (!Params->TryGetStringField(TEXT("state_name"),   StateName))  return FMCPToolResult::Error(TEXT("Missing: state_name"));

	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPPath));

	UAnimationStateMachineGraph* SMGraph = FindSMGraph(AnimBP, SMName);
	if (!SMGraph)
		return FMCPToolResult::Error(FString::Printf(TEXT("State machine '%s' not found"), *SMName));

	UAnimStateEntryNode* EntryNode = nullptr;
	UAnimStateNode*      TargetState = nullptr;
	for (UEdGraphNode* N : SMGraph->Nodes)
	{
		if (!EntryNode)   EntryNode   = Cast<UAnimStateEntryNode>(N);
		if (!TargetState)
		{
			if (UAnimStateNode* SN = Cast<UAnimStateNode>(N))
				if (SN->GetStateName() == StateName) TargetState = SN;
		}
		if (EntryNode && TargetState) break;
	}
	if (!EntryNode)    return FMCPToolResult::Error(TEXT("State machine has no Entry node"));
	if (!TargetState)  return FMCPToolResult::Error(FString::Printf(TEXT("State '%s' not found in '%s'"), *StateName, *SMName));

	// Break any existing entry connection first
	for (UEdGraphPin* Pin : EntryNode->Pins)
		if (Pin->Direction == EGPD_Output) { Pin->BreakAllPinLinks(); break; }

	const UEdGraphSchema* Schema = SMGraph->GetSchema();
	UEdGraphPin* EntryOut = nullptr; UEdGraphPin* StateIn = nullptr;
	for (UEdGraphPin* P : EntryNode->Pins)  if (P->Direction == EGPD_Output) { EntryOut = P; break; }
	for (UEdGraphPin* P : TargetState->Pins) if (P->Direction == EGPD_Input)  { StateIn  = P; break; }

	if (!EntryOut || !StateIn)
		return FMCPToolResult::Error(TEXT("Could not find entry/state pins"));

	Schema->TryCreateConnection(EntryOut, StateIn);
	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	return FMCPToolResult::Success(FString::Printf(
		TEXT("Entry -> '%s' connected in state machine '%s'"), *StateName, *SMName));
}

// ---------------------------------------------------------------------------
// anim_create_locomotion_setup
// ---------------------------------------------------------------------------

FMCPToolResult FMCPAnimTools::AnimCreateLocomotionSetup(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required params ---
	FString AnimBPPath, IdlePath, WalkRunPath, JumpPath, FallPath, LandPath;
	if (!Params->TryGetStringField(TEXT("anim_bp_path"), AnimBPPath))  return FMCPToolResult::Error(TEXT("Missing: anim_bp_path"));
	if (!Params->TryGetStringField(TEXT("idle"),         IdlePath))    return FMCPToolResult::Error(TEXT("Missing: idle"));
	if (!Params->TryGetStringField(TEXT("walk_run"),     WalkRunPath)) return FMCPToolResult::Error(TEXT("Missing: walk_run"));
	if (!Params->TryGetStringField(TEXT("jump"),         JumpPath))    return FMCPToolResult::Error(TEXT("Missing: jump"));
	if (!Params->TryGetStringField(TEXT("fall"),         FallPath))    return FMCPToolResult::Error(TEXT("Missing: fall"));
	if (!Params->TryGetStringField(TEXT("land"),         LandPath))    return FMCPToolResult::Error(TEXT("Missing: land"));

	FString ThresholdStr;
	float SpeedThreshold = 10.f;
	if (Params->TryGetStringField(TEXT("speed_threshold"), ThresholdStr))
		SpeedThreshold = FCString::Atof(*ThresholdStr);

	// --- Load AnimBP ---
	UAnimBlueprint* AnimBP = LoadAnimBlueprint(AnimBPPath);
	if (!AnimBP)
		return FMCPToolResult::Error(FString::Printf(
			TEXT("AnimBP not found: %s — create it first with anim_create_anim_blueprint"), *AnimBPPath));

	// --- Add Speed (float) and IsInAir (bool) variables ---
	{
		FEdGraphPinType FloatType;
		FloatType.PinCategory    = UEdGraphSchema_K2::PC_Real;
		FloatType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		FBlueprintEditorUtils::AddMemberVariable(AnimBP, FName(TEXT("Speed")), FloatType);

		FEdGraphPinType BoolType;
		BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		FBlueprintEditorUtils::AddMemberVariable(AnimBP, FName(TEXT("IsInAir")), BoolType);
	}

	// --- Find main AnimGraph ---
	UAnimationGraph* AnimGraph = nullptr;
	for (UEdGraph* G : AnimBP->FunctionGraphs)
		if (UAnimationGraph* AG = Cast<UAnimationGraph>(G)) { AnimGraph = AG; break; }
	if (!AnimGraph)
		return FMCPToolResult::Error(TEXT("No AnimationGraph found — was the AnimBP created with anim_create_anim_blueprint?"));

	// --- Create LocomotionSM state machine ---
	FGraphNodeCreator<UAnimGraphNode_StateMachine> SMCreator(*AnimGraph);
	UAnimGraphNode_StateMachine* SMNode = SMCreator.CreateNode(false);
	SMNode->NodePosX = -300; SMNode->NodePosY = 0;
	SMCreator.Finalize();

	UAnimationStateMachineGraph* SMGraph = Cast<UAnimationStateMachineGraph>(SMNode->EditorStateMachineGraph);
	if (!SMGraph) return FMCPToolResult::Error(TEXT("SM graph creation failed"));
	SMGraph->Rename(TEXT("LocomotionSM"), nullptr, REN_DontCreateRedirectors);

	// Wire SM output to Output Pose
	UAnimGraphNode_Root* RootNode = nullptr;
	for (UEdGraphNode* N : AnimGraph->Nodes)
		if (UAnimGraphNode_Root* R = Cast<UAnimGraphNode_Root>(N)) { RootNode = R; break; }
	if (RootNode)
	{
		if (RootNode->Pins.Num() == 0) RootNode->AllocateDefaultPins();
		const UAnimationGraphSchema* ASch = CastChecked<UAnimationGraphSchema>(AnimGraph->GetSchema());
		UEdGraphPin* SMOut = nullptr; UEdGraphPin* RootIn = nullptr;
		for (UEdGraphPin* P : SMNode->Pins)  if (P->Direction == EGPD_Output) { SMOut = P; break; }
		for (UEdGraphPin* P : RootNode->Pins) if (P->Direction == EGPD_Input) { RootIn = P; break; }
		if (SMOut && RootIn) ASch->TryCreateConnection(SMOut, RootIn);
	}

	// --- Add 5 states (Idle first so Entry auto-connects to it) ---
	struct FStateInfo { FString Name; int32 X; int32 Y; };
	const TArray<FStateInfo> StateLayout = {
		{ TEXT("Idle"),    200,    0 },
		{ TEXT("WalkRun"), 500,    0 },
		{ TEXT("Jump"),    350, -200 },
		{ TEXT("Fall"),    550, -200 },
		{ TEXT("Land"),    750,    0 },
	};

	for (const FStateInfo& SI : StateLayout)
	{
		// Count existing states to respect auto-position logic, but use our explicit layout
		FGraphNodeCreator<UAnimStateNode> SC(*SMGraph);
		UAnimStateNode* SN = SC.CreateNode(false);
		SN->NodePosX = SI.X; SN->NodePosY = SI.Y;
		SC.Finalize();
		SN->OnRenameNode(SI.Name);

		// Auto-connect Entry → Idle (first state)
		if (SI.Name == TEXT("Idle"))
		{
			UAnimStateEntryNode* EntryNode = nullptr;
			for (UEdGraphNode* N : SMGraph->Nodes)
				if (UAnimStateEntryNode* EN = Cast<UAnimStateEntryNode>(N)) { EntryNode = EN; break; }
			if (EntryNode)
			{
				const UEdGraphSchema* SMSchema = SMGraph->GetSchema();
				UEdGraphPin* EOut = nullptr; UEdGraphPin* SIn = nullptr;
				for (UEdGraphPin* P : EntryNode->Pins) if (P->Direction == EGPD_Output) { EOut = P; break; }
				for (UEdGraphPin* P : SN->Pins)        if (P->Direction == EGPD_Input)  { SIn  = P; break; }
				if (EOut && SIn) SMSchema->TryCreateConnection(EOut, SIn);
			}
		}
	}

	// --- Assign animations ---
	auto SetAnim = [&](const FString& State, const FString& AssetPath) -> FMCPToolResult
	{
		auto P = MakeShared<FJsonObject>();
		P->SetStringField(TEXT("anim_bp_path"),    AnimBPPath);
		P->SetStringField(TEXT("sm_name"),         TEXT("LocomotionSM"));
		P->SetStringField(TEXT("state_name"),      State);
		P->SetStringField(TEXT("animation_asset"), AssetPath);
		return AnimSetStateAnimation(P);
	};
	FMCPToolResult R;
	R = SetAnim(TEXT("Idle"),    IdlePath);    if (!R.bSuccess) return R;
	R = SetAnim(TEXT("WalkRun"), WalkRunPath); if (!R.bSuccess) return R;
	R = SetAnim(TEXT("Jump"),    JumpPath);    if (!R.bSuccess) return R;
	R = SetAnim(TEXT("Fall"),    FallPath);    if (!R.bSuccess) return R;
	R = SetAnim(TEXT("Land"),    LandPath);    if (!R.bSuccess) return R;

	// --- Add transitions ---
	auto AddTrans = [&](const FString& From, const FString& To) -> FMCPToolResult
	{
		auto P = MakeShared<FJsonObject>();
		P->SetStringField(TEXT("anim_bp_path"), AnimBPPath);
		P->SetStringField(TEXT("sm_name"),      TEXT("LocomotionSM"));
		P->SetStringField(TEXT("from_state"),   From);
		P->SetStringField(TEXT("to_state"),     To);
		return AnimAddTransition(P);
	};
	R = AddTrans(TEXT("Idle"),    TEXT("WalkRun")); if (!R.bSuccess) return R;
	R = AddTrans(TEXT("WalkRun"), TEXT("Idle"));    if (!R.bSuccess) return R;
	R = AddTrans(TEXT("Idle"),    TEXT("Jump"));    if (!R.bSuccess) return R;
	R = AddTrans(TEXT("WalkRun"), TEXT("Jump"));    if (!R.bSuccess) return R;
	R = AddTrans(TEXT("Jump"),    TEXT("Fall"));    if (!R.bSuccess) return R;
	R = AddTrans(TEXT("Fall"),    TEXT("Land"));    if (!R.bSuccess) return R;
	R = AddTrans(TEXT("Land"),    TEXT("Idle"));    if (!R.bSuccess) return R;

	// --- Compile before setting conditions (variable getter needs compiled FProperty) ---
	FKismetEditorUtilities::CompileBlueprint(AnimBP, EBlueprintCompileOptions::None);

	// --- Set transition conditions ---
	FString ThreshStr = FString::Printf(TEXT("%f"), SpeedThreshold);
	auto SetCond = [&](const FString& From, const FString& To,
	                   const FString& Type, const FString& Var = TEXT(""), const FString& Val = TEXT("")) -> FMCPToolResult
	{
		auto P = MakeShared<FJsonObject>();
		P->SetStringField(TEXT("anim_bp_path"),   AnimBPPath);
		P->SetStringField(TEXT("sm_name"),        TEXT("LocomotionSM"));
		P->SetStringField(TEXT("from_state"),     From);
		P->SetStringField(TEXT("to_state"),       To);
		P->SetStringField(TEXT("condition_type"), Type);
		if (!Var.IsEmpty()) P->SetStringField(TEXT("variable"), Var);
		if (!Val.IsEmpty()) P->SetStringField(TEXT("value"),    Val);
		return AnimSetTransitionCondition(P);
	};
	R = SetCond(TEXT("Idle"),    TEXT("WalkRun"), TEXT("variable_greater_than"), TEXT("Speed"),   ThreshStr); if (!R.bSuccess) return R;
	R = SetCond(TEXT("WalkRun"), TEXT("Idle"),    TEXT("variable_less_than"),    TEXT("Speed"),   ThreshStr); if (!R.bSuccess) return R;
	R = SetCond(TEXT("Idle"),    TEXT("Jump"),    TEXT("variable_true"),          TEXT("IsInAir"));            if (!R.bSuccess) return R;
	R = SetCond(TEXT("WalkRun"), TEXT("Jump"),    TEXT("variable_true"),          TEXT("IsInAir"));            if (!R.bSuccess) return R;
	R = SetCond(TEXT("Jump"),    TEXT("Fall"),    TEXT("variable_true"),          TEXT("IsInAir"));            if (!R.bSuccess) return R;
	R = SetCond(TEXT("Fall"),    TEXT("Land"),    TEXT("variable_false"),         TEXT("IsInAir"));            if (!R.bSuccess) return R;
	R = SetCond(TEXT("Land"),    TEXT("Idle"),    TEXT("always_true"));                                        if (!R.bSuccess) return R;

	// --- Wire EventGraph (BlueprintUpdateAnimation → Speed / IsInAir update) ---
	bool bEventGraphWired = false;
	{
		UEdGraph* EventGraph = AnimBP->UbergraphPages.Num() > 0 ? AnimBP->UbergraphPages[0] : nullptr;
		if (EventGraph)
		{
			const UEdGraphSchema_K2* K2Schema = CastChecked<UEdGraphSchema_K2>(EventGraph->GetSchema());

			// Find or create the BlueprintUpdateAnimation event node
			UK2Node_Event* EventNode = nullptr;
			static const FName UpdateAnimFuncName = GET_FUNCTION_NAME_CHECKED(UAnimInstance, BlueprintUpdateAnimation);
			for (UEdGraphNode* N : EventGraph->Nodes)
				if (UK2Node_Event* E = Cast<UK2Node_Event>(N))
					if (E->EventReference.GetMemberName() == UpdateAnimFuncName)
					{ EventNode = E; break; }

			if (!EventNode)
			{
				FGraphNodeCreator<UK2Node_Event> EvCreator(*EventGraph);
				EventNode = EvCreator.CreateNode(false);
				EventNode->EventReference.SetExternalMember(UpdateAnimFuncName, UAnimInstance::StaticClass());
				EventNode->bOverrideFunction = true;
				EventNode->NodePosX = 0; EventNode->NodePosY = 0;
				EvCreator.Finalize();
			}

			// Helper: create a call function node
			auto MakeCallNode = [&](UClass* Class, const TCHAR* FuncName, int32 X, int32 Y) -> UK2Node_CallFunction*
			{
				if (!Class) return nullptr;
				UFunction* Func = Class->FindFunctionByName(FuncName);
				if (!Func) return nullptr;
				FGraphNodeCreator<UK2Node_CallFunction> C(*EventGraph);
				UK2Node_CallFunction* Node = C.CreateNode(false);
				Node->SetFromFunction(Func);
				Node->NodePosX = X; Node->NodePosY = Y;
				C.Finalize();
				return Node;
			};

			// Speed path: TryGetPawnOwner → GetVelocity → (VSize pure) → SetSpeed
			UK2Node_CallFunction* TryGetPawnNode = MakeCallNode(UAnimInstance::StaticClass(),      TEXT("TryGetPawnOwner"),      250,   0);
			UK2Node_CallFunction* GetVelNode     = MakeCallNode(AActor::StaticClass(),             TEXT("GetVelocity"),          500,   0);
			UK2Node_CallFunction* VSizeNode      = MakeCallNode(UKismetMathLibrary::StaticClass(), TEXT("VSize"),                700,   0);

			FGraphNodeCreator<UK2Node_VariableSet> SetSpeedCreator(*EventGraph);
			UK2Node_VariableSet* SetSpeedNode = SetSpeedCreator.CreateNode(false);
			SetSpeedNode->VariableReference.SetSelfMember(FName(TEXT("Speed")));
			SetSpeedNode->NodePosX = 900; SetSpeedNode->NodePosY = 0;
			SetSpeedCreator.Finalize();

			// IsInAir path: TryGetPawnOwner → GetMovementComponent → IsFalling → SetIsInAir
			UK2Node_CallFunction* TryGetPawnNode2 = MakeCallNode(UAnimInstance::StaticClass(),      TEXT("TryGetPawnOwner"),     250, 250);
			UK2Node_CallFunction* GetMovCompNode  = MakeCallNode(APawn::StaticClass(),              TEXT("GetMovementComponent"),500, 250);
			UK2Node_CallFunction* IsFallingNode   = MakeCallNode(UMovementComponent::StaticClass(), TEXT("IsFalling"),           700, 250);

			FGraphNodeCreator<UK2Node_VariableSet> SetIsInAirCreator(*EventGraph);
			UK2Node_VariableSet* SetIsInAirNode = SetIsInAirCreator.CreateNode(false);
			SetIsInAirNode->VariableReference.SetSelfMember(FName(TEXT("IsInAir")));
			SetIsInAirNode->NodePosX = 900; SetIsInAirNode->NodePosY = 250;
			SetIsInAirCreator.Finalize();

			// Exec chain (VSize is pure — no exec pins, skip in chain)
			auto ConnExec = [&](UEdGraphNode* From, UEdGraphNode* To)
			{
				if (!From || !To) return;
				UEdGraphPin* Out = From->FindPin(UEdGraphSchema_K2::PN_Then);
				UEdGraphPin* In  = To->FindPin(UEdGraphSchema_K2::PN_Execute);
				if (Out && In) K2Schema->TryCreateConnection(Out, In);
			};
			ConnExec(EventNode,        TryGetPawnNode);
			ConnExec(TryGetPawnNode,   GetVelNode);
			ConnExec(GetVelNode,       SetSpeedNode);
			ConnExec(SetSpeedNode,     TryGetPawnNode2);
			ConnExec(TryGetPawnNode2,  GetMovCompNode);
			ConnExec(GetMovCompNode,   IsFallingNode);
			ConnExec(IsFallingNode,    SetIsInAirNode);

			// Data: TryGetPawn.Return → GetVelocity.Target
			if (TryGetPawnNode && GetVelNode)
			{
				UEdGraphPin* PawnRet   = TryGetPawnNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				UEdGraphPin* VelTarget = GetVelNode->FindPin(UEdGraphSchema_K2::PN_Self);
				if (PawnRet && VelTarget) K2Schema->TryCreateConnection(PawnRet, VelTarget);
			}
			// GetVelocity.Return → VSize.A
			if (GetVelNode && VSizeNode)
			{
				UEdGraphPin* VelRet = GetVelNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				UEdGraphPin* SizeA  = VSizeNode->FindPin(TEXT("A"));
				if (VelRet && SizeA) K2Schema->TryCreateConnection(VelRet, SizeA);
			}
			// VSize.Return → SetSpeed.Speed
			if (VSizeNode && SetSpeedNode)
			{
				UEdGraphPin* SizeRet  = VSizeNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				UEdGraphPin* SpeedPin = SetSpeedNode->FindPin(TEXT("Speed"));
				if (SizeRet && SpeedPin) K2Schema->TryCreateConnection(SizeRet, SpeedPin);
			}
			// TryGetPawn2.Return → GetMovComp.Target
			if (TryGetPawnNode2 && GetMovCompNode)
			{
				UEdGraphPin* PawnRet2  = TryGetPawnNode2->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				UEdGraphPin* MovTarget = GetMovCompNode->FindPin(UEdGraphSchema_K2::PN_Self);
				if (PawnRet2 && MovTarget) K2Schema->TryCreateConnection(PawnRet2, MovTarget);
			}
			// GetMovComp.Return → IsFalling.Target
			if (GetMovCompNode && IsFallingNode)
			{
				UEdGraphPin* MovRet     = GetMovCompNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				UEdGraphPin* FallTarget = IsFallingNode->FindPin(UEdGraphSchema_K2::PN_Self);
				if (MovRet && FallTarget) K2Schema->TryCreateConnection(MovRet, FallTarget);
			}
			// IsFalling.Return → SetIsInAir.IsInAir
			if (IsFallingNode && SetIsInAirNode)
			{
				UEdGraphPin* FallRet = IsFallingNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				UEdGraphPin* AirPin  = SetIsInAirNode->FindPin(TEXT("IsInAir"));
				if (FallRet && AirPin) K2Schema->TryCreateConnection(FallRet, AirPin);
			}

			FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
			bEventGraphWired = true;
		}
	}

	// --- Final compile + save ---
	FKismetEditorUtilities::CompileBlueprint(AnimBP, EBlueprintCompileOptions::None);
	UPackage* Package = AnimBP->GetOutermost();
	if (Package)
	{
		Package->MarkPackageDirty();
		FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);
	}

	const TCHAR* EventGraphNote = bEventGraphWired
		? TEXT(" EventGraph wired: BlueprintUpdateAnimation -> TryGetPawnOwner -> VSize(GetVelocity) -> SetSpeed + IsFalling -> SetIsInAir.")
		: TEXT(" NOTE: No EventGraph found — wire Speed/IsInAir updates manually in the AnimBP.");
	return FMCPToolResult::Success(FString::Printf(
		TEXT("Locomotion setup complete on '%s'. States: Idle/WalkRun/Jump/Fall/Land with Speed (threshold %.1f) and IsInAir conditions.%s"),
		*AnimBPPath, SpeedThreshold, EventGraphNote));
}

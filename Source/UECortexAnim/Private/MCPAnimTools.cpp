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
#include "AnimStateNode.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
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
		T.Name = TEXT("anim_compile_anim_blueprint");
		T.Description = TEXT("Save an Animation Blueprint and mark it for recompile.");
		T.Params = { { TEXT("anim_bp_path"), TEXT("string"), TEXT("Content path to the Animation Blueprint"), true } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return AnimCompileAnimBlueprint(P); };
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

	// Create the state machine graph
	UAnimationStateMachineGraph* SMGraph = CastChecked<UAnimationStateMachineGraph>(
		FBlueprintEditorUtils::CreateNewGraph(
			AnimBP,
			FName(*SMName),
			UAnimationStateMachineGraph::StaticClass(),
			UAnimationStateMachineSchema::StaticClass()));

	// Create the state machine node in the anim graph
	UAnimGraphNode_StateMachine* SMNode = NewObject<UAnimGraphNode_StateMachine>(AnimGraph);
	SMNode->EditorStateMachineGraph = SMGraph;
	SMGraph->OwnerAnimGraphNode = SMNode;
	AnimGraph->AddNode(SMNode, false, false);

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

	UAnimStateNode* StateNode = NewObject<UAnimStateNode>(SMGraph);
	StateNode->OnRenameNode(StateName);
	SMGraph->AddNode(StateNode, false, false);

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

	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	UPackage* Package = AnimBP->GetOutermost();
	if (Package)
	{
		Package->MarkPackageDirty();
		FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);
	}
	return FMCPToolResult::Success(FString::Printf(TEXT("Marked '%s' for recompile and saved"), *AnimBPPath));
}

#include "MCPUMGTools.h"
#include "UECortexModule.h"
#include "MCPToolRegistry.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"
#include "Dom/JsonObject.h"
#include "Editor.h"

// ---------------------------------------------------------------------------
// Tool registration
// NOTE: HTTP handlers are called from the game thread tick (FHttpServerModule::ProcessRequests).
// Do NOT use AsyncTask(GameThread) + FEvent — that deadlocks because we are already
// on the game thread. Run all UE API calls directly, same as PCG/Niagara list tools.
// ---------------------------------------------------------------------------

void FMCPUMGTools::RegisterTools(TArray<FMCPToolDef>& OutTools)
{
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_list_widgets");
		T.Description = TEXT("List Widget Blueprint assets in the project.");
		T.Params = { { TEXT("filter"), TEXT("string"), TEXT("Optional substring filter on asset name"), false } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGListWidgets(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_create_widget");
		T.Description = TEXT("Create a new Widget Blueprint asset.");
		T.Params = {
			{ TEXT("path"),         TEXT("string"), TEXT("Content path, e.g. /Game/UI/WBP_HUD"), true },
			{ TEXT("parent_class"), TEXT("string"), TEXT("Parent class name (default: UserWidget)"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGCreateWidget(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_get_widget_tree");
		T.Description = TEXT("Get the full widget hierarchy of a Widget Blueprint.");
		T.Params = { { TEXT("widget_path"), TEXT("string"), TEXT("Content path to the Widget Blueprint"), true } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGGetWidgetTree(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_add_widget");
		T.Description = TEXT("Add a widget to a Widget Blueprint's tree. widget_class: TextBlock/Button/Image/CanvasPanel/VerticalBox/HorizontalBox/Border/Overlay.");
		T.Params = {
			{ TEXT("widget_path"),  TEXT("string"), TEXT("Content path to the Widget Blueprint"), true },
			{ TEXT("widget_class"), TEXT("string"), TEXT("Widget class to add"), true },
			{ TEXT("widget_name"),  TEXT("string"), TEXT("Name for the new widget"), true },
			{ TEXT("parent_name"),  TEXT("string"), TEXT("Name of parent panel widget (defaults to root)"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGAddWidget(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_set_text");
		T.Description = TEXT("Set the text on a TextBlock widget.");
		T.Params = {
			{ TEXT("widget_path"), TEXT("string"), TEXT("Content path to the Widget Blueprint"), true },
			{ TEXT("widget_name"), TEXT("string"), TEXT("Name of the TextBlock widget"), true },
			{ TEXT("text"),        TEXT("string"), TEXT("Text content to set"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGSetText(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_set_canvas_slot");
		T.Description = TEXT("Set canvas slot position and size for a widget inside a Canvas Panel.");
		T.Params = {
			{ TEXT("widget_path"), TEXT("string"), TEXT("Content path to the Widget Blueprint"), true },
			{ TEXT("widget_name"), TEXT("string"), TEXT("Name of the widget"), true },
			{ TEXT("x"),      TEXT("number"), TEXT("Left position in pixels"), false },
			{ TEXT("y"),      TEXT("number"), TEXT("Top position in pixels"), false },
			{ TEXT("width"),  TEXT("number"), TEXT("Width in pixels"), false },
			{ TEXT("height"), TEXT("number"), TEXT("Height in pixels"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGSetCanvasSlot(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_set_anchor");
		T.Description = TEXT("Set canvas slot anchors (0.0-1.0). Presets: top-left=(0,0,0,0), center=(0.5,0.5,0.5,0.5), full-stretch=(0,0,1,1).");
		T.Params = {
			{ TEXT("widget_path"), TEXT("string"), TEXT("Content path to the Widget Blueprint"), true },
			{ TEXT("widget_name"), TEXT("string"), TEXT("Name of the widget"), true },
			{ TEXT("min_x"), TEXT("number"), TEXT("Anchor minimum X (0-1)"), false },
			{ TEXT("min_y"), TEXT("number"), TEXT("Anchor minimum Y (0-1)"), false },
			{ TEXT("max_x"), TEXT("number"), TEXT("Anchor maximum X (0-1)"), false },
			{ TEXT("max_y"), TEXT("number"), TEXT("Anchor maximum Y (0-1)"), false },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGSetAnchor(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_set_visibility");
		T.Description = TEXT("Set Visible or Hidden on a widget.");
		T.Params = {
			{ TEXT("widget_path"), TEXT("string"),  TEXT("Content path to the Widget Blueprint"), true },
			{ TEXT("widget_name"), TEXT("string"),  TEXT("Name of the widget"), true },
			{ TEXT("visible"),     TEXT("boolean"), TEXT("True = Visible, false = Hidden"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGSetVisibility(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_add_variable");
		T.Description = TEXT("Add a variable to a Widget Blueprint.");
		T.Params = {
			{ TEXT("widget_path"), TEXT("string"), TEXT("Content path to the Widget Blueprint"), true },
			{ TEXT("var_name"),    TEXT("string"), TEXT("Variable name"), true },
			{ TEXT("var_type"),    TEXT("string"), TEXT("Type: bool/int/float/string/text/vector2d/linearcolor"), true },
		};
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGAddVariable(P); };
		OutTools.Add(T);
	}
	{
		FMCPToolDef T;
		T.Name = TEXT("umg_compile_widget");
		T.Description = TEXT("Save a Widget Blueprint and mark it for recompile.");
		T.Params = { { TEXT("widget_path"), TEXT("string"), TEXT("Content path to the Widget Blueprint"), true } };
		T.Handler = [](const TSharedPtr<FJsonObject>& P) { return UMGCompileWidget(P); };
		OutTools.Add(T);
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static UWidgetBlueprint* LoadWidgetBlueprint(const FString& Path)
{
	return Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *Path));
}

static TSharedPtr<FJsonObject> WidgetToJson(UWidget* Widget)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), Widget->GetName());
	Obj->SetStringField(TEXT("class"), Widget->GetClass()->GetName());

	if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		Obj->SetStringField(TEXT("text"), Text->GetText().ToString());

	TArray<TSharedPtr<FJsonValue>> Children;
	if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 i = 0; i < Panel->GetChildrenCount(); i++)
		{
			if (UWidget* Child = Panel->GetChildAt(i))
				Children.Add(MakeShared<FJsonValueObject>(WidgetToJson(Child)));
		}
	}
	Obj->SetArrayField(TEXT("children"), Children);
	return Obj;
}

static UClass* FindWidgetClass(const FString& ClassName)
{
	if (ClassName == TEXT("TextBlock"))     return UTextBlock::StaticClass();
	if (ClassName == TEXT("Button"))        return UButton::StaticClass();
	if (ClassName == TEXT("Image"))         return UImage::StaticClass();
	if (ClassName == TEXT("CanvasPanel"))   return UCanvasPanel::StaticClass();
	if (ClassName == TEXT("VerticalBox"))   return UVerticalBox::StaticClass();
	if (ClassName == TEXT("HorizontalBox")) return UHorizontalBox::StaticClass();
	if (ClassName == TEXT("Border"))        return UBorder::StaticClass();
	if (ClassName == TEXT("Overlay"))       return UOverlay::StaticClass();
	return FindObject<UClass>(nullptr, *(FString(TEXT("/Script/UMG.")) + ClassName));
}

static FEdGraphPinType PinTypeFromString(const FString& TypeName)
{
	FEdGraphPinType Pin;
	if (TypeName == TEXT("bool"))             { Pin.PinCategory = UEdGraphSchema_K2::PC_Boolean; }
	else if (TypeName == TEXT("int"))         { Pin.PinCategory = UEdGraphSchema_K2::PC_Int; }
	else if (TypeName == TEXT("float"))       { Pin.PinCategory = UEdGraphSchema_K2::PC_Real; Pin.PinSubCategory = UEdGraphSchema_K2::PC_Float; }
	else if (TypeName == TEXT("string"))      { Pin.PinCategory = UEdGraphSchema_K2::PC_String; }
	else if (TypeName == TEXT("text"))        { Pin.PinCategory = UEdGraphSchema_K2::PC_Text; }
	else if (TypeName == TEXT("vector2d"))    { Pin.PinCategory = UEdGraphSchema_K2::PC_Struct; Pin.PinSubCategoryObject = TBaseStructure<FVector2D>::Get(); }
	else if (TypeName == TEXT("linearcolor")) { Pin.PinCategory = UEdGraphSchema_K2::PC_Struct; Pin.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); }
	else                                      { Pin.PinCategory = UEdGraphSchema_K2::PC_Boolean; }
	return Pin;
}

// ---------------------------------------------------------------------------
// umg_list_widgets  (runs on calling thread — asset registry reads are thread-safe)
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGListWidgets(const TSharedPtr<FJsonObject>& Params)
{
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/UMGEditor"), TEXT("WidgetBlueprint")));
	ARFilter.bRecursivePaths = true;
	AR.GetAssets(ARFilter, Assets);

	TArray<TSharedPtr<FJsonValue>> WidgetArr;
	for (const FAssetData& Asset : Assets)
	{
		FString Name = Asset.AssetName.ToString();
		if (!Filter.IsEmpty() && !Name.Contains(Filter, ESearchCase::IgnoreCase)) continue;

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Name);
		Obj->SetStringField(TEXT("path"), Asset.GetSoftObjectPath().ToString());
		WidgetArr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetArrayField(TEXT("widgets"), WidgetArr);
	Response->SetNumberField(TEXT("count"), WidgetArr.Num());
	return FMCPToolResult::Success(FString::Printf(TEXT("Found %d widget blueprints"), WidgetArr.Num()), Response);
}

// ---------------------------------------------------------------------------
// umg_create_widget
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGCreateWidget(const TSharedPtr<FJsonObject>& Params)
{
	FString Path;
	if (!Params->TryGetStringField(TEXT("path"), Path))
		return FMCPToolResult::Error(TEXT("Missing required param: path"));

	FString ParentClassName = TEXT("UserWidget");
	Params->TryGetStringField(TEXT("parent_class"), ParentClassName);

	int32 LastSlash;
	if (!Path.FindLastChar(TEXT('/'), LastSlash))
		return FMCPToolResult::Error(TEXT("Invalid path — must be a full content path like /Game/UI/WBP_HUD"));
	FString AssetName = Path.Mid(LastSlash + 1);

	UClass* ParentClass = UUserWidget::StaticClass();
	if (ParentClassName != TEXT("UserWidget"))
	{
		if (UClass* Found = FindObject<UClass>(nullptr, *(FString(TEXT("/Script/UMG.")) + ParentClassName)))
			ParentClass = Found;
	}

	UPackage* Package = CreatePackage(*Path);
	UWidgetBlueprint* WBP = NewObject<UWidgetBlueprint>(
		Package, UWidgetBlueprint::StaticClass(), FName(*AssetName),
		RF_Public | RF_Standalone | RF_Transactional);
	WBP->ParentClass = ParentClass;
	WBP->WidgetTree = NewObject<UWidgetTree>(WBP, TEXT("WidgetTree"), RF_Transactional);

	FAssetRegistryModule::AssetCreated(WBP);
	Package->MarkPackageDirty();

	return FMCPToolResult::Success(FString::Printf(TEXT("Created Widget Blueprint '%s'"), *Path));
}

// ---------------------------------------------------------------------------
// umg_get_widget_tree
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGGetWidgetTree(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	UWidgetTree* Tree = WBP->WidgetTree;
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("widget_path"), WidgetPath);

	if (Tree && Tree->RootWidget)
		Response->SetObjectField(TEXT("root"), WidgetToJson(Tree->RootWidget));
	else
		Response->SetBoolField(TEXT("empty"), true);

	TArray<TSharedPtr<FJsonValue>> FlatList;
	if (Tree)
	{
		TArray<UWidget*> AllWidgets;
		Tree->GetAllWidgets(AllWidgets);
		for (UWidget* W : AllWidgets)
		{
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("name"), W->GetName());
			Obj->SetStringField(TEXT("class"), W->GetClass()->GetName());
			FlatList.Add(MakeShared<FJsonValueObject>(Obj));
		}
	}
	Response->SetArrayField(TEXT("all_widgets"), FlatList);
	Response->SetNumberField(TEXT("widget_count"), FlatList.Num());

	return FMCPToolResult::Success(
		FString::Printf(TEXT("Widget Blueprint '%s': %d widgets"), *WidgetPath, FlatList.Num()), Response);
}

// ---------------------------------------------------------------------------
// umg_add_widget
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGAddWidget(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath, WidgetClass, WidgetName, ParentName;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));
	if (!Params->TryGetStringField(TEXT("widget_class"), WidgetClass))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_class"));
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_name"));
	Params->TryGetStringField(TEXT("parent_name"), ParentName);

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	UClass* Class = FindWidgetClass(WidgetClass);
	if (!Class)
		return FMCPToolResult::Error(FString::Printf(TEXT("Unknown widget class: %s"), *WidgetClass));

	UWidgetTree* Tree = WBP->WidgetTree;
	UWidget* NewWidget = Tree->ConstructWidget<UWidget>(Class, FName(*WidgetName));

	if (!ParentName.IsEmpty())
	{
		UWidget* ParentWidget = Tree->FindWidget(FName(*ParentName));
		if (UPanelWidget* Panel = Cast<UPanelWidget>(ParentWidget))
			Panel->AddChild(NewWidget);
		else
			return FMCPToolResult::Error(FString::Printf(TEXT("Parent '%s' is not a panel or not found"), *ParentName));
	}
	else if (!Tree->RootWidget)
	{
		Tree->RootWidget = NewWidget;
	}
	else if (UPanelWidget* RootPanel = Cast<UPanelWidget>(Tree->RootWidget))
	{
		RootPanel->AddChild(NewWidget);
	}
	else
	{
		return FMCPToolResult::Error(TEXT("Root widget is not a panel — specify parent_name."));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	return FMCPToolResult::Success(FString::Printf(TEXT("Added %s '%s' to '%s'"), *WidgetClass, *WidgetName, *WidgetPath));
}

// ---------------------------------------------------------------------------
// umg_set_text
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGSetText(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath, WidgetName, Text;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_name"));
	if (!Params->TryGetStringField(TEXT("text"), Text))
		return FMCPToolResult::Error(TEXT("Missing required param: text"));

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	UTextBlock* TextBlock = Cast<UTextBlock>(WBP->WidgetTree->FindWidget(FName(*WidgetName)));
	if (!TextBlock)
		return FMCPToolResult::Error(FString::Printf(TEXT("'%s' not found or not a TextBlock"), *WidgetName));

	TextBlock->SetText(FText::FromString(Text));
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	return FMCPToolResult::Success(FString::Printf(TEXT("Set text on '%s' to \"%s\""), *WidgetName, *Text));
}

// ---------------------------------------------------------------------------
// umg_set_canvas_slot
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGSetCanvasSlot(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath, WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_name"));

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
	if (!Slot)
		return FMCPToolResult::Error(FString::Printf(TEXT("'%s' not found or not in a Canvas Panel"), *WidgetName));

	double X = Slot->GetPosition().X, Y = Slot->GetPosition().Y;
	double W = Slot->GetSize().X,     H = Slot->GetSize().Y;
	Params->TryGetNumberField(TEXT("x"), X);
	Params->TryGetNumberField(TEXT("y"), Y);
	Params->TryGetNumberField(TEXT("width"),  W);
	Params->TryGetNumberField(TEXT("height"), H);

	Slot->SetPosition(FVector2D(X, Y));
	Slot->SetSize(FVector2D(W, H));
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	return FMCPToolResult::Success(FString::Printf(
		TEXT("Set canvas slot on '%s': pos=(%.0f,%.0f) size=(%.0f,%.0f)"), *WidgetName, X, Y, W, H));
}

// ---------------------------------------------------------------------------
// umg_set_anchor
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGSetAnchor(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath, WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_name"));

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
	if (!Slot)
		return FMCPToolResult::Error(FString::Printf(TEXT("'%s' not found or not in a Canvas Panel"), *WidgetName));

	FAnchors Current = Slot->GetAnchors();
	double MinX = Current.Minimum.X, MinY = Current.Minimum.Y;
	double MaxX = Current.Maximum.X, MaxY = Current.Maximum.Y;
	Params->TryGetNumberField(TEXT("min_x"), MinX);
	Params->TryGetNumberField(TEXT("min_y"), MinY);
	Params->TryGetNumberField(TEXT("max_x"), MaxX);
	Params->TryGetNumberField(TEXT("max_y"), MaxY);

	Slot->SetAnchors(FAnchors((float)MinX, (float)MinY, (float)MaxX, (float)MaxY));
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	return FMCPToolResult::Success(FString::Printf(
		TEXT("Set anchors on '%s': min=(%.2f,%.2f) max=(%.2f,%.2f)"), *WidgetName, MinX, MinY, MaxX, MaxY));
}

// ---------------------------------------------------------------------------
// umg_set_visibility
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGSetVisibility(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath, WidgetName;
	bool bVisible = true;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_name"));
	Params->TryGetBoolField(TEXT("visible"), bVisible);

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget '%s' not found"), *WidgetName));

	Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	return FMCPToolResult::Success(FString::Printf(
		TEXT("Set visibility on '%s' to %s"), *WidgetName, bVisible ? TEXT("Visible") : TEXT("Hidden")));
}

// ---------------------------------------------------------------------------
// umg_add_variable
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGAddVariable(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath, VarName, VarType;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));
	if (!Params->TryGetStringField(TEXT("var_name"), VarName))
		return FMCPToolResult::Error(TEXT("Missing required param: var_name"));
	if (!Params->TryGetStringField(TEXT("var_type"), VarType))
		return FMCPToolResult::Error(TEXT("Missing required param: var_type"));

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	FEdGraphPinType PinType = PinTypeFromString(VarType.ToLower());
	FBlueprintEditorUtils::AddMemberVariable(WBP, FName(*VarName), PinType);
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	return FMCPToolResult::Success(FString::Printf(TEXT("Added variable '%s' (%s) to '%s'"), *VarName, *VarType, *WidgetPath));
}

// ---------------------------------------------------------------------------
// umg_compile_widget
// ---------------------------------------------------------------------------

FMCPToolResult FMCPUMGTools::UMGCompileWidget(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
		return FMCPToolResult::Error(TEXT("Missing required param: widget_path"));

	UWidgetBlueprint* WBP = LoadWidgetBlueprint(WidgetPath);
	if (!WBP)
		return FMCPToolResult::Error(FString::Printf(TEXT("Widget Blueprint not found: %s"), *WidgetPath));

	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	UPackage* Package = WBP->GetOutermost();
	if (Package)
	{
		Package->MarkPackageDirty();
		FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);
	}
	return FMCPToolResult::Success(FString::Printf(TEXT("Marked '%s' for recompile and saved"), *WidgetPath));
}

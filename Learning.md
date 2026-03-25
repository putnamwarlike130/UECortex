# UECortex — Lessons Learned

Build and debug log for the UECortex MCP server plugin. Each entry is an issue we hit and how we resolved it.

---

## Build & Linking

### Missing Json/JsonUtilities in submodule Build.cs
**Issue:** LNK2019 unresolved externals for `FJsonValue`, `FJsonObject` in UECortexGAS and UECortexPCG.
**Fix:** Add `"Json"` and `"JsonUtilities"` to `PrivateDependencyModuleNames` in every submodule's Build.cs. Easy to forget since the main module already has them.

### HTTP and HTTPServer are separate modules
**Issue:** Linker errors when only `"HTTP"` was in Build.cs.
**Fix:** Both `"HTTP"` and `"HTTPServer"` must be listed. They are distinct modules despite being related.

### Full rebuild required for new symbols in main module
**Issue:** After adding a new exported symbol (e.g. `UnregisterModule`) to the main UECortex DLL, submodules got LNK2019 even after live coding.
**Fix:** Live coding only patches changed files. When the main module's public API changes, do a full rebuild (close UE, rebuild from Visual Studio).

### BlueprintGraph missing from UECortexUMG Build.cs
**Issue:** `UEdGraphSchema_K2::PC_Boolean` and related symbols unresolved.
**Fix:** `UEdGraphSchema_K2` lives in the `BlueprintGraph` module. Add it to `PrivateDependencyModuleNames`.

---

## Plugin / Module Declaration

### Optional plugins must be declared in .uplugin
**Issue:** No plugin dependency warnings at startup; plugin tools silently unavailable.
**Fix:** Add to `.uplugin` Plugins section: `{ "Name": "FooPlugin", "Enabled": false, "Optional": true }`. Required for: GameplayAbilities, PCG, Niagara, EnhancedInput.

### UMG, AnimGraph, LevelSequence cannot be listed in .uplugin
**Issue:** Adding `{ "Name": "UMG" }` or `{ "Name": "LevelSequence" }` to .uplugin Plugins section causes:
`Unable to find plugin 'UMG'. Install it and try again.`
**Fix:** These are built-in engine modules/plugins — they are always present and cannot be declared as dependencies. Only external plugins (Niagara, PCG, GameplayAbilities) go in the Plugins section.

---

## Threading — Critical

### AsyncTask(GameThread) + FEvent::Wait() = deadlock
**Issue:** All UMG write tools hung the editor permanently on the first call.
**Root cause:** `FHttpServerModule::ProcessRequests()` is called from the **game thread tick**. HTTP handlers already execute on the game thread. Calling `AsyncTask(ENamedThreads::GameThread, lambda)` then `Event->Wait()` blocks the game thread — the lambda can never run because the thread it needs is blocked.
**Fix:** Remove all `AsyncTask + Wait` patterns. Call UE APIs directly in handlers — you are already on the game thread.
**Rule:** Fire-and-forget `AsyncTask(GameThread)` without a Wait is fine. Never pair it with `FEvent::Wait()` inside a handler.

---

## Blueprint / Asset Creation

### FKismetEditorUtilities::CreateBlueprint deadlocks
**Issue:** Calling `FKismetEditorUtilities::CreateBlueprint()` inside an HTTP handler hung the editor.
**Root cause:** Internally schedules a compilation task on the game thread — self-deadlock (see threading rule above).
**Fix:** Use `NewObject<UWidgetBlueprint>()` directly with `RF_Public | RF_Standalone | RF_Transactional`, then call `FAssetRegistryModule::AssetCreated()` and `Package->MarkPackageDirty()`.

### IAssetTools::CreateAsset shows a dialog
**Issue:** `IAssetTools::CreateAsset()` opened a UI dialog, blocking the handler.
**Fix:** Replace with direct `CreatePackage()` + `NewObject<T>()` pattern. No dialogs.

### Package->FullyLoad() deadlocks
**Issue:** Calling `Package->FullyLoad()` inside a handler hung the editor.
**Root cause:** Dispatches IO tasks and waits — same game thread deadlock.
**Fix:** Remove it entirely. Assets created with `NewObject` don't need FullyLoad.

### FKismetEditorUtilities::CompileBlueprint deadlocks
**Issue:** Calling `FKismetEditorUtilities::CompileBlueprint()` hung the editor.
**Fix:** Use `FBlueprintEditorUtils::MarkBlueprintAsModified()` + `FEditorFileUtils::SaveDirtyPackages()` instead.

---

## UMG Specific

### Wrong header path for WidgetBlueprintFactory
**Issue:** `#include "Factories/WidgetBlueprintFactory.h"` — file not found.
**Fix:** `#include "WidgetBlueprintFactory.h"` (no `Factories/` prefix).

### GetTools() vs RegisterTools()
**Issue:** `virtual TArray<FMCPToolDef> GetTools() const override` — compiler error, no such virtual method.
**Fix:** Base class defines `virtual void RegisterTools(TArray<FMCPToolDef>& OutTools)`. Override that.

### UCanvasPanelSlot::LayoutData deprecated
**Issue:** Deprecated access to `LayoutData` struct members.
**Fix:** Use accessor methods: `Slot->GetAnchors()`, `Slot->GetPosition()`, etc.

---

## FMCPToolDef Registration

### Brace initialization fails
**Issue:** `OutTools.Add({ name, desc, lambda })` — compiler error, can't initialize with 3 args.
**Root cause:** `FMCPToolDef` contains `TArray<FMCPParamSchema> Params` — not aggregate-initializable.
**Fix:** Use the scoped block pattern:
```cpp
{
    FMCPToolDef T;
    T.Name = TEXT("foo_bar");
    T.Description = TEXT("...");
    T.Handler = [](const TSharedPtr<FJsonObject>& P) { return FooTools::Bar(P); };
    OutTools.Add(T);
}
```

### FMCPToolResult::Ok doesn't exist
**Issue:** `FMCPToolResult::Ok(...)` — identifier not found.
**Fix:** The correct methods are `FMCPToolResult::Success(TEXT("..."))` and `FMCPToolResult::Error(TEXT("..."))`.

---

## FTopLevelAssetPath

### Class initialization deadlock in asset registry filters
**Issue:** Using `UFoo::StaticClass()->GetClassPathName()` inside a handler caused a deadlock (triggers class initialization on game thread while already there).
**Fix:** Use hardcoded `FTopLevelAssetPath` strings:
```cpp
FTopLevelAssetPath(TEXT("/Script/UMGEditor"),   TEXT("WidgetBlueprint"))
FTopLevelAssetPath(TEXT("/Script/Engine"),       TEXT("AnimBlueprint"))
FTopLevelAssetPath(TEXT("/Script/Engine"),       TEXT("Skeleton"))
FTopLevelAssetPath(TEXT("/Script/CoreUObject"),  TEXT("ObjectRedirector"))
```

---

## AnimGraph Headers (UE 5.6)

### AnimStateMachineGraph.h not found
**Issue:** `#include "AnimStateMachineGraph.h"` — file not found.
**Fix:** The correct name in UE 5.6 is `AnimationStateMachineGraph.h`. Same for `AnimationStateMachineSchema.h`. The class names also have the `Animation` prefix: `UAnimationStateMachineGraph`, `UAnimationStateMachineSchema`.

### SetStateName doesn't exist on state nodes
**Issue:** `StateNode->SetStateName(FName(...))` — no such method.
**Fix:** Use `StateNode->OnRenameNode(FString(...))`.

---

## Niagara

### SetNiagaraVariableFloat FName overload missing in UE 5.6
**Issue:** `UNiagaraComponent::SetNiagaraVariableFloat(FName, float)` — no matching function.
**Fix:** In UE 5.6, only the `FString` overload exists: `SetNiagaraVariableFloat(FString, float)`.

### FNiagaraVariable constructor needs FName not FString
**Issue:** Passing `FString` to `FNiagaraVariable` constructor — no matching overload.
**Fix:** Wrap with `FName(*ParamName)`.

---

## Sequencer

### FMovieSceneObjectBindingID wrong constructor
**Issue:** `FMovieSceneObjectBindingID(CameraGuid, MovieSceneSequenceID::Root)` — does not take 2 arguments.
**Fix:** Check the actual constructor signature; binding IDs are set via their own factory methods, not direct construction with a Guid + SequenceID pair.

### ACineCameraActor — wrong prefix and wrong construction
**Issue (1):** Used `UCineCameraActor` — undeclared identifier.
**Fix:** Actors use the `A` prefix: `ACineCameraActor`.
**Issue (2):** Used `NewObject<ACineCameraActor>()` — actors cannot be created with NewObject.
**Fix:** Use `ACineCameraActor::StaticClass()->GetDefaultObject()` as the template for `UMovieScene::AddSpawnable()`.

---

## Python Executor

### ExecuteStatement mode doesn't support multi-line code
**Issue:** `EPythonCommandExecutionMode::ExecuteStatement` only handles a single statement. Multi-line scripts returned `SyntaxError: multiple statements`.
**Fix:** Write code to a temp `.py` file and execute with `EPythonCommandExecutionMode::ExecuteFile`.

### Private file execution scope — variables don't persist
**Issue:** After executing the temp file, trying to read `_mcp_result` in a follow-up `EvaluateStatement` call returned `NameError: name '_mcp_result' is not defined`.
**Root cause:** `EPythonFileExecutionScope::Private` runs the file in an isolated scope.
**Fix:** Use `EPythonFileExecutionScope::Public` so variables written in the file persist in `__main__`.
**Better fix:** Print `_mcp_result` at the end of the boot code so it's captured in the same output — no separate eval needed.

### JSON params with newlines break Python string literal injection
**Issue:** Pretty-printed JSON (from `TJsonWriterFactory<>`) contains newlines, which broke inline string literal injection into Python code: `_mcp_params = json.loads('{\n\t...'}`.
**Fix:** Write params JSON to a separate temp file. Boot code reads it with `json.load(open(...))`. No escaping issues.

---

## PCG

### PCGDensityFilterSettings property names
**Issue:** `LowThreshold` / `HighThreshold` — property not found.
**Fix:** Actual property names are `LowerBound` and `UpperBound`. Always use `pcg_get_node_params` to discover real property names before setting them.

### PCG SurfaceSampler doesn't work with static mesh floors
**Issue:** PCG volume placed over a static mesh floor produced no visible output — surface sampler expects landscape collision.
**Fix:** For non-landscape indoor environments, scatter via Python using noise-based point generation instead of the PCG surface sampler.

---

## Asset Saving

### UPackage::Save() / FSavePackageArgs undefined
**Issue:** Direct `UPackage::Save()` calls and `FSavePackageArgs` not available in this build configuration.
**Fix:** Always use `FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false)`.

---

## Level / World

### New actors placed on top of existing world geometry
**Issue:** Spawning actors at origin (0,0,0) overlapped with existing level content.
**Fix:** Always query existing actor bounds first (`get_all_level_actors()` → compute min/max extents), then place new content beyond `max_x + clearance`.

---

## MCP Registration

### claude mcp add (project scope) not visible in `claude mcp list`
**Issue:** `claude mcp add` defaults to project scope. `claude mcp list` only shows user-scope servers.
**Fix:** Use `--scope user` to register globally: `claude mcp add uecortex --transport http http://localhost:7777/mcp --scope user`.

### MCP tools not available in same session as registration
**Issue:** After running `claude mcp add`, the tools didn't appear in the current session.
**Fix:** MCP servers are loaded at session start. Start a new Claude Code session after registering.

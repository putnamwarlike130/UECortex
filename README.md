# UECortex

Native Unreal Engine 5 plugin that runs a full [MCP (Model Context Protocol)](https://modelcontextprotocol.io) server inside the Unreal Editor. Control your project directly from Claude Code, VS Code, or any MCP client — no Python sidecar, no Node.js process, no external dependencies.

**142 tools** across 15 categories. Pure C++ core, with a Python-backed dynamic tool layer.

---

## Quick Start

### 1. Install

Copy the `UECortex` folder into your project's `Plugins/` directory and enable it in your `.uproject`:

```json
{
  "Name": "UECortex",
  "Enabled": true
}
```

Reopen the project. The server starts automatically on editor launch.

### 2. Connect

**Claude Code:**
```bash
claude mcp add uecortex --transport http http://localhost:7777/mcp
```

**VS Code** (`.vscode/mcp.json`):
```json
{ "servers": { "uecortex": { "url": "http://localhost:7777/mcp" } } }
```

**Health check:**
```
GET http://localhost:7777/health
```

---

## Tool Categories

### Level & Actors
`actor_spawn` `actor_spawn_on_surface` `actor_delete` `actor_duplicate` `actor_set_transform` `actor_set_property` `actor_get_properties` `actor_get_selected` `actor_list` `actor_find_by_name` `level_get_info` `line_trace` `viewport_focus` `viewport_set_camera` `console_command`

### Blueprints
`blueprint_list` `blueprint_get_graph` `blueprint_add_node` `blueprint_connect_pins` `blueprint_set_property` `blueprint_compile` `blueprint_add_component` ...

### C++ Codegen
`cpp_create_class` `cpp_add_property` `cpp_add_function` `cpp_hot_reload` ...

### Rendering
`rendering_set_nanite` `rendering_set_lumen` `material_create` `material_set_parameter` `material_compile` ...

### Splines & Volumes
`spline_create` `spline_add_point` `spline_get_info` `volume_create` `volume_set_bounds` ...

### PCG *(requires PCG plugin)*
`pcg_create_graph` `pcg_add_node` `pcg_connect_pins` `pcg_get_graph` `pcg_list_node_types` `pcg_execute` ...

### Gameplay Ability System *(requires GameplayAbilities plugin)*
`gas_create_ability` `gas_add_attribute_set` `gas_grant_ability` `gas_apply_effect` `gas_list_abilities` ...

### Niagara
`niagara_list_systems` `niagara_create_system` `niagara_add_emitter` `niagara_set_parameter` `niagara_compile` ...

### Animation
`anim_list_skeletons` `anim_list_skeletal_meshes` `anim_get_skeleton_bones` `anim_create_anim_blueprint` `anim_add_state_machine` `anim_add_state` `anim_get_anim_graph` `anim_compile_anim_blueprint` ...

### UMG Widgets
`umg_list_widgets` `umg_create_widget` `umg_get_widget_tree` `umg_add_widget` `umg_set_text` `umg_set_canvas_slot` `umg_set_anchor` `umg_set_visibility` `umg_add_variable` `umg_compile_widget`

### Sequencer
`seq_create` `seq_list` `seq_get_info` `seq_set_playback_range` `seq_add_actor_binding` `seq_add_transform_track` `seq_add_keyframe` `seq_add_camera_cut` `seq_add_spawnable_camera` `seq_add_visibility_track`

### Asset Management
`asset_list` `asset_get_info` `asset_get_references` `asset_get_dependencies` `asset_move` `asset_copy` `asset_delete` `asset_create_folder` `asset_list_redirectors` `asset_fix_redirectors`

### Viewport & Input *(Windows/macOS)*
`viewport_capture` `editor_input`

`viewport_capture` captures the 3D viewport or the full editor window as a PNG image. `editor_input` simulates mouse clicks and keyboard input on the editor window — useful for driving UI flows like the Fab browser search bar.

### Fab Marketplace
`fab_login_status` `fab_search` `fab_get_asset`

`fab_login_status` checks EOS login state (required for purchases). `fab_search` and `fab_get_asset` are dynamic Python tools — load them once per session:

```bash
# Via MCP or curl:
python_reload_tools_from_file  →  Scripts/fab_tools.json
```

> **Purchase consent policy:** `fab_get_asset` is an information-only tool. Every response includes an explicit `CONSENT REQUIRED` notice. AI agents must never trigger a purchase, add-to-cart, or any payment action without explicit confirmation from the user.

### Python & Dynamic Tools
`python_exec` `python_exec_file` `python_check` `python_set_var` `python_get_var` `python_list_scripts` `python_register_tool` `python_unregister_tool` `python_list_dynamic_tools` `python_reload_tools_from_file`

---

## Dynamic Tool Registration

Register new tools at runtime — no recompile needed:

```json
{
  "name": "python_register_tool",
  "arguments": {
    "name": "my_custom_tool",
    "description": "Does something useful.",
    "code": "import unreal\nactors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()\n_mcp_result = f'{len(actors)} actors in scene'"
  }
}
```

The tool immediately appears in `tools/list` and is callable like any built-in tool. Tools receive their arguments as `_mcp_params` (dict) and return a result by setting `_mcp_result` (string).

Load an entire library from a JSON file:

```json
[
  { "name": "tool_a", "description": "...", "code": "..." },
  { "name": "tool_b", "description": "...", "code": "..." }
]
```

```json
{ "name": "python_reload_tools_from_file", "arguments": { "file_path": "Scripts/MCPTools.json" } }
```

---

## Optional Plugin Dependencies

Some tool categories only activate when the corresponding plugin is enabled in your project:

| Category | Plugin | How to enable |
|----------|--------|---------------|
| PCG | Procedural Content Generation | Enable in Plugins window |
| GAS | Gameplay Abilities | Enable in Plugins window |

All other categories load unconditionally.

---

## Architecture

- **Transport:** HTTP + SSE on `localhost:7777`
- **Protocol:** MCP 2024-11-05 / JSON-RPC 2.0
- **Engine version:** UE 5.6+ (5.7 compatible)
- **Module type:** Editor plugin, `PostEngineInit`
- **Threading:** HTTP handlers run on the game thread tick — all UE API calls are direct, no `AsyncTask` wrappers

### Module layout

| Module | Contents |
|--------|----------|
| `UECortex` | Core server, router, registry, level/blueprint/rendering/spline/cpp tools, `editor_input`, `viewport_capture`, `fab_login_status` |
| `UECortexPCG` | PCG graph tools |
| `UECortexGAS` | Gameplay Ability System tools |
| `UECortexNiagara` | Niagara VFX tools |
| `UECortexUMG` | Widget Blueprint tools |
| `UECortexAnim` | Animation Blueprint + skeleton tools |
| `UECortexAsset` | Asset management tools |
| `UECortexSequencer` | Level Sequence / cinematics tools |
| `UECortexPython` | Python executor + dynamic tool registry |

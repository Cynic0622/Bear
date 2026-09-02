#pragma once
namespace Bear
{
	// Per-frame statistics, aggregated across all passes.
	// Each pass owns its own RenderStats instance and fills only the fields it produces;
	// the Renderer sums them into its frame stats.
	struct RenderStats
	{
		uint32_t sceneObjects = 0;      // mesh entities before culling (Scene)
		uint32_t visibleObjects = 0;    // after CPU frustum culling (Scene/Renderer)
		uint32_t opaqueObjects = 0;     // drawn via indirect commands (PbrPass)
		uint32_t transparentObjects = 0;// drawn by OIT (OitPass)
		uint32_t drawCalls = 0;         // scene vkCmd draw invocations (all passes)
	};

	// Toggleable renderer states, displayed in the UI.
	struct RenderStates
	{
		bool frustumCull = true;   // CPU culling, toggled with Z
		bool gpuCulling = false;   // GPU culling, toggled with C
		bool oitEnabled = false;   // toggled with X
		bool editorMode = true;    // toggled with Q
	};
}

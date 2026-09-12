#pragma once

namespace Bear
{
	struct RenderContext;

	// Base of every engine-level pass (graphics or compute).
	class Pass
	{
	public:
		virtual ~Pass() = default;

		virtual void Setup(RenderContext* context) = 0;
		virtual void Cleanup() = 0;
		virtual const char* GetName() const = 0;

		bool IsEnabled() const { return m_Enabled; }
		void SetEnabled(bool enabled) { m_Enabled = enabled; }

	protected:
		RenderContext* m_Context = nullptr;
		bool m_Enabled = true;
	};
}

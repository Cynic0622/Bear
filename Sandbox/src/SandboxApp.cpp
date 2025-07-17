#include <Bear.h>

class SandboxApp : public Bear::Application
{
	public:
	SandboxApp()
	{
	}
	~SandboxApp()
	{
	}
};

Bear::Application* Bear::CreateApplication()
{
	return new SandboxApp();
}
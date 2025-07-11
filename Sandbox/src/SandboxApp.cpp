#include <Bear.h>
//namespace Bear {
//	__declspec(dllimport)class Application;
//}
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
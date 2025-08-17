#pragma once
#include "bearpch.h"
#include "Events/ApplicationEvent.h"
extern Bear::Application* Bear::CreateApplication();

int main(int argc, char** argv) {

	Bear::Log::Init();
	
	auto app = Bear::CreateApplication();
	app->Run();
	delete app;
	return 0;
}
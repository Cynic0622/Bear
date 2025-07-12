#pragma once
#include "bearpch.h"
extern Bear::Application* Bear::CreateApplication();

int main(int argc, char** argv) {

	Bear::Log::Init();
	Bear::Log::GetCoreLogger()->warn("Bear Engine Initialized");
	Bear::Log::GetClientLogger()->error("Application Starting");
	BEAR_CORE_INFO("Bear Engine is running!");
	BEAR_CLIENT_INFO("Application is starting!");
	BEAR_CLIENT_TRACE("This is a test message!");
	auto app = Bear::CreateApplication();
	app->Run();
	delete app;
	return 0;
}
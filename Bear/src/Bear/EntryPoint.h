#pragma once

extern Bear::Application* Bear::CreateApplication();

int main(int argc, char** argv) {
	auto app = Bear::CreateApplication();
	app->Run();
	delete app;
	return 0;
}
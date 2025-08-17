#pragma once


namespace Bear {
	// declaration specifier,声明说明符，dllexport 作为参数表示外部程序可以访问该函数
	__declspec(dllexport)void Print();
}
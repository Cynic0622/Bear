#pragma once

#include "Layer.h"
#include <vector>

namespace Bear {

	class BEAR_API LayerStack
	{
	public:
        // default 是显式要求编译器生成默认实现
		LayerStack() = default;
		~LayerStack();
		void PushLayer(Layer* layer); // 添加Layer到LayerStack中
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);
		auto begin() { return m_Layers.begin(); }
		auto end() { return m_Layers.end(); }
		auto rbegin() { return m_Layers.rbegin(); }
		auto rend() { return m_Layers.rend(); }

	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0; // layer和overlay的分割点，所有的layer都在这个索引之前，所有的overlay都在这个索引之后
	};
}
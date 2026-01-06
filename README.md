# Bear 🐻

Bear 是一个基于 Vulkan API 开发的微型渲染引擎。该项目主要用于探索和学习现代 Vulkan 渲染技术以及 C++ 编程范式。

## 📥 获取项目

由于本项目包含子模块，请使用以下命令克隆：

```bash
git clone --recursive https://github.com/Cynic0622/Bear.git
```

## ✨ 主要特性 (Features)

本项目实现并集成了一系列现代图形学技术与架构设计：

- **RHI 抽象设计 (Render Hardware Interface)**
  - 针对 Vulkan 的底层图形 API 抽象层，为上层渲染逻辑提供统一接口，降低图形 API 调用的复杂度，未来拓展为多渲染 API 后端。

- **神经网络纹理压缩 (Neural Texture Compression)**
  - 集成 NTC 技术，探索基于神经网络的纹理压缩方案，优化显存占用。

- **PBR IBL 渲染管线**
  - 完整的物理渲染 (Physically Based Rendering) 工作流。
  - 支持基于图像的照明 (Image Based Lighting)，实现更真实的材质与环境光照交互。

- **OIT (Order Independent Transparency)**
  - 实现顺序无关透明渲染技术，正确处理半透明物体的深度排序与混合问题。

- **ECS 场景架构**
  - 采用实体组件系统 (Entity Component System) 构建场景图，提高数据局部性及加速场景遍历。

- **渲染优化**
  - **CPU 端视锥剔除 (Frustum Culling)**: 在提交 GPU 之前剔除视野外的物体，减少无效渲染。
  - **Pre-Z Pass**: 提前深度测试通道，能有效减少 Overdraw，提升片段着色阶段的性能。

## 🚀 构建 (Build)

```bash
cd Bear
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 🖼️ 渲染展示 (Gallery)

| Scene (Base) | PBR Material | OIT |
| :---: | :---: | :---: |
| ![Sponza](./rendered%20image/sponza.png) | ![Placeholder](./rendered%20image/helmet10.png) | ![Helmet](./rendered%20image/helmet0.png) |
| **Sponza Scene** | **Damaged Helmet** | **FlightHelmet** |
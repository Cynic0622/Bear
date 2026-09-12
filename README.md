# Bear 🐻

Bear 目前是一个基于 Vulkan API 开发的微型渲染引擎。该项目主要用于探索和学习现代 Vulkan 渲染技术以及 C++ 编程范式。

## 📥 获取项目

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
  - **GPU-Driven 剔除 (GPU Culling)**: Compute Shader 视锥剔除 + 材质分桶间接绘制（`DrawIndexedIndirectCount`），绘制调用数量与物体数量解耦。
  - **Hi-Z 遮挡剔除 (Occlusion Culling)**: 基于深度金字塔的物体级遮挡剔除，采用"上一帧预剔 + 同帧修正"两阶段方案，避免时序伪影。

- **帧图 (Render Graph)**
  - **自动同步**: 资源导入与 transient 创建，由 pass 读写声明自动推导 RAW/WAR/WAW 依赖，稳定拓扑排序后生成 Pipeline Barrier 与布局转换；支持死 pass 剔除与屏障裁剪。
  - **生命周期别名**: 按执行序计算 transient 资源的存活区间，描述相同且区间不重叠的资源共享同一底层内存，并按 frame-in-flight 池化复用。
  - **全动态渲染 (Dynamic Rendering)**: 场景、OIT、UI 与 Present 交接全部基于 `vkCmdBeginRendering` 并由帧图管理，无 `VkRenderPass`/framebuffer。

## 🗓️ 待实现（TODO）

### 🛠️ 工程架构 (Engineering)

- [x] **GPU剔除**: 基于 Compute shader 实现剔除（视锥 + Hi-Z 遮挡，两阶段同帧修正）。
- [x] **渲染依赖图 (Render Graph)**: 自动分析并管理 Pass 之间的依赖关系，自动插入 Pipeline Barriers 与布局转换，含 transient 资源生命周期别名与死 pass 剔除。
- [ ] **资源缓存系统 (Resource Caching)**: 实现 Pipeline State Object (PSO)、Descriptor Set 及纹理等核心资源的缓存与复用机制，减少运行时开销。

### 🎨 渲染特性 (Rendering)

- [ ] **延迟渲染 (Deferred Rendering)**: 应对大量光源场景，解耦几何处理与光照计算。
- [ ] **基础阴影**: Shadow Mapping 及其改进版本 (PCF)。
- [ ] **软阴影**: PCSS (Percentage-Closer Soft Shadows)。
- [ ] **级联阴影**: CSM (Cascaded Shadow Maps)，优化大场景视距阴影。
- [ ] **环境光遮蔽**: SSAO (Screen Space Ambient Occlusion) / HBAO+。
- [ ] **反射**: SSR (Screen Space Reflection)。
- [ ] **全局光照近似**: SSDO (Screen Space Directional Occlusion)。
- [ ] **实时光线追踪 (Real-time Ray Tracing)**: 基于硬件加速的光追管线集成。

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
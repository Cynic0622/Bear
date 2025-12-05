#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <windows.h>

#include <stdio.h>

#include <iostream>

#include <string>

#include <functional>

#include <sstream>

#include <set>

#include "Log.h"

#include <algorithm>
#include <array>

#include "Validation.h"

#include <vector>
#include <unordered_map>
#include <memory>

#include "Utils.h"
#include "Types.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/ApplicationEvent.h"

#include "Input.h"

#include "Scene/AssetLoader.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
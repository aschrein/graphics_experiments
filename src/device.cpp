#include "../include/device.hpp"
#include "../include/error_handling.hpp"

PFN_vkCreateDebugReportCallbackEXT pfnVkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT pfnVkDestroyDebugReportCallbackEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugReportCallbackEXT *pCallback) {
  return pfnVkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator,
                                           pCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
    VkInstance instance, VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks *pAllocator) {
  pfnVkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkDebugReportFlagsEXT flags,
                                       VkDebugReportObjectTypeEXT /*objType*/,
                                       uint64_t /*srcObject*/,
                                       size_t /*location*/, int32_t msgCode,
                                       const char *pLayerPrefix,
                                       const char *pMsg, void * /*pUserData*/) {
  std::ostringstream message;

  switch (flags) {
  case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
    message << "INFORMATION: ";
    break;
  case VK_DEBUG_REPORT_WARNING_BIT_EXT:
    message << "WARNING: ";
    break;
  case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
    message << "PERFORMANCE WARNING: ";
    break;
  case VK_DEBUG_REPORT_ERROR_BIT_EXT:
    message << "ERROR: ";
    break;
  case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
    message << "DEBUG: ";
    break;
  default:
    message << "unknown flag (" << flags << "): ";
    break;
  }
  message << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg;

#ifdef _WIN32
  MessageBox(NULL, message.str().c_str(), "Alert", MB_OK);
#else
  std::cout << message.str() << std::endl;
#endif

  return false;
}

static char const *AppName = "CreateDebugReportCallback";
static char const *EngineName = "Vulkan.hpp";

bool checkLayers(std::vector<char const *> const &layers,
                 std::vector<vk::LayerProperties> const &properties) {
  // return true if all layers are listed in the properties
  return std::all_of(
      layers.begin(), layers.end(), [&properties](char const *name) {
        return std::find_if(properties.begin(), properties.end(),
                            [&name](vk::LayerProperties const &property) {
                              return strcmp(property.layerName, name) == 0;
                            }) != properties.end();
      });
}

void Device_Wrapper::update_swap_chain() {
  ASSERT_PANIC(this->window);
  vk::SurfaceCapabilitiesKHR caps;
  this->physical_device.getSurfaceCapabilitiesKHR(this->surface, &caps);
  auto surface_formats =
      this->physical_device.getSurfaceFormatsKHR(this->surface);
  auto surface_present_modes =
      this->physical_device.getSurfacePresentModesKHR(this->surface);
  this->swapchain_image_views.clear();
  this->swap_chain = this->device->createSwapchainKHRUnique(
      vk::SwapchainCreateInfoKHR()
          .setImageFormat(vk::Format::eB8G8R8A8Unorm)
          .setImageColorSpace(surface_formats[0].colorSpace)
          .setPresentMode(surface_present_modes[0])
          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
          .setClipped(true)
          .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
          .setImageSharingMode(vk::SharingMode::eExclusive)
          .setSurface(surface)
          .setPQueueFamilyIndices(&this->graphics_queue_family_id)
          .setQueueFamilyIndexCount(1)
          .setImageArrayLayers(1)
          .setImageExtent(caps.currentExtent)
          .setOldSwapchain(this->swap_chain.get())
          .setMinImageCount(caps.minImageCount));
  ASSERT_PANIC(this->swap_chain);
  this->swap_chain_images =
      this->device->getSwapchainImagesKHR(swap_chain.get());

  for (auto &image : this->swap_chain_images)
    this->swapchain_image_views.emplace_back(
        this->device->createImageViewUnique(vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D,
            vk::Format::eB8G8R8A8Unorm,
            vk::ComponentMapping(
                vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                      1))));
}

/*
        VK_LAYER_LUNARG_monitor
        VK_LAYER_LUNARG_screenshot
        VK_LAYER_GOOGLE_threading
        VK_LAYER_GOOGLE_unique_objects
        VK_LAYER_LUNARG_device_simulation
        VK_LAYER_LUNARG_core_validation
        VK_LAYER_LUNARG_vktrace
        VK_LAYER_LUNARG_assistant_layer
        VK_LAYER_LUNARG_api_dump
        VK_LAYER_LUNARG_standard_validation
        VK_LAYER_LUNARG_parameter_validation
        VK_LAYER_LUNARG_object_tracker
*/

extern "C" Device_Wrapper init_device(bool init_glfw) {
  Device_Wrapper out{};
  std::vector<vk::LayerProperties> instanceLayerProperties =
      vk::enumerateInstanceLayerProperties();

  std::vector<char const *> instanceLayerNames;
  instanceLayerNames.push_back("VK_LAYER_LUNARG_standard_validation");
  // instanceLayerNames.push_back("VK_LAYER_LUNARG_object_tracker");
  // instanceLayerNames.push_back("VK_LAYER_LUNARG_parameter_validation");
  // instanceLayerNames.push_back("VK_LAYER_LUNARG_assistant_layer");
  // instanceLayerNames.push_back("VK_LAYER_LUNARG_core_validation");
  // instanceLayerNames.push_back("VK_LAYER_LUNARG_monitor");
  ASSERT_PANIC(checkLayers(instanceLayerNames, instanceLayerProperties));

  vk::ApplicationInfo applicationInfo(AppName, 1, EngineName, 1,
                                      VK_API_VERSION_1_1);

  std::vector<char const *> instanceExtensionNames;

  if (init_glfw) {

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
      exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    out.window = glfwCreateWindow(512, 512, "Vulkan Window", NULL, NULL);
    ASSERT_PANIC(out.window);

    uint32_t glfw_extensions_count;
    const char **glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

    // instanceExtensionNames.push_back("VK_KHR_surface");

    for (auto i = 0u; i < glfw_extensions_count; i++) {
      instanceExtensionNames.push_back(glfw_extensions[i]);
    }
  }

  // instanceExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

  vk::InstanceCreateInfo instanceCreateInfo(
      vk::InstanceCreateFlags(), &applicationInfo,
      static_cast<uint32_t>(instanceLayerNames.size()),
      instanceLayerNames.data(),
      static_cast<uint32_t>(instanceExtensionNames.size()),
      instanceExtensionNames.data());
  out.instance = vk::createInstanceUnique(instanceCreateInfo);
  // pfnVkCreateDebugReportCallbackEXT =
  //     reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
  //         out.instance->getProcAddr("vkCreateDebugReportCallbackEXT"));
  // ASSERT_PANIC(pfnVkCreateDebugReportCallbackEXT);

  // pfnVkDestroyDebugReportCallbackEXT =
  //     reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
  //         out.instance->getProcAddr("vkDestroyDebugReportCallbackEXT"));
  // ASSERT_PANIC(pfnVkDestroyDebugReportCallbackEXT);

  // out.debugReportCallback = out.instance->createDebugReportCallbackEXTUnique(
  //     vk::DebugReportCallbackCreateInfoEXT(
  //         vk::DebugReportFlagBitsEXT::eError |
  //             vk::DebugReportFlagBitsEXT::eWarning,
  //         dbgFunc));

  ASSERT_PANIC(out.instance);
  out.physical_device = out.instance->enumeratePhysicalDevices().front();
  if (init_glfw) {

    ASSERT_PANIC(!glfwCreateWindowSurface(out.instance.get(), out.window,
                                          nullptr, &out.surface));
  }
  std::vector<vk::QueueFamilyProperties> queue_family_properties =
      out.physical_device.getQueueFamilyProperties();
  size_t graphics_queue_id = -1;
  for (size_t i = 0; i < queue_family_properties.size(); i++) {
    auto const &qfp = queue_family_properties[i];
    if (init_glfw) {
      if (qfp.queueFlags & vk::QueueFlagBits::eGraphics &&
          out.physical_device.getSurfaceSupportKHR(i, out.surface))
        graphics_queue_id = i;
    } else {
      if (qfp.queueFlags & vk::QueueFlagBits::eGraphics)
        graphics_queue_id = i;

      ;
    }
  }
  ASSERT_PANIC(graphics_queue_id < queue_family_properties.size());
  out.graphics_queue_family_id = graphics_queue_id;
  if (init_glfw) {

    ASSERT_PANIC(glfwGetPhysicalDevicePresentationSupport(
        out.instance.get(), out.physical_device, out.graphics_queue_family_id));
  }
  float queuePriority = 0.0f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo[] =

      {
          {vk::DeviceQueueCreateFlags(),
           static_cast<uint32_t>(graphics_queue_id), 1, &queuePriority},
      };
  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  out.device = std::move(out.physical_device.createDeviceUnique(
      vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, deviceQueueCreateInfo)
          .setPpEnabledLayerNames(&instanceLayerNames[0])
          .setEnabledLayerCount(instanceLayerNames.size())
          .setPpEnabledExtensionNames(&deviceExtensions[0])
          .setEnabledExtensionCount(deviceExtensions.size())));
  ASSERT_PANIC(out.device);

  if (init_glfw) {
    out.update_swap_chain();
  }

  vk::DescriptorPoolSize aPoolSizes[] = {
      {vk::DescriptorType::eSampler, 1000},
      {vk::DescriptorType::eCombinedImageSampler, 1000},
      {vk::DescriptorType::eSampledImage, 1000},
      {vk::DescriptorType::eStorageImage, 1000},
      {vk::DescriptorType::eUniformTexelBuffer, 1000},
      {vk::DescriptorType::eStorageTexelBuffer, 1000},
      {vk::DescriptorType::eCombinedImageSampler, 1000},
      {vk::DescriptorType::eStorageBuffer, 1000},
      {vk::DescriptorType::eUniformBufferDynamic, 1000},
      {vk::DescriptorType::eStorageBufferDynamic, 1000},
      {vk::DescriptorType::eInputAttachment, 1000}};
  out.descset_pool =
      out.device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000 * 11, 11,
          aPoolSizes));

  out.graphcis_cmd_pool =
      out.device->createCommandPoolUnique(vk::CommandPoolCreateInfo(
          vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
          graphics_queue_id));
  out.graphics_cmds = std::move(
      out.device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
          out.graphcis_cmd_pool.get(), vk::CommandBufferLevel::ePrimary, 6)));
  out.graphics_queue = out.device->getQueue(graphics_queue_id, 0);

  return out;
}
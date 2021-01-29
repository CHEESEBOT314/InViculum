#include "vulkan_wrapper.hxx"

#include <memory>
#include <optional>
#include <set>

/**
 * vulkan_wrapper - Namespace which acts like a singleton class. Provides all functions to interact with Vulkan
 */
namespace vulkan_wrapper {
    /**
     * Anonymous namespace to hold 'private' variables
     */
    namespace {
        const int MAX_FRAMES_IN_FLIGHT = 3;
        void (*resolution_function)(int*, int*);
        // Structure used to hold both the command pool and its buffers
        struct Command {
            vk::CommandPool pool;
            std::vector<vk::CommandBuffer> buffers;
        };
        // Holds all 'private' variables used by Vulkan
        struct info {
            vk::Instance instance;
            vk::SurfaceKHR surface;

            vk::PhysicalDevice physical_device;
            vk::Device device;

            vk::Queue graphics_queue;
            vk::Queue present_queue;
            uint32_t graphics_id = 0;
            uint32_t present_id = 0;

            vk::SwapchainKHR swapchain;
            std::vector<vk::Image> swapchain_images;
            vk::Format swapchain_image_format = vk::Format::eB8G8R8A8Unorm;
            vk::Extent2D swapchain_extent;
            std::vector<vk::ImageView> swapchain_image_views;
            std::vector<vk::Framebuffer> swapchain_framebuffers;
            vk::RenderPass render_pass;

            std::vector<Command> commands;
            std::vector<vk::Semaphore> image_available_semaphores;
            std::vector<vk::Semaphore> render_finished_semaphores;
            std::vector<vk::Fence> in_flight_fences;
            std::vector<vk::Fence> images_in_flight;

            vk::Image depth_image;
            vk::DeviceMemory depth_image_memory;
            vk::ImageView depth_image_view;

            size_t current_frame = 0;
            bool draw = false;

            vk::DispatchLoaderDynamic dldi;
            // If DEBUG is enabled, also include the messenger
#ifdef DEBUG_MODE
            vk::DebugUtilsMessengerEXT debugMessenger;
#endif
        };
        std::unique_ptr<info> info_p;

#ifdef DEBUG_MODE
        VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
            printf("%s\n", pCallbackData->pMessage);
            return VK_FALSE;
        }
        vk::Result createDebugUtilsMessengerEXT() {
            vk::DebugUtilsMessengerCreateInfoEXT createInfo = {vk::DebugUtilsMessengerCreateFlagsEXT(),
                                                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                                                               vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                                                               debugCallback};
            return info_p->instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &info_p->debugMessenger, info_p->dldi);
        }
        void destroyDebugUtilsMessengerEXT() {
            info_p->instance.destroyDebugUtilsMessengerEXT(info_p->debugMessenger, nullptr, info_p->dldi);
        }
#endif
        // Structure used for querying queue families for devices
        struct queue_family_indices {
            std::optional<uint32_t> graphics_family;
            std::optional<uint32_t> present_family;
            [[nodiscard]] bool is_complete() const {
                return graphics_family.has_value() && present_family.has_value();
            }
        };
        // 'private' function only called from within this file, tries to find graphics and present families for the
        // given physical device. This must succeed for the application to run.
        queue_family_indices find_queue_families(vk::PhysicalDevice physical_device) {
            queue_family_indices indices;
            std::vector<vk::QueueFamilyProperties> queue_families = physical_device.getQueueFamilyProperties();

            uint32_t i = 0;
            for (const vk::QueueFamilyProperties& properties : queue_families) {
                if (properties.queueCount > 0) {
                    if (properties.queueFlags & vk::QueueFlagBits::eGraphics) {
                        indices.graphics_family = i;
                    }
                    if (physical_device.getSurfaceSupportKHR(i, info_p->surface)) {
                        indices.present_family = i;
                    }
                    if (indices.is_complete()) {
                        break;
                    }
                }
                i++;
            }
            return indices;
        }
        // 'private' function only called from within this file, checks to see if the given physical device has all of
        // the required extensions for this application
        bool check_device_extension_support(vk::PhysicalDevice pd) {
            std::vector<vk::ExtensionProperties> extension_properties = pd.enumerateDeviceExtensionProperties(nullptr);

            std::set<std::string> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

            for (vk::ExtensionProperties properties : extension_properties) {
                device_extensions.erase(properties.extensionName);
            }
            return device_extensions.empty();
        }
        // Contains details for the swapchain from a physical device
        struct swapchain_support_details {
            vk::SurfaceCapabilitiesKHR capabilities;
            std::vector<vk::SurfaceFormatKHR> formats;
            std::vector<vk::PresentModeKHR> present_modes;
        };

        // 'private' function only called from within this file, for the given physical device returns details about the
        // swapchain availability
        swapchain_support_details query_swapchain_support(vk::PhysicalDevice device) {
            swapchain_support_details details;

            details.capabilities = device.getSurfaceCapabilitiesKHR(info_p->surface);
            details.formats = device.getSurfaceFormatsKHR(info_p->surface);
            details.present_modes = device.getSurfacePresentModesKHR(info_p->surface);

            return details;
        }
        // 'private' function only called from within this file, chooses a suitable surface format from the given available
        // formats, preferred format is 32bit Srgb nonlinear
        vk::SurfaceFormatKHR choose_swapchain_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
            if (available_formats.size() == 1 && available_formats[0].format == vk::Format::eUndefined) {
                return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear};
            }

            for (const auto& available_format : available_formats) {
                if (available_format.format == vk::Format::eB8G8R8A8Unorm && available_format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
                    return available_format;
                }
            }
            return available_formats[0];
        }
        // 'private' function only called from within this file, chooses the best present mode given the available modes,
        // the preferred mode is Mailbox, then Immediate and finally First-in-First-out is chosen as a fallback
        vk::PresentModeKHR choose_swapchain_present_mode(const std::vector<vk::PresentModeKHR>& availableModes) {
            vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

            for (const vk::PresentModeKHR mode : availableModes) {
                if (mode == vk::PresentModeKHR::eMailbox) {
                    return mode;
                }
                if (mode == vk::PresentModeKHR::eImmediate) {
                    bestMode = mode;
                }
            }
            return bestMode;
        }
        // 'private' function only called from within this file, creates a valid extent from the given swapchain capabilities,
        // glfw 'resolution_function' is queried to get the size of the window and uses that as a basis
        vk::Extent2D choose_swapchain_extent(const vk::SurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            }
            int width, height;
            resolution_function(&width, &height);

            vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
        // 'private' function only called from within this file, pulls together most of the functions above to check if
        // the given physical device is suitable
        bool is_device_suitable(vk::PhysicalDevice physcial_device) {
            queue_family_indices indices = find_queue_families(physcial_device);

            bool extensions_supported = check_device_extension_support(physcial_device);

            bool swapchain_adequate = false;
            if (extensions_supported) {
                swapchain_support_details swapchain_support = query_swapchain_support(physcial_device);
                swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
            }

            return indices.is_complete() && extensions_supported && swapchain_adequate;
        }
        // 'private' function only called from within this file, returns the best format from the given formats for
        // the provided image tiling and feature flags, this is used to create the depth buffer
        vk::Format find_supported_format(const std::vector<vk::Format>& formats, vk::ImageTiling tiling, const vk::FormatFeatureFlags& flags) {
            for (vk::Format format : formats) {
                vk::FormatProperties properties;
                info_p->physical_device.getFormatProperties(format, &properties);
                if (((tiling == vk::ImageTiling::eLinear) && ((properties.linearTilingFeatures & flags) == flags)) ||
                    ((tiling == vk::ImageTiling::eOptimal) && ((properties.optimalTilingFeatures & flags) == flags))) {
                    return format;
                }
            }
            return vk::Format::eUndefined;
        }
    }
    /**
     * create_instance - Create Instance function creates a Vulkan instance using the given extensions
     * @param extensions - required extensions
     * @return successful or not
     */
    bool create_instance(std::vector<const char*> extensions) {
        if (info_p) {
            return false;
        }
        info_p = std::make_unique<info>();

        const std::vector<const char*> validation_layers = {"VK_LAYER_LUNARG_standard_validation"};

        // If DEBUG enabled, also check for enabled validation layers
#ifdef DEBUG_MODE
        std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
        for (const char* layerName : validationLayers) {
            bool found = false;
            for (const vk::LayerProperties& props : layerProperties) {
                if (strcmp(layerName, props.layerName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                printf("Could not find validation layer");
                return false;
            }
        }
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        // Create the instance, if DEBUG enabled include validation layers
        vk::ApplicationInfo app_info = {"DMFP", 0x00400000/* 0000000000,0000000000,000000000000 */, "No Engine", 0x00400000, 0x00401000};
        vk::InstanceCreateInfo create_info = {vk::InstanceCreateFlags(), &app_info,
#ifdef DEBUG_MODE
        static_cast<uint32_t>(validation_layers.size()), validation_layers.data(),
#else
            0, nullptr,
#endif
            static_cast<uint32_t>(extensions.size()), extensions.data()};

        info_p->instance = vk::createInstance(create_info);
        info_p->dldi = vk::DispatchLoaderDynamic(info_p->instance);
#ifdef DEBUG_MODE
        create_debug_utils_messenger_EXT();
#endif
        return true;
    }
    /**
     * create_surface - Create Surface function uses a provided function to create the surface such as create_surface in
     * glfw_wrapper, this function saves the resolution function for later use
     * @param fn - function to call with the instance and surface
     * @param r - resolution function that returns the width and height of the window
     * @return - successful or not
     */
    bool create_surface(bool(*fn)(const vk::Instance&, vk::SurfaceKHR&), void (*r)(int*, int*)) {
        if (info_p) {
            resolution_function = r;
            return fn(info_p->instance, info_p->surface);
        }
        return false;
    }

    /**
     * create_others - Create Others function creates all the necessary resources after the instance and surface creation.
     * Firstly it finds a suitable physical device by checking each one with the @link is_device_suitable function.
     * Secondly it creates a virtual device and the graphics and present queues.
     * Thirdly it creates the command pools and buffers in order to send render commands to the GPU.
     * Fourthly it creates synchronisation objects (semaphores and fences) in order to utilise double/triple buffering.
     * Lastly it calls the @link create_swapchain function to load the rest of the objects.
     * @return - If it successfully loaded everything
     */
    bool create_others() {
        /////////////////////////
        //// PHYSICAL DEVICE ////
        /////////////////////////

        // Pick a suitable physical device using the functions described above, if none suitable, fail
        std::vector<vk::PhysicalDevice> physcial_devices = info_p->instance.enumeratePhysicalDevices();
        for (const vk::PhysicalDevice& d : physcial_devices) {
            if (is_device_suitable(d)) {
                info_p->physical_device = d;
                break;
            }
        }
        if (!info_p->physical_device) {
            return false;
        }

        ////////////////
        //// DEVICE ////
        ////////////////

        // Create the device from the selected physical device
        queue_family_indices indices = find_queue_families(info_p->physical_device);

        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

        // Create the graphics and present queues
        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families) {
            vk::DeviceQueueCreateInfo queue_create_info = {vk::DeviceQueueCreateFlags(), queue_family, 1, &queue_priority};
                queue_create_infos.push_back(queue_create_info);
        }

        vk::PhysicalDeviceFeatures device_features = {};
        const std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#ifdef DEBUG_MODE
        const std::vector<const char*> validation_layers = {"VK_LAYER_LUNARG_standard_validation"};
#endif

        vk::DeviceCreateInfo device_create_info = {vk::DeviceCreateFlags(), static_cast<uint32_t>(queue_create_infos.size()), queue_create_infos.data(),
            
#ifdef  DEBUG_MODE
            static_cast<uint32_t>(validation_layers.size()), validationLayers.data(),
#else
            0, nullptr,
#endif
                                                 static_cast<uint32_t>(device_extensions.size()), device_extensions.data(), &device_features};

        // Create the device from the given physical device and get the queues and ids
        info_p->device = info_p->physical_device.createDevice(device_create_info);

        info_p->graphics_queue = info_p->device.getQueue(indices.graphics_family.value(), 0);
        info_p->present_queue = info_p->device.getQueue(indices.present_family.value(), 0);
        info_p->graphics_id = indices.graphics_family.value();
        info_p->present_id = indices.present_family.value();

        ///////////////////////
        //// COMMAND POOLS ////
        ///////////////////////

        // Create a command pool for each buffer, used for parallel rendering
        info_p->commands.resize(MAX_FRAMES_IN_FLIGHT);

        // Use the correct variables from the device and create them
        vk::CommandPoolCreateInfo command_pool_create_info = {vk::CommandPoolCreateFlags(), indices.graphics_family.value()};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            info_p->commands[i].pool = info_p->device.createCommandPool(command_pool_create_info);
        }

        /////////////////////////
        //// COMMAND BUFFERS ////
        /////////////////////////

        // Create one command buffer for each pool, this stores the rendering commands before they are sent to the GPU
        for (Command& cmd : info_p->commands) {
            cmd.buffers.resize(1);
            vk::CommandBufferAllocateInfo command_buffer_allocate_info = {cmd.pool, vk::CommandBufferLevel::ePrimary, 1};
            cmd.buffers = info_p->device.allocateCommandBuffers(command_buffer_allocate_info);
        }

        //////////////////////
        //// SYNC OBJECTS ////
        //////////////////////

        // The image available semaphore is used to signal that the chosen image is ready to be rendered to,
        // this is used as a barrier to prevent rendering to an in use framebuffer
        // The render finished semaphore is used to signal that the chosen image has finished being displayed,
        // this is used as a barrier to prevent deletion of an in use framebuffer
        // The in flight fences are used as a way to communicate between the multiple framebuffers, keeping
        // them synchronised
        // One of each is needed per framebuffer used
        info_p->image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        info_p->render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        info_p->in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semaphore_create_info = {vk::SemaphoreCreateFlags()};
        vk::FenceCreateInfo fence_create_info = {vk::FenceCreateFlagBits::eSignaled};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            info_p->image_available_semaphores[i] = info_p->device.createSemaphore(semaphore_create_info);
            info_p->render_finished_semaphores[i] = info_p->device.createSemaphore(semaphore_create_info);
            info_p->in_flight_fences[i] = info_p->device.createFence(fence_create_info);
        }

        // Create the swap chain (found below)
        return create_swapchain();
    }

    /**
     * create_swapchain - Create Swapchain function creates all of the resources associated with the swapchain.
     * Firstly it creates a swapchain object from the already decided queue families.
     * Secondly it creates all image views for the swapchain.
     * Thirdly it creates a depth image used for depth testing, makes rendering shadows easier.
     * Fourthly it creates the renderpass which is how the rendering is performed, a simple renderpass is created for
     * the purposes of this application.
     * Finally the framebuffers are created which enables the application to be rendered to the screen
     * @return successful or not
     */
    bool create_swapchain() {
        info_p->device.waitIdle();
        ///////////////////
        //// SWAPCHAIN ////
        ///////////////////

        // Using all of the details provided by the functions above, the swapchain is created and swapchain images are
        // retrieved
        swapchain_support_details swapchain_support = query_swapchain_support(info_p->physical_device);

        vk::SurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats);
        vk::PresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.present_modes);
        vk::Extent2D extent = choose_swapchain_extent(swapchain_support.capabilities);

        uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
        if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
            image_count = swapchain_support.capabilities.maxImageCount;
        }

        queue_family_indices indices = find_queue_families(info_p->physical_device);
        uint32_t  queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};
        bool queue_different = indices.graphics_family != indices.present_family;

        vk::SwapchainCreateInfoKHR swapchain_create_info = {vk::SwapchainCreateFlagsKHR(), info_p->surface, image_count, surface_format.format,
                                                            surface_format.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment,
                                                            queue_different ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
                                                            queue_different ? 2U : 0U, queue_different ? queue_family_indices : nullptr,
                                                            swapchain_support.capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                            present_mode, VK_TRUE};

        info_p->swapchain = info_p->device.createSwapchainKHR(swapchain_create_info);

        info_p->swapchain_images = info_p->device.getSwapchainImagesKHR(info_p->swapchain);
        info_p->swapchain_image_format = surface_format.format;
        info_p->swapchain_extent = extent;

        /////////////////////
        //// IMAGE VIEWS ////
        /////////////////////

        // For all of the swapchain images their image view is created and stored so they can be bound to the framebuffers
        info_p->swapchain_image_views.resize(info_p->swapchain_images.size());
        for (uint32_t i = 0; i < info_p->swapchain_images.size(); i++) {
            vk::ImageViewCreateInfo image_view_create_info = {vk::ImageViewCreateFlags(), info_p->swapchain_images[i], vk::ImageViewType::e2D, info_p->swapchain_image_format,
                                                            {vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
                                                            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};


            info_p->swapchain_image_views[i] = info_p->device.createImageView(image_view_create_info);
        }

        /////////////////////
        //// DEPTH IMAGE ////
        /////////////////////

        // A simple depth image is created allong with its image view, this is used in the framebuffer to perform
        // depth testing
        vk::Format depth_format = find_supported_format({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);

        vk::ImageCreateInfo depth_image_create_info = {vk::ImageCreateFlags(), vk::ImageType::e2D, depth_format, vk::Extent3D(info_p->swapchain_extent.width, info_p->swapchain_extent.width, 1), 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::SharingMode::eExclusive};
        info_p->depth_image = info_p->device.createImage(depth_image_create_info);
        vk::MemoryRequirements memory_requirements = info_p->device.getImageMemoryRequirements(info_p->depth_image);

        vk::PhysicalDeviceMemoryProperties physcial_device_memory_properties = info_p->physical_device.getMemoryProperties();

        uint32_t chosen = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < physcial_device_memory_properties.memoryTypeCount; i++) {
            if ((memory_requirements.memoryTypeBits & (1u << i)) && (physcial_device_memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal) {
                chosen = i;
            }
        }

        vk::MemoryAllocateInfo memory_allocate_info = {memory_requirements.size, chosen};
        info_p->depth_image_memory = info_p->device.allocateMemory(memory_allocate_info);
        info_p->device.bindImageMemory(info_p->depth_image, info_p->depth_image_memory, 0);

        vk::ImageViewCreateInfo depth_image_view_create_info = {vk::ImageViewCreateFlags(), info_p->depth_image, vk::ImageViewType::e2D, depth_format, vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)};
        info_p->depth_image_view = info_p->device.createImageView(depth_image_view_create_info);


        /////////////////////
        //// RENDER PASS ////
        /////////////////////

        // A renderpass is created which is told about the swapchain and also the depth image, this is a simple
        // implementation used for basic rendering

        vk::AttachmentDescription attachment_description = {vk::AttachmentDescriptionFlags(), info_p->swapchain_image_format, vk::SampleCountFlagBits::e1,
                                                            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
                                                            vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR};
        vk::AttachmentDescription depth_attachment_description = {vk::AttachmentDescriptionFlags(), depth_format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal};


        vk::AttachmentReference attachment_reference = {0, vk::ImageLayout::eColorAttachmentOptimal};
        vk::AttachmentReference depth_attachment_reference = {1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

        vk::SubpassDescription subpass_description = {vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &attachment_reference, nullptr, &depth_attachment_reference, 0, nullptr};

        vk::SubpassDependency subpass_dependency = {~0U, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlags(),
                                                    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, vk::DependencyFlags()};

        std::array<vk::AttachmentDescription, 2> attachments = {attachment_description, depth_attachment_description};
        vk::RenderPassCreateInfo render_pass_create_info = {vk::RenderPassCreateFlags(), attachments.size(), attachments.data(), 1, &subpass_description, 1, &subpass_dependency};

        info_p->render_pass = info_p->device.createRenderPass(render_pass_create_info);

        //////////////////////
        //// FRAMEBUFFERS ////
        //////////////////////

        // For each of the swapchain images a framebuffer is created which references the depth image
        info_p->swapchain_framebuffers.resize(info_p->swapchain_image_views.size());

        for (uint32_t i = 0; i < info_p->swapchain_framebuffers.size(); i++) {
            std::array<vk::ImageView, 2> views = {info_p->swapchain_image_views[i], info_p->depth_image_view};
            vk::FramebufferCreateInfo framebuffer_create_info = {vk::FramebufferCreateFlags(), info_p->render_pass, views.size(), views.data(),
                                                                info_p->swapchain_extent.width, info_p->swapchain_extent.height, 1};

            info_p->swapchain_framebuffers[i] = info_p->device.createFramebuffer(framebuffer_create_info);
        }
        info_p->images_in_flight.resize(info_p->swapchain_images.size(), vk::Fence());
        return true;
    }
    /**
     * create_vertex_buffer - Create Vertex Buffer function that creates a vertex buffer of the given size
     * @param buffer - returns the buffer
     * @param memory - returns the memory
     * @param size - size of the buffer
     * @return successfuly or not
     */
    bool create_vertex_buffer(vk::Buffer& buffer, vk::DeviceMemory& memory, uint32_t size) {
        vk::BufferCreateInfo buffer_create_info = {vk::BufferCreateFlags(), size, vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, 1, &info_p->graphics_id};
        if (!(buffer = info_p->device.createBuffer(buffer_create_info))) {
            return false;
        }
        vk::MemoryRequirements memory_requirements = info_p->device.getBufferMemoryRequirements(buffer);
        vk::PhysicalDeviceMemoryProperties physcial_device_memory_properties = info_p->physical_device.getMemoryProperties();

        uint32_t chosen = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < physcial_device_memory_properties.memoryTypeCount; i++) {
            if ((memory_requirements.memoryTypeBits & (1u << i)) && (physcial_device_memory_properties.memoryTypes[i].propertyFlags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))) {
                chosen = i;
            }
        }
        if (chosen == std::numeric_limits<uint32_t>::max()) {
            info_p->device.destroyBuffer(buffer);
            return false;
        }
        vk::MemoryAllocateInfo memory_allocate_info = {memory_requirements.size, chosen};
        if (!(memory = info_p->device.allocateMemory(memory_allocate_info))) {
            info_p->device.destroyBuffer(buffer);
            return false;
        }
        info_p->device.bindBufferMemory(buffer, memory, 0);
        return true;
    }
    /**
     * map_vertex_buffer - Map Vertex Buffer function copies the provided data into the provided device memory which is
     * given from the function above
     * @param memory - memory to copy into
     * @param size - size (in bytes) to copy
     * @param data - data to copy
     */
    void map_vertex_buffer(const vk::DeviceMemory& memory, uint32_t size, const void* data) {
        void* mapped_memory = info_p->device.mapMemory(memory, 0, size);
        memcpy(mapped_memory, data, size);
        info_p->device.unmapMemory(memory);
    }
    /**
     * destroy_vertex_buffer - Destroy Vertex Buffer function removes the allocated memory from the device
     * @param buffer - buffer provided
     * @param memory - device memory provided
     */
    void destroy_vertex_buffer(const vk::Buffer& buffer, const vk::DeviceMemory& memory) {
        info_p->device.destroyBuffer(buffer);
        info_p->device.freeMemory(memory);
    }

    /**
     * create_shader_module - Create Shader Module creates a shader module from the given binary source
     * @param shader_module - created shader module
     * @param src - shader binary
     * @return successful or not
     */
    bool create_shader_module(vk::ShaderModule& shader_module, const std::vector<uint8_t>& src) {
        vk::ShaderModuleCreateInfo shader_module_create_info = {vk::ShaderModuleCreateFlags(), src.size(), reinterpret_cast<const uint32_t*>(src.data())};
        shader_module = info_p->device.createShaderModule(shader_module_create_info);
        return true;
    }
    /**
     * destroy_shader_module - Destroy Shader Module function destroys the allocated shader module
     * @param shader_module - shader module to destroy
     */
    void destroy_shader_module(const vk::ShaderModule& shader_module) {
        info_p->device.destroyShaderModule(shader_module);
    }
    /**
     * create_pipeline_layout - Create Pipeline Layout function creates a pipeline layout from the provided create info
     * @param pipeline_layout - created pipeline layout
     * @param pipeline_layout_create_info - creation info
     * @return
     */
    bool create_pipeline_layout(vk::PipelineLayout& pipeline_layout, const vk::PipelineLayoutCreateInfo& pipeline_layout_create_info) {
        pipeline_layout = info_p->device.createPipelineLayout(pipeline_layout_create_info);
        return !!pipeline_layout;
    }
    /**
     * destroy_pipeline_layout - Destroy Pipeline Layout function destroys the provided pipeline layout
     * @param pipeline_layout - pipeline layout to destroy
     */
    void destroy_pipeline_layout(const vk::PipelineLayout& pipeline_layout) {
        info_p->device.destroyPipelineLayout(pipeline_layout);
    }
    /**
     * create_pipeline - Create Pipeline function creates the shader pipeline from the provided parameters, this is
     * where certain aspects like alpha blending are enabled for shadows to work
     * @param pipeline
     * @param pipeline_layout
     * @param shader_module_count
     * @param shader_modules
     * @param vertex_binding_description_count
     * @param vertex_binding_descriptions
     * @param vertex_attribute_description_count
     * @param vertex_attribute_descriptions
     * @param target_aspect
     * @return - successful or not
     */
    bool create_pipeline(vk::Pipeline& pipeline, const vk::PipelineLayout& pipeline_layout, uint32_t shader_module_count, const vk::PipelineShaderStageCreateInfo* shader_modules, uint32_t vertex_binding_description_count, const vk::VertexInputBindingDescription* vertex_binding_descriptions, uint32_t vertex_attribute_description_count, const vk::VertexInputAttributeDescription* vertex_attribute_descriptions, float target_aspect) {
        vk::PipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {vk::PipelineVertexInputStateCreateFlags(), vertex_binding_description_count, vertex_binding_descriptions, vertex_attribute_description_count, vertex_attribute_descriptions};
        vk::PipelineInputAssemblyStateCreateInfo pipeline_assembly_state_create_info = {vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, VK_FALSE};

        float width = info_p->swapchain_extent.width;
        float height = info_p->swapchain_extent.height;
        vk::Viewport viewport = {0.0f, 0.0f, width, height, 0.0f, 1.0f};
        vk::Rect2D scissor = {{0, 0}, info_p->swapchain_extent};
        vk::PipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor};
        vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {vk::PipelineRasterizationStateCreateFlags(), VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f};
        vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE};
        vk::PipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
        vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eCopy, 1, &pipeline_color_blend_attachment_state, {0.0f, 0.0f, 0.0f, 0.0f}};
        vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {vk::PipelineDepthStencilStateCreateFlags(), true, true, vk::CompareOp::eLess, false, false};

        vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info = {vk::PipelineCreateFlags(), shader_module_count, shader_modules, &pipeline_vertex_input_state_create_info, &pipeline_assembly_state_create_info, nullptr, &pipeline_viewport_state_create_info, &pipeline_rasterization_state_create_info, &pipeline_multisample_state_create_info, &pipeline_depth_stencil_state_create_info, &pipeline_color_blend_state_create_info, nullptr, pipeline_layout, info_p->render_pass, 0, vk::Pipeline(), -1};
        pipeline = info_p->device.createGraphicsPipeline(vk::PipelineCache(), graphics_pipeline_create_info);
        return !!pipeline;
    }
    /**
     * destroy_pipeline - Destroy Pipeline function destroys the provided pipeline
     * @param pipeline - pipeline to destroy
     */
    void destroy_pipeline(const vk::Pipeline& pipeline) {
        info_p->device.destroyPipeline(pipeline);
    }

    /**
     * render_frame - Render Frame function deals with all of the rendering every frame
     * @param external_render - render function to call in the middle (e.g game::render)
     * @return successful or not
     */
    bool render_frame(void (*external_render)()) {
        // Wait for image to become available
        info_p->device.waitForFences(1, &info_p->in_flight_fences[info_p->current_frame], VK_TRUE, std::numeric_limits<uint64_t >::max());
        // Retrieve the next available image
        vk::ResultValue<uint32_t> result_value = info_p->device.acquireNextImageKHR(info_p->swapchain, std::numeric_limits<uint64_t >::max(), info_p->image_available_semaphores[info_p->current_frame], vk::Fence());
        if (result_value.result == vk::Result::eErrorOutOfDateKHR) {
            if (reload_swapchain()) {
                return render_frame(external_render);
            }
            return false;
        }
        else if (result_value.result != vk::Result::eSuccess && result_value.result != vk::Result::eSuboptimalKHR) {
            return false;
        }
        // Check if the image is ready to be rendered to
        uint32_t currentIndex = result_value.value;
        if (info_p->images_in_flight[currentIndex] != vk::Fence()) {
            info_p->device.waitForFences(1, &info_p->images_in_flight[currentIndex], VK_TRUE, std::numeric_limits<uint64_t >::max());
        }
        info_p->images_in_flight[currentIndex] = info_p->in_flight_fences[info_p->current_frame];

        // Remove all stored commands from last frame
        info_p->device.resetCommandPool(info_p->commands[info_p->current_frame].pool, vk::CommandPoolResetFlagBits::eReleaseResources);

        // Start a new command recording
        vk::CommandBufferBeginInfo command_buffer_begin_info = {};
        info_p->commands[info_p->current_frame].buffers[0].begin(command_buffer_begin_info);

        // Clear the colour and depth images
        std::array<vk::ClearValue, 2> clear_values{};
        std::array<float, 4> colour = {0.0F, 0.0F, 0.0F, 1.0F};
        clear_values[0].color = {colour};
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0F, 0);

        // Begin the render pass
        vk::RenderPassBeginInfo render_pass_begin_info = {info_p->render_pass, info_p->swapchain_framebuffers[currentIndex], {{0, 0}, info_p->swapchain_extent}, clear_values.size(), clear_values.data()};
        info_p->commands[info_p->current_frame].buffers[0].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        // Call the extenal renderer to add commands to the buffer
        info_p->draw = true;
        external_render();
        info_p->draw = false;

        // End the renderpass and finish the buffer
        info_p->commands[info_p->current_frame].buffers[0].endRenderPass();
        info_p->commands[info_p->current_frame].buffers[0].end();

        // Tell the GPU how to use this command buffer by using the synchronisation objects
        vk::Semaphore wait_semaphores[] = {info_p->image_available_semaphores[info_p->current_frame]};
        vk::Semaphore signal_semaphores[] = {info_p->render_finished_semaphores[info_p->current_frame]};
        vk::PipelineStageFlags pipeline_stage_flags[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo submit_info = {1, wait_semaphores, pipeline_stage_flags, 1, &info_p->commands[info_p->current_frame].buffers[0], 1, signal_semaphores};

        // Reset the fence currently in use
        info_p->device.resetFences(1, &info_p->in_flight_fences[info_p->current_frame]);
        // Submit the commands to the GPU
        info_p->graphics_queue.submit(1, &submit_info, info_p->in_flight_fences[info_p->current_frame]);

        // Tell the GPU to present the image
        vk::PresentInfoKHR present_info = {1, signal_semaphores, 1, &info_p->swapchain, &currentIndex};
        info_p->present_queue.presentKHR(present_info);

        (info_p->current_frame += 1) %= MAX_FRAMES_IN_FLIGHT;
        return true;
    }
    // Returns the aspect ratio used to make correct perspective matrices
    float get_aspect_ratio() {
        return (float)info_p->swapchain_extent.width / (float)info_p->swapchain_extent.height;
    }
    // Bind the chosen pipeline to the command buffer
    void bind_pipeline(const vk::Pipeline& pipeline) {
        if (!info_p->draw) return;
        info_p->commands[info_p->current_frame].buffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    }
    // Bind the chosen vertex buffer to the command buffer
    void bind_vertex_buffers(uint32_t count, const vk::Buffer* buffers, const vk::DeviceSize* offsets) {
        if (!info_p->draw) return;
        info_p->commands[info_p->current_frame].buffers[0].bindVertexBuffers(0, count, buffers, offsets);
    }
    // Push the constants to the command buffer
    void push_constants(const vk::PipelineLayout& layout, const vk::ShaderStageFlags& stage, uint32_t offset, uint32_t size, const void* ptr) {
        if (!info_p->draw) return;
        info_p->commands[info_p->current_frame].buffers[0].pushConstants(layout, stage, offset, size, ptr);
    }
    // Draw the supplied vertex buffer by submitting it to the command buffer
    void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
        if (!info_p->draw) return;
        info_p->commands[info_p->current_frame].buffers[0].draw(vertex_count, instance_count, first_vertex, first_instance);
    }

    // Used if the swapchain no longer matches up correctly
    bool reload_swapchain() {
        destroy_swapchain();
        return create_swapchain();
    }

    // Destroy all objects associated with the swapchain
    void destroy_swapchain() {
        info_p->device.waitIdle();
        for (const vk::Framebuffer& framebuffer : info_p->swapchain_framebuffers) {
            info_p->device.destroyFramebuffer(framebuffer);
        }
        info_p->device.destroyRenderPass(info_p->render_pass);
        
        for (const vk::ImageView& image_view : info_p->swapchain_image_views) {
            info_p->device.destroyImageView(image_view);
        }
        info_p->device.destroySwapchainKHR(info_p->swapchain);
    }

    // Wait for the device to become idle (stop rendering)
    void wait_idle() {
        info_p->device.waitIdle();
    }

    // Destroys all Vulkan associated objects in the correct order
    void terminate() {
        destroy_swapchain();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            info_p->device.destroySemaphore(info_p->image_available_semaphores[i]);
            info_p->device.destroySemaphore(info_p->render_finished_semaphores[i]);
            info_p->device.destroyFence(info_p->in_flight_fences[i]);
        }

        for (const Command& cmd : info_p->commands) {
            info_p->device.freeCommandBuffers(cmd.pool, cmd.buffers);
            info_p->device.destroyCommandPool(cmd.pool);
        }

        info_p->device.destroy();

#ifdef DEBUG_MODE
        destroyDebugUtilsMessengerEXT();
#endif
        info_p->instance.destroySurfaceKHR(info_p->surface);
        info_p->instance.destroy();
        info_p.reset(nullptr);
    }
}
#define GLFW_INCLUDE_VULKAN // Includes #include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APG_IMPLEMENTATION
#define APG_NO_BACKTRACES
#include "apg.h" // C stuff like file loading.

#define APG_M_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define APG_M_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define APG_M_CLAMP( x, lo, hi ) ( APG_M_MIN( hi, APG_M_MAX( lo, x ) ) )

#define WIN_W 1024 // In screen coordinates (not necessarily pixels).
#define WIN_H 768  // In screen coordinates (not necessarily pixels).
#define MAX_EXTENSION_STR 2048
#define N_REQUIRED_EXTENSIONS 1
#define N_REQUIRED_VALIDATION_LAYERS 1
const char* required_extensions[N_REQUIRED_EXTENSIONS]               = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const char* required_validation_layers[N_REQUIRED_VALIDATION_LAYERS] = { "VK_LAYER_KHRONOS_validation" };

typedef struct gfx_vul_t {
  GLFWwindow* window_ptr;                   //
  VkApplicationInfo app_info;               //
  VkInstance instance;                      //
  VkPhysicalDevice physical_device;         //
  VkDevice device;                          // "Logical" device.
  VkSurfaceKHR surface;                     // "KHR" suffix implies it's from an extension.
  VkSwapchainKHR swapchain;                 // Collection of render targets. e.g. front & back.
  VkImage swapchain_images[16];             // NB: Typical 1 image view and 1 framebuffer per swapchain image.
  uint32_t n_swapchain_images;              //
  VkFormat swapchain_image_format;          //
  VkExtent2D swapchain_extent;              //
  VkImageView swapchain_image_views[16];    // 1 per swapchain image. Required to draw any part of an image from swapchain.
  VkRenderPass render_pass;                 //
  VkPipelineLayout pipeline_layout;         //
  VkPipeline pipeline;                      // Graphics pipeline. All render state. One per shader/render set up.
  VkFramebuffer swapchain_framebuffers[16]; // Image views used for colour, depth, stencil targets.
  VkCommandPool command_pool;               //
  VkCommandBuffer command_buffer;           //
  VkSemaphore image_available_semaphore;    //
  VkSemaphore render_finished_semaphore;    //
  VkFence in_flight_fence;                  //
  VkQueue present_queue;
  VkQueue graphics_queue;
} gfx_vul_t;

// Command Queues Required. Most ops performed async by submitting to a VkQueue.
typedef struct queue_family_indices_t {
  uint32_t gfx_idx, present_idx;
  bool has_gfx, has_present;
} queue_family_indices_t;

typedef struct swapchain_support_t {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR surface_formats[1024];
  VkPresentModeKHR present_modes[1024];
  uint32_t n_surface_formats, n_present_modes;
} swapchain_support_t;

gfx_vul_t gfx;

#define MAX_VALIDATION_LAYERS 1024
bool check_validation_layer_support( void ) {
  uint32_t n_layers = 0;
  vkEnumerateInstanceLayerProperties( &n_layers, NULL );

  VkLayerProperties layer_props[MAX_VALIDATION_LAYERS];
  assert( n_layers < MAX_VALIDATION_LAYERS );
  vkEnumerateInstanceLayerProperties( &n_layers, layer_props );

  for ( int i = 0; i < N_REQUIRED_VALIDATION_LAYERS; i++ ) {
    bool found = false;
    for ( int j = 0; j < n_layers; j++ ) {
      // printf( "%i) %s\n", j, layer_props[j].layerName );
      if ( 0 == strncmp( required_validation_layers[i], layer_props[j].layerName, MAX_EXTENSION_STR ) ) {
        found = true;
        break;
      }
    }
    if ( !found ) {
      fprintf( stderr, "ERROR: Validation layer not found `%s`\n", required_validation_layers[i] );
      return false;
    }
  }

  return true;
}

bool create_instance( bool enable_validation ) {
  if ( enable_validation && !check_validation_layer_support() ) {
    fprintf( stderr, "ERROR: Validation layer check.\n" );
    return false;
  }

  if ( !glfwInit() ) {
    fprintf( stderr, "ERROR: Failed to init GLFW.\n" );
    return false;
  }
  if ( !glfwVulkanSupported() ) {
    fprintf( stderr, "ERROR: Vulkan not available.\n" );
    return false;
  }

  uint32_t n_extentions = 0;
  const char** extentions_ptr;
  extentions_ptr = glfwGetRequiredInstanceExtensions( &n_extentions );
  if ( !extentions_ptr ) { return false; }

  gfx.app_info = (VkApplicationInfo){ //
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = "Hello Triangle",
    .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
    .pEngineName        = "No Engine",
    .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
    .apiVersion         = VK_API_VERSION_1_0
  };

  VkInstanceCreateInfo create_info = (VkInstanceCreateInfo){           //
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, //
    .pApplicationInfo        = &gfx.app_info,
    .enabledExtensionCount   = n_extentions,
    .ppEnabledExtensionNames = extentions_ptr
  };
  if ( enable_validation ) {
    create_info.enabledLayerCount   = N_REQUIRED_VALIDATION_LAYERS;
    create_info.ppEnabledLayerNames = required_validation_layers;
  }

  VkResult result = vkCreateInstance( &create_info, NULL, &gfx.instance ); // https://registry.khronos.org/vulkan/specs/latest/man/html/vkCreateInstance.html
  if ( VK_SUCCESS != result ) {
    fprintf( stderr, "ERROR: Creating Vulkan instance. Code %i.\n", (int)result );
    return false;
  }

  return true;
}

// TODO could use glfwGetPhysicalDevicePresentationSupport( instance, physical_device, queue_family_index )
queue_family_indices_t find_queue_families( VkPhysicalDevice device ) {
  queue_family_indices_t indices = (queue_family_indices_t){ .has_gfx = false };
  uint32_t n_queue_fams          = 0;

  vkGetPhysicalDeviceQueueFamilyProperties( device, &n_queue_fams, NULL );
  printf( "INFO: Found %i queue families.\n", n_queue_fams );

  assert( n_queue_fams < 16 );
  VkQueueFamilyProperties queue_families[16];
  vkGetPhysicalDeviceQueueFamilyProperties( device, &n_queue_fams, queue_families );
  for ( int i = 0; i < n_queue_fams; i++ ) {
    if ( queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
      indices.gfx_idx = i;
      indices.has_gfx = true;
    }
    VkBool32 fam_has_present = false;
    vkGetPhysicalDeviceSurfaceSupportKHR( device, i, gfx.surface, &fam_has_present );
    if ( fam_has_present ) {
      indices.present_idx = i;
      indices.has_present = true;
    }
  }

  return indices;
}

bool check_extension_support( VkPhysicalDevice device ) {
  uint32_t n_extensions = 0;
  vkEnumerateDeviceExtensionProperties( device, NULL, &n_extensions, NULL );
  printf( "INFO: n_extensions=%u\n", n_extensions );
  bool found_required_extensions[N_REQUIRED_EXTENSIONS] = { false };
  assert( n_extensions < 2048 );
  VkExtensionProperties avail_extensions[2048];
  vkEnumerateDeviceExtensionProperties( device, NULL, &n_extensions, avail_extensions );
  // NOTE: Really a string comp rather than just a code to compare? This can't be right.
  int n_found = 0;
  for ( int i = 0; i < n_extensions; i++ ) {
    // printf( "%s\n", avail_extensions[i].extensionName );
    for ( int j = 0; j < N_REQUIRED_EXTENSIONS; j++ ) {
      if ( 0 == strncmp( required_extensions[j], avail_extensions[i].extensionName, MAX_EXTENSION_STR ) ) {
        if ( !found_required_extensions[j] ) {
          found_required_extensions[j] = true;
          n_found++;
          printf( "found ext %i for %i %s\n", i, j, avail_extensions[i].extensionName );
          if ( n_found >= N_REQUIRED_EXTENSIONS ) { return true; }
        }
      }
    }
  }
  fprintf( stderr, "ERROR: Vulkan instance is missing required extensions.\n" );
  return false;
}

swapchain_support_t query_swapchain_support( VkPhysicalDevice physical_device ) {
  swapchain_support_t details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physical_device, gfx.surface, &details.capabilities );

  vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, gfx.surface, &details.n_surface_formats, NULL );
  assert( details.n_surface_formats < 1024 );
  if ( details.n_surface_formats > 0 ) {
    vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, gfx.surface, &details.n_surface_formats, details.surface_formats );
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR( physical_device, gfx.surface, &details.n_present_modes, NULL );
  assert( details.n_present_modes < 1024 );
  if ( details.n_present_modes > 0 ) {
    vkGetPhysicalDeviceSurfacePresentModesKHR( physical_device, gfx.surface, &details.n_present_modes, details.present_modes );
  }

  return details;
}

bool is_device_suitable( VkPhysicalDevice physical_device ) {
  VkPhysicalDeviceProperties device_props;
  vkGetPhysicalDeviceProperties( physical_device, &device_props );
  VkPhysicalDeviceFeatures device_feats;
  vkGetPhysicalDeviceFeatures( physical_device, &device_feats );
  printf( "INFO: VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU=%i\n", (int)device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU );
  bool got_extensions            = check_extension_support( physical_device );
  queue_family_indices_t indices = find_queue_families( physical_device );
  bool swap_chain_adequate       = false;
  if ( got_extensions ) {
    swapchain_support_t details = query_swapchain_support( physical_device );
    swap_chain_adequate         = details.n_present_modes > 0 && details.n_surface_formats > 0;
  }
  return indices.has_gfx && indices.has_present && got_extensions && swap_chain_adequate;
}

bool pick_physical_device() {
  gfx.physical_device = VK_NULL_HANDLE;
  uint32_t n_devices  = 0;
  vkEnumeratePhysicalDevices( gfx.instance, &n_devices, NULL );
  printf( "INFO: Found %i physical devices.\n", n_devices );
  if ( 0 == n_devices ) {
    fprintf( stderr, "ERROR: Could not find any devices supporting Vulkan.\n" );
    return false;
  }
  assert( n_devices < 8 );
  VkPhysicalDevice devices_ptr[8];
  vkEnumeratePhysicalDevices( gfx.instance, &n_devices, devices_ptr );
  bool picked = false;
  for ( int i = 0; i < n_devices; i++ ) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties( devices_ptr[i], &props );
    bool suitable = is_device_suitable( devices_ptr[i] );
    printf( "INFO: Device %i: %s. Suitable: %i.\n", i, props.deviceName, (int)suitable );

    if ( !picked && suitable ) {
      // NOTE: Could choose best of devices here and check queue families.
      gfx.physical_device = devices_ptr[i];
      printf( "INFO: Using physical device %i.\n", i );
      picked = true;
    }
  }

  return VK_NULL_HANDLE != gfx.physical_device;
}

#define N_QUEUE_CREATE_INFOS 2
bool create_logical_device( void ) {
  VkDeviceQueueCreateInfo queue_create_infos[N_QUEUE_CREATE_INFOS];
  queue_family_indices_t indices            = find_queue_families( gfx.physical_device );
  uint32_t queue_fams[N_QUEUE_CREATE_INFOS] = { indices.gfx_idx, indices.present_idx };

  float priority = 1.0f;
  for ( int i = 0; i < N_QUEUE_CREATE_INFOS; i++ ) {
    queue_create_infos[i] = (VkDeviceQueueCreateInfo){                //
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, //
      .queueFamilyIndex = queue_fams[i],
      .queueCount       = 1,
      .pQueuePriorities = &priority
    };
  }

  VkPhysicalDeviceFeatures device_feats = (VkPhysicalDeviceFeatures){ .alphaToOne = VK_FALSE }; // Just to set everything to 0 for now.

  VkDeviceCreateInfo create_info = (VkDeviceCreateInfo){
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, //
    .pQueueCreateInfos       = queue_create_infos,
    .queueCreateInfoCount    = N_QUEUE_CREATE_INFOS,
    .pEnabledFeatures        = &device_feats,
    .enabledExtensionCount   = N_REQUIRED_EXTENSIONS, // These device extensions will be enabled at vkCreateDeveice().
    .ppEnabledExtensionNames = required_extensions    // Need to explicitly enable swap chain extension here.
  };

  if ( VK_SUCCESS != vkCreateDevice( gfx.physical_device, &create_info, NULL, &gfx.device ) ) {
    fprintf( stderr, "ERROR: Failed to create logical device!\n" );
    return false;
  }
  vkGetDeviceQueue( gfx.device, indices.gfx_idx, 0, &gfx.graphics_queue );
  vkGetDeviceQueue( gfx.device, indices.present_idx, 0, &gfx.present_queue );

  return true;
}

bool create_window_and_surface( void ) {
  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API ); // "No OpenGL".
  glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );   // Need to be careful with resizing.
  gfx.window_ptr = glfwCreateWindow( WIN_W, WIN_H, "Anton's Vulkan Hello Triangle in C", NULL, NULL );
  if ( !gfx.window_ptr ) {
    fprintf( stderr, "ERROR: Creating Window.\n" );
    return false;
  }
  if ( VK_SUCCESS != glfwCreateWindowSurface( gfx.instance, gfx.window_ptr, NULL, &gfx.surface ) ) {
    fprintf( stderr, "ERROR: failed to create window surface!\n" );
    return false;
  }
  return true;
}

VkSurfaceFormatKHR choose_swap_surface_format( swapchain_support_t* details_ptr ) {
  for ( int i = 0; i < details_ptr->n_surface_formats; i++ ) {
    // Prefer sRGB. B8G8R8A8 is a commonly-available format for sRGB.
    if ( details_ptr->surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && details_ptr->surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
      return details_ptr->surface_formats[i];
    }
  }
  return details_ptr->surface_formats[0]; // Default to first format is usually okay.
}

VkPresentModeKHR choose_swap_present_mode( swapchain_support_t* details_ptr ) {
  for ( int i = 0; i < details_ptr->n_present_modes; i++ ) {
    // Prefer VK_PRESENT_MODE_MAILBOX_KHR - modern triple-buffering style.
    if ( details_ptr->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR ) { return details_ptr->present_modes[i]; }
  }
  return VK_PRESENT_MODE_FIFO_KHR; // Only mode guaranteed to be available. Standard double-buffering style.
}

// Resolution of swapchain images.
VkExtent2D choose_swap_extent( VkSurfaceCapabilitiesKHR* caps_ptr ) {
  if ( caps_ptr->currentExtent.width != UINT32_MAX ) {
    return caps_ptr->currentExtent;
  } else {
    int fb_width = 0, fb_height = 0;
    glfwGetFramebufferSize( gfx.window_ptr, &fb_width, &fb_height );
    VkExtent2D extent = { (uint32_t)fb_width, (uint32_t)fb_height };

    extent.width  = APG_M_CLAMP( extent.width, caps_ptr->minImageExtent.width, caps_ptr->maxImageExtent.width );
    extent.height = APG_M_CLAMP( extent.height, caps_ptr->minImageExtent.height, caps_ptr->maxImageExtent.height );

    return extent;
  }
}

bool create_swap_chain( void ) {
  swapchain_support_t details       = query_swapchain_support( gfx.physical_device );
  VkSurfaceFormatKHR surface_format = choose_swap_surface_format( &details );
  VkPresentModeKHR present_mode     = choose_swap_present_mode( &details );
  gfx.swapchain_extent              = choose_swap_extent( &details.capabilities );
  uint32_t n_images                 = details.capabilities.minImageCount + 1; // +1 to reduce wait times.
  // 0 is a special number meaning "no maximum" here.
  if ( details.capabilities.maxImageCount > 0 && n_images > details.capabilities.maxImageCount ) { n_images = details.capabilities.maxImageCount; }
  VkSwapchainCreateInfoKHR create_info = (VkSwapchainCreateInfoKHR){
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, //
    .surface          = gfx.surface,                                 //
    .minImageCount    = n_images,                                    //
    .imageFormat      = surface_format.format,                       //
    .imageColorSpace  = surface_format.colorSpace,                   //
    .imageExtent      = gfx.swapchain_extent,                        //
    .imageArrayLayers = 1,                                           // 1 if not stereoscopic.
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT          // VK_IMAGE_USAGE_TRANSFER_DST_BIT to render to image for post-proc.
  };

  queue_family_indices_t indices = find_queue_families( gfx.physical_device );
  uint32_t combined_indices[]    = { indices.gfx_idx, indices.present_idx };

  // Drawing images in swapchain from gfx queue, then submit images on presentation queue.
  if ( indices.gfx_idx != indices.present_idx ) {
    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices   = combined_indices;
  } else {
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;    // Optional
    create_info.pQueueFamilyIndices   = NULL; // Optional
  }
  create_info.preTransform   = details.capabilities.currentTransform; // Current to say "no transform like flip/rotate/etc."
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;     // Don't blend alpha channel with other OS windows.
  create_info.presentMode    = present_mode;
  create_info.clipped        = VK_TRUE;        // Unless needing to read pixel colours back when overlapped by another window, then set to true for best perf.
  create_info.oldSwapchain   = VK_NULL_HANDLE; // For recreating swapchain on resize/tab out etc.
  if ( VK_SUCCESS != vkCreateSwapchainKHR( gfx.device, &create_info, NULL, &gfx.swapchain ) ) {
    fprintf( stderr, "ERROR: Failed to create swap chain!\n" );
    return false;
  }

  // Retrieve swapchain image handles for later reference.
  vkGetSwapchainImagesKHR( gfx.device, gfx.swapchain, &gfx.n_swapchain_images, NULL );
  assert( gfx.n_swapchain_images < 16 );
  vkGetSwapchainImagesKHR( gfx.device, gfx.swapchain, &gfx.n_swapchain_images, gfx.swapchain_images );
  gfx.swapchain_image_format = surface_format.format;

  return true;
}

bool create_image_views( void ) {
  for ( size_t i = 0; i < gfx.n_swapchain_images; i++ ) {
    VkImageViewCreateInfo create_info = (VkImageViewCreateInfo){
      //
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, //
      .image                           = gfx.swapchain_images[i],                  //
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,                    //
      .format                          = gfx.swapchain_image_format,               //
      .components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY,            // Can e.g. map all chans to red for monochrome.
      .components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY,            // Or set constant value e.g. 0 or 1 to a chan.
      .components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY,            // But this is the default mapping.
      .components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY,            //
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,                //
      .subresourceRange.baseMipLevel   = 0,                                        //
      .subresourceRange.levelCount     = 1,                                        //
      .subresourceRange.baseArrayLayer = 0,                                        //
      .subresourceRange.layerCount     = 1                                         // Stereoscopic apps have multiple layers for left and right eyes.
    };
    if ( VK_SUCCESS != vkCreateImageView( gfx.device, &create_info, NULL, &gfx.swapchain_image_views[i] ) ) {
      fprintf( stderr, "ERROR: Failed to create image view!\n" );
      return false;
    }
  }
  return true;
}

bool create_render_pass( void ) {
  // Attachment description.
  VkAttachmentDescription color_attachment = (VkAttachmentDescription){
    .format        = gfx.swapchain_image_format,     //
    .samples       = VK_SAMPLE_COUNT_1_BIT,          // No MSAA yet.
    .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,    // Clear the FB before drawing new frame.
    .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,   // Store so we can render to screen.
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,      //
    .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // Means images are to be presented in the swapchain.
  };

  VkAttachmentReference color_attachment_ref = (VkAttachmentReference){
    .attachment = 0,                                       //
    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL //
  };

  VkSubpassDescription subpass = (VkSubpassDescription){
    .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS, //
    .colorAttachmentCount = 1,                               //
    .pColorAttachments    = &color_attachment_ref            //
  };

  VkRenderPassCreateInfo render_pass_create_info = (VkRenderPassCreateInfo){
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, //
    .attachmentCount = 1,                                         //
    .pAttachments    = &color_attachment,                         //
    .subpassCount    = 1,                                         //
    .pSubpasses      = &subpass                                   //
  };

  if ( VK_SUCCESS != vkCreateRenderPass( gfx.device, &render_pass_create_info, NULL, &gfx.render_pass ) ) {
    fprintf( stderr, "ERROR: Failed to create render pass!\n" );
    return false;
  }
  return true;
}

VkShaderModule create_shader_module( apg_file_t file ) {
  VkShaderModuleCreateInfo create_info = (VkShaderModuleCreateInfo){
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, //
    .codeSize = file.sz,                                     //
    .pCode    = file.data_ptr                                //
  };
  VkShaderModule shader_module;
  if ( vkCreateShaderModule( gfx.device, &create_info, NULL, &shader_module ) != VK_SUCCESS ) {
    fprintf( stderr, "ERROR: Failed to create shader module!\n" );
    assert( false );
  }
  return shader_module;
}

bool create_graphics_pipeline( void ) {
  apg_file_t vert, frag;
  if ( !apg_read_entire_file( "vert.spv", &vert ) ) {
    fprintf( stderr, "ERROR: Loading vert.spv\n" );
    return 1;
  }
  if ( !apg_read_entire_file( "frag.spv", &frag ) ) {
    fprintf( stderr, "ERROR: Loading frag.spv\n" );
    return 1;
  }
  VkShaderModule vert_shader_module = create_shader_module( vert );
  VkShaderModule frag_shader_module = create_shader_module( frag );

  VkPipelineShaderStageCreateInfo vert_stage_info = (VkPipelineShaderStageCreateInfo){
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vert_shader_module, .pName = "main"
  };
  VkPipelineShaderStageCreateInfo frag_stage_info = (VkPipelineShaderStageCreateInfo){
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = frag_shader_module, .pName = "main"
  };
  VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

  ///////////
#define N_DYNAMIC_STATES 2
  VkDynamicState dynamic_states[N_DYNAMIC_STATES]            = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = (VkPipelineDynamicStateCreateInfo){
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, //
    .dynamicStateCount = N_DYNAMIC_STATES,                                     //
    .pDynamicStates    = dynamic_states                                        //
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_info = (VkPipelineVertexInputStateCreateInfo){
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, //
    .vertexBindingDescriptionCount   = 0,                                                         //
    .pVertexBindingDescriptions      = NULL,                                                      // Optional
    .vertexAttributeDescriptionCount = 0,                                                         //
    .pVertexAttributeDescriptions    = NULL,                                                      // Optional
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly = (VkPipelineInputAssemblyStateCreateInfo){
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, //
    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                         //
    .primitiveRestartEnable = VK_FALSE                                                     //
  };

  VkViewport viewport = (VkViewport){
    viewport.x        = 0.0f,                               //
    viewport.y        = 0.0f,                               //
    viewport.width    = (float)gfx.swapchain_extent.width,  //
    viewport.height   = (float)gfx.swapchain_extent.height, //
    viewport.minDepth = 0.0f,                               //
    viewport.maxDepth = 1.0f                                //
  };

  VkRect2D scissor = (VkRect2D){ scissor.offset = (VkOffset2D){ 0, 0 }, scissor.extent = gfx.swapchain_extent };

  VkPipelineViewportStateCreateInfo viewport_state = (VkPipelineViewportStateCreateInfo){
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, //
    .viewportCount = 1,                                                     //
    .pViewports    = &viewport,                                             //
    .scissorCount  = 1,                                                     //
    .pScissors     = &scissor                                               //
  };

  VkPipelineRasterizationStateCreateInfo rasteriser = (VkPipelineRasterizationStateCreateInfo){
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, //
    .depthClampEnable        = VK_FALSE,                                                   // True useful for shadow maps.
    .rasterizerDiscardEnable = VK_FALSE,                                                   // True disables output to fb.
    .polygonMode             = VK_POLYGON_MODE_FILL,                                       //
    .lineWidth               = 1.0f,                                                       // Thicker requires wideLines GPU feature.
    .cullMode                = VK_CULL_MODE_BACK_BIT,                                      //
    .frontFace               = VK_FRONT_FACE_CLOCKWISE,                                    //
    .depthBiasEnable         = VK_FALSE,                                                   //
    .depthBiasConstantFactor = 0.0f,                                                       // Optional
    .depthBiasClamp          = 0.0f,                                                       // Optional
    .depthBiasSlopeFactor    = 0.0f                                                        // Optional
  };

  VkPipelineMultisampleStateCreateInfo multisampling = (VkPipelineMultisampleStateCreateInfo){
    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable   = VK_FALSE,
    .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading      = 1.0f,     // Optional
    .pSampleMask           = NULL,     // Optional
    .alphaToCoverageEnable = VK_FALSE, // Optional
    .alphaToOneEnable      = VK_FALSE  // Optional
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment = (VkPipelineColorBlendAttachmentState){
    .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable         = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .colorBlendOp        = VK_BLEND_OP_ADD,      // Optional
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .alphaBlendOp        = VK_BLEND_OP_ADD       // Optional
  };

  VkPipelineColorBlendStateCreateInfo color_blending = (VkPipelineColorBlendStateCreateInfo){
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable     = VK_FALSE,
    .logicOp           = VK_LOGIC_OP_COPY, // Optional
    .attachmentCount   = 1,
    .pAttachments      = &color_blend_attachment,
    .blendConstants[0] = 0.0f, // Optional
    .blendConstants[1] = 0.0f, // Optional
    .blendConstants[2] = 0.0f, // Optional
    .blendConstants[3] = 0.0f  // Optional
  };

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = (VkPipelineLayoutCreateInfo){
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount         = 0,    // Optional
    .pSetLayouts            = NULL, // Optional
    .pushConstantRangeCount = 0,    // Optional
    .pPushConstantRanges    = NULL  // Optional
  };

  if ( VK_SUCCESS != vkCreatePipelineLayout( gfx.device, &pipeline_layout_create_info, NULL, &gfx.pipeline_layout ) ) {
    fprintf( stderr, "ERROR: Failed to create pipeline layout!\n" );
    return false;
  }

  VkGraphicsPipelineCreateInfo pipeline_create_info = (VkGraphicsPipelineCreateInfo){
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, //
    .stageCount          = 2,                                               //
    .pStages             = shader_stages,                                   //
    .pVertexInputState   = &vertex_input_info,                              //
    .pInputAssemblyState = &input_assembly,                                 //
    .pViewportState      = &viewport_state,                                 //
    .pRasterizationState = &rasteriser,                                     //
    .pMultisampleState   = &multisampling,                                  //
    .pDepthStencilState  = NULL,                                            // Optional
    .pColorBlendState    = &color_blending,                                 //
    .pDynamicState       = &dynamic_state_create_info,                      //
    .layout              = gfx.pipeline_layout,                             //
    .renderPass          = gfx.render_pass,                                 //
    .subpass             = 0,                                               //
    .basePipelineHandle  = VK_NULL_HANDLE,                                  // Optional
    .basePipelineIndex   = -1                                               // Optional
  };
  // NB Addtional params allow you to create a pipeline based on an existing one.

  if ( VK_SUCCESS != vkCreateGraphicsPipelines( gfx.device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &gfx.pipeline ) ) {
    fprintf( stderr, "ERROR: Failed to create graphics pipeline!\n" );
    return false;
  }

  //////////////

  vkDestroyShaderModule( gfx.device, frag_shader_module, NULL );
  vkDestroyShaderModule( gfx.device, vert_shader_module, NULL );
  free( vert.data_ptr );
  free( frag.data_ptr );

  return true;
}

bool create_framebuffers( void ) {
  for ( int i = 0; i < gfx.n_swapchain_images; i++ ) {
    VkImageView attachments[] = { gfx.swapchain_image_views[i] };

    VkFramebufferCreateInfo framebuffer_create_info = (VkFramebufferCreateInfo){
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, //
      .renderPass      = gfx.render_pass,                           //
      .attachmentCount = 1,                                         //
      .pAttachments    = attachments,                               //
      .width           = gfx.swapchain_extent.width,                //
      .height          = gfx.swapchain_extent.height,               //
      .layers          = 1                                          //
    };

    if ( VK_SUCCESS != vkCreateFramebuffer( gfx.device, &framebuffer_create_info, NULL, &gfx.swapchain_framebuffers[i] ) ) {
      fprintf( stderr, "ERROR: Failed to create framebuffer!\n" );
      return false;
    }
  }

  return true;
}

bool create_command_pool( void ) {
  queue_family_indices_t queue_fam_indices = find_queue_families( gfx.physical_device );

  VkCommandPoolCreateInfo pool_create_info = (VkCommandPoolCreateInfo){
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,      //
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, //  Allow command buffers re=recorded individually. W/o flag all have to be reset together.
    .queueFamilyIndex = queue_fam_indices.gfx_idx             //
  };

  if ( VK_SUCCESS != vkCreateCommandPool( gfx.device, &pool_create_info, NULL, &gfx.command_pool ) ) {
    fprintf( stderr, "ERROR: Failed to create command pool!\n" );
    return false;
  }

  return true;
}

bool create_command_buffer( void ) {
  VkCommandBufferAllocateInfo alloc_info = (VkCommandBufferAllocateInfo){
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, //
    .commandPool        = gfx.command_pool,                               //
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,                //
    .commandBufferCount = 1                                               //
  };

  if ( VK_SUCCESS != vkAllocateCommandBuffers( gfx.device, &alloc_info, &gfx.command_buffer ) ) {
    fprintf( stderr, "ERROR: Failed to create command buffers!\n" );
    return false;
  }

  return true;
}

bool record_command_buffer( VkCommandBuffer command_buffer, uint32_t image_idx ) {
  VkCommandBufferBeginInfo begin_info = (VkCommandBufferBeginInfo){
    .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, //
    .flags            = 0,                                           // Optional
    .pInheritanceInfo = NULL                                         // Optional
  };

  if ( VK_SUCCESS != vkBeginCommandBuffer( command_buffer, &begin_info ) ) {
    fprintf( stderr, "ERROR: Failed to begin recording command buffer!\n" );
    return false;
  }

  // Start render pass:
  VkClearValue clear_colour                    = { { { 0.3f, 0.3f, 0.3f, 1.0f } } };
  VkRenderPassBeginInfo render_pass_begin_info = (VkRenderPassBeginInfo){
    .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, //
    .renderPass        = gfx.render_pass,                          //
    .framebuffer       = gfx.swapchain_framebuffers[image_idx],    //
    .renderArea.offset = { 0, 0 },                                 //
    .renderArea.extent = gfx.swapchain_extent,                     //
    .clearValueCount   = 1,                                        //
    .pClearValues      = &clear_colour                             //
  };
  vkCmdBeginRenderPass( command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );
  /////
  vkCmdBindPipeline( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx.pipeline );
  VkViewport viewport = (VkViewport){ viewport.x = 0.0f, viewport.y = 0.0f, viewport.width = (float)gfx.swapchain_extent.width,
    viewport.height = (float)gfx.swapchain_extent.height, viewport.minDepth = 0.0f, viewport.maxDepth = 1.0f };
  vkCmdSetViewport( command_buffer, 0, 1, &viewport );

  VkRect2D scissor = (VkRect2D){ .offset = { 0, 0 }, .extent = gfx.swapchain_extent };
  vkCmdSetScissor( command_buffer, 0, 1, &scissor );

  // Draw triangle! (cmd, n_verts, n_instances, first_vert, first_instance )
  vkCmdDraw( command_buffer, 3, 1, 0, 0 );

  vkCmdEndRenderPass( command_buffer ); // End render pass.
  // Stop recording.
  if ( VK_SUCCESS != vkEndCommandBuffer( command_buffer ) ) {
    fprintf( stderr, "ERROR: Failed to record command buffer!\n" );
    return false;
  }

  return true;
}

bool create_sync_objects( void ) {
  VkSemaphoreCreateInfo semaphore_create_info = (VkSemaphoreCreateInfo){ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
  VkFenceCreateInfo fence_create_info         = (VkFenceCreateInfo){ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
  if ( VK_SUCCESS != vkCreateSemaphore( gfx.device, &semaphore_create_info, NULL, &gfx.image_available_semaphore ) ||
       VK_SUCCESS != vkCreateSemaphore( gfx.device, &semaphore_create_info, NULL, &gfx.render_finished_semaphore ) ||
       VK_SUCCESS != vkCreateFence( gfx.device, &fence_create_info, NULL, &gfx.in_flight_fence ) ) {
    fprintf( stderr, "ERROR: Failed to create semaphores.\n" );
    return false;
  }
  return true;
}

bool init_gfx( bool enable_validation ) {
  if ( !create_instance( enable_validation ) ) {
    fprintf( stderr, "ERROR: Failed to create instance.\n" );
    return false;
  } // Also creates Vulkan instance.
  if ( !create_window_and_surface() ) {
    fprintf( stderr, "ERROR: Failed to create_window_and_surface.\n" );
    return false;
  }
  if ( !pick_physical_device() ) {
    fprintf( stderr, "ERROR: Failed to pick_physical_device.\n" );
    return false;
  }
  if ( !create_logical_device() ) {
    fprintf( stderr, "ERROR: Failed to create_logical_device.\n" );
    return false;
  }
  if ( !create_swap_chain() ) {
    fprintf( stderr, "ERROR: Failed to create_swap_chain.\n" );
    return false;
  }
  if ( !create_image_views() ) {
    fprintf( stderr, "ERROR: Failed to create_image_views.\n" );
    return false;
  }
  if ( !create_render_pass() ) {
    fprintf( stderr, "ERROR: Failed to create_render_passs.\n" );
    return false;
  }
  if ( !create_graphics_pipeline() ) {
    fprintf( stderr, "ERROR: Failed to create_graphics_pipeline.\n" );
    return false;
  }
  if ( !create_framebuffers() ) {
    fprintf( stderr, "ERROR: Failed to create_framebuffers.\n" );
    return false;
  }
  if ( !create_command_pool() ) {
    fprintf( stderr, "ERROR: Failed to create_command_pool.\n" );
    return false;
  }
  if ( !create_command_buffer() ) {
    fprintf( stderr, "ERROR: Failed to create_command_buffer.\n" );
    return false;
  }
  if ( !create_sync_objects() ) {
    fprintf( stderr, "ERROR: Failed to create_sync_objects.\n" );
    return false;
  }
  return true;
}

bool draw_frame( void ) {
  vkWaitForFences( gfx.device, 1, &gfx.in_flight_fence, VK_TRUE, UINT64_MAX );
  vkResetFences( gfx.device, 1, &gfx.in_flight_fence );

  // Aquire next image from swapchain.
  uint32_t image_idx = 0;
  vkAcquireNextImageKHR( gfx.device, gfx.swapchain, UINT64_MAX, gfx.image_available_semaphore, VK_NULL_HANDLE, &image_idx );

  vkResetCommandBuffer( gfx.command_buffer, 0 );
  record_command_buffer( gfx.command_buffer, image_idx );

  // Submit command buffer.
  VkSemaphore wait_semaphores[]      = { gfx.image_available_semaphore };
  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSemaphore signal_semaphores[]    = { gfx.render_finished_semaphore };

  VkSubmitInfo submit_info = (VkSubmitInfo){ //
    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount   = 1,
    .pWaitSemaphores      = wait_semaphores,
    .pWaitDstStageMask    = wait_stages,
    .commandBufferCount   = 1,
    .pCommandBuffers      = &gfx.command_buffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores    = signal_semaphores
  };

  // Execute a command buffer for that image.
  if ( VK_SUCCESS != vkQueueSubmit( gfx.graphics_queue, 1, &submit_info, gfx.in_flight_fence ) ) {
    fprintf( stderr, "ERROR: Failed to submit draw command buffer!\n" );
    return false;
  }

  VkSwapchainKHR swapchains[]   = { gfx.swapchain };
  VkPresentInfoKHR present_info = (VkPresentInfoKHR){
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = signal_semaphores,
    .swapchainCount     = 1,
    .pSwapchains        = swapchains,
    .pImageIndices      = &image_idx //
  };

  vkQueuePresentKHR( gfx.present_queue, &present_info ); // Return image to the swapchain for presentation.

  return true;
}

void main_loop( void ) {
  double prev_s           = glfwGetTime();
  double text_countdown_s = 0.1;
  while ( !glfwWindowShouldClose( gfx.window_ptr ) ) {
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_ESCAPE ) ) { break; }
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;
    text_countdown_s -= elapsed_s;
    if ( text_countdown_s <= 0.0 ) {
      text_countdown_s = 0.1;
      double fps = 1.0 / elapsed_s;
      char title_str[2048];
      sprintf( title_str, "Anton's Vulkan Hello Triangle @ %.2lf FPS", fps );
      glfwSetWindowTitle( gfx.window_ptr, title_str );
    }

    draw_frame();
  }
  vkDeviceWaitIdle( gfx.device );
}

void free_gfx( void ) {
  vkDestroySemaphore( gfx.device, gfx.image_available_semaphore, NULL );
  vkDestroySemaphore( gfx.device, gfx.render_finished_semaphore, NULL );
  vkDestroyFence( gfx.device, gfx.in_flight_fence, NULL );
  vkDestroyCommandPool( gfx.device, gfx.command_pool, NULL );
  for ( int i = 0; i < gfx.n_swapchain_images; i++ ) { vkDestroyFramebuffer( gfx.device, gfx.swapchain_framebuffers[i], NULL ); }
  vkDestroyPipeline( gfx.device, gfx.pipeline, NULL );
  vkDestroyPipelineLayout( gfx.device, gfx.pipeline_layout, NULL );
  vkDestroyRenderPass( gfx.device, gfx.render_pass, NULL );
  for ( int i = 0; i < gfx.n_swapchain_images; i++ ) { vkDestroyImageView( gfx.device, gfx.swapchain_image_views[i], NULL ); }
  vkDestroySwapchainKHR( gfx.device, gfx.swapchain, NULL );
  vkDestroySurfaceKHR( gfx.instance, gfx.surface, NULL );
  vkDestroyDevice( gfx.device, NULL );
  vkDestroyInstance( gfx.instance, NULL );
  glfwDestroyWindow( gfx.window_ptr );
  glfwTerminate();
}

int main() {
  printf( "Anton's Vulkan Hello Triangle in C\n" );

  bool enable_validation = true;

  if ( !init_gfx( enable_validation ) ) {
    fprintf( stderr, "ERROR: Failed to init gfx.\n" );
    return 1;
  }

  main_loop();

  free_gfx();

  printf( "Normal exit.\n" );
  return 0;
}

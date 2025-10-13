/*
  Textures and update example to match this:

  https://docs.vulkan.org/tutorial/latest/00_Introduction.html
  DONE - Vulkan 1.4 as a baseline
  TODO - Dynamic rendering instead of render passes
  TODO - Timeline semaphores
  TODO - Slang as the primary shading language
  NOPE - Modern C++ (20) with modules
  NOPE - Vulkan-Hpp with RAII
*/

#include "apg_bmp.h"
#include "apg_maths.h"

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

typedef struct img_t {
  unsigned char* ptr;
  int w, h, n;
} img_t;

typedef struct ubo_t {
  mat4 model, view, proj;
} ubo_t;

#define WIN_W 1024 // In screen coordinates (not necessarily pixels).
#define WIN_H 768  // In screen coordinates (not necessarily pixels).
#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_EXTENSION_STR 2048
#define N_REQUIRED_EXTENSIONS 1
#define N_REQUIRED_VALIDATION_LAYERS 1
const char* required_extensions[N_REQUIRED_EXTENSIONS]               = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const char* required_validation_layers[N_REQUIRED_VALIDATION_LAYERS] = { "VK_LAYER_KHRONOS_validation" };

typedef struct gfx_vul_t {
  GLFWwindow* window_ptr;                                //
  VkApplicationInfo app_info;                            //
  VkInstance instance;                                   //
  VkPhysicalDevice physical_device;                      //
  VkDevice device;                                       // "Logical" device.
  VkSurfaceKHR surface;                                  // "KHR" suffix implies it's from an extension.
  VkSwapchainKHR swapchain;                              // Collection of render targets. e.g. front & back.
  VkImage swapchain_images[16];                          // NB: Typical 1 image view and 1 framebuffer per swapchain image.
  uint32_t n_swapchain_images;                           //
  VkFormat swapchain_image_format;                       //
  VkExtent2D swapchain_extent;                           //
  VkImageView swapchain_image_views[16];                 // 1 per swapchain image. Required to draw any part of an image from swapchain.
  VkRenderPass render_pass;                              //
  VkDescriptorPool descriptor_pool;                      //
  VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT]; //
  VkDescriptorSetLayout descriptor_set_layout;           //
  VkPipelineLayout pipeline_layout;                      //
  VkPipeline pipeline;                                   // Graphics pipeline. All render state. One per shader/render set up.
  VkFramebuffer swapchain_framebuffers[16];              // Image views used for colour, depth, stencil targets.
  VkCommandPool command_pool;                            //
  VkCommandBuffer command_buffer;                        //
  VkSemaphore image_available_semaphore;                 //
  VkSemaphore render_finished_semaphore;                 //
  VkFence in_flight_fence;                               //
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

// x,y,r,g,b
#define VERTICES_SZ sizeof( float ) * ( 2 + 3 ) * 3
VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_memory;

// We have multiple buffers ( 1 for each frame in flight - update one while other is being read ).
VkBuffer uniform_buffers[MAX_FRAMES_IN_FLIGHT];
VkDeviceMemory uniform_buffers_memory[MAX_FRAMES_IN_FLIGHT];
void* uniform_buffers_mapped[MAX_FRAMES_IN_FLIGHT];

static VkImage texture_image;
static VkDeviceMemory texture_image_memory;
static VkImageView texture_image_view;
static VkSampler texture_sampler;

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

  gfx.app_info = (VkApplicationInfo){
    //
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = "Hello Triangle",
    .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
    .pEngineName        = "No Engine",
    .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
    .apiVersion         = VK_API_VERSION_1_4 // NOTE(Anton): Update from 1_0.
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
  // Check for optional features like device anisotropy.
  return indices.has_gfx && indices.has_present && got_extensions && swap_chain_adequate && device_feats.samplerAnisotropy;
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

  // Request optional device features like anisotripic sampler filtering, or we'll get a validation error about it.
  // Note: also checked for these features in is_device_suitable().
  VkPhysicalDeviceFeatures device_feats = (VkPhysicalDeviceFeatures){ .samplerAnisotropy = VK_TRUE };

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

bool image_view_create( VkImage image, VkFormat format, VkImageView* img_view_ptr ) {
  *img_view_ptr                   = NULL;
  VkImageViewCreateInfo view_info = (VkImageViewCreateInfo){
    .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image                           = image,
    .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
    .format                          = format,
    .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel   = 0,
    .subresourceRange.levelCount     = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount     = 1 //
  };
  // NOTE(Anton) can explicitly set .components channels to red for monochrome or a constant value (0 or 1) per channel. Default 0.

  if ( vkCreateImageView( gfx.device, &view_info, NULL, img_view_ptr ) != VK_SUCCESS ) {
    fprintf( stderr, "ERROR: Failed to create image view!\n" );
    return false;
  }

  return true;
}

bool create_image_views( void ) {
  for ( size_t i = 0; i < gfx.n_swapchain_images; i++ ) {
    if ( !image_view_create( gfx.swapchain_images[i], gfx.swapchain_image_format, &gfx.swapchain_image_views[i] ) ) { return false; }
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

bool create_descriptor_pool() {
  VkDescriptorPoolSize pool_sizes[2];
  pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
  pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo pool_info = (VkDescriptorPoolCreateInfo){
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, //
    .poolSizeCount = 2,                                             //
    .pPoolSizes    = pool_sizes,                                    //
    .maxSets       = (uint32_t)MAX_FRAMES_IN_FLIGHT                 //
  };

  if ( VK_SUCCESS != vkCreateDescriptorPool( gfx.device, &pool_info, NULL, &gfx.descriptor_pool ) ) {
    fprintf( stderr, "ERROR: Failed to create descriptor pool!\n" );
    return false;
  }
  return true;
}

#define N_DESCRIPTOR_SETS 2
bool create_descriptor_sets() {
  // one descriptor set for each frame in flight, all with the same layout.
  // we do need all the copies of the layout because the next function expects an array matching the number of sets.
  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = { gfx.descriptor_set_layout, gfx.descriptor_set_layout };
  VkDescriptorSetAllocateInfo alloc_info              = (VkDescriptorSetAllocateInfo){
                 .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = gfx.descriptor_pool, .descriptorSetCount = MAX_FRAMES_IN_FLIGHT, .pSetLayouts = layouts
  };

  if ( VK_SUCCESS != vkAllocateDescriptorSets( gfx.device, &alloc_info, gfx.descriptor_sets ) ) {
    fprintf( stderr, "ERROR: Failed to allocate descriptor sets!\n" );
    return false;
  }

  for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
    VkDescriptorImageInfo image_info =
      (VkDescriptorImageInfo){ .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .imageView = texture_image_view, .sampler = texture_sampler };

    VkWriteDescriptorSet descriptor_writes[N_DESCRIPTOR_SETS];
    memset( descriptor_writes, 0, sizeof( VkWriteDescriptorSet ) * N_DESCRIPTOR_SETS ); // Some extra params default to 0.

    VkDescriptorBufferInfo buffer_info   = (VkDescriptorBufferInfo){ .buffer = uniform_buffers[i], .offset = 0, .range = sizeof( ubo_t ) };
    descriptor_writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet          = gfx.descriptor_sets[i];
    descriptor_writes[0].dstBinding      = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pBufferInfo     = &buffer_info;

    VkDescriptorImageInfo image_info =  (VkDescriptorBufferInfo){ .buffer = uniform_buffers[i], .offset = 0, .range = sizeof( ubo_t ) };
    descriptor_writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet          = gfx.descriptor_sets[i];
    descriptor_writes[1].dstBinding      = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].pImageInfo      = &image_info;

    vkUpdateDescriptorSets( gfx.device, N_DESCRIPTOR_SETS, descriptor_writes, 0, NULL );
  }
  return true;
}

#define N_BINDINGS 2
bool create_descriptor_set_layout( void ) {
  VkDescriptorSetLayoutBinding ubo_layout_binding = (VkDescriptorSetLayoutBinding){
    .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .pImmutableSamplers = NULL, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
  };

  VkDescriptorSetLayoutBinding sampler_layout_binding = (VkDescriptorSetLayoutBinding){
    .binding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImmutableSamplers = NULL, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  VkDescriptorSetLayoutBinding bindings[N_BINDINGS] = { ubo_layout_binding, sampler_layout_binding };

  VkDescriptorSetLayoutCreateInfo layout_info =
    (VkDescriptorSetLayoutCreateInfo){ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = N_BINDINGS, .pBindings = bindings };

  if ( VK_SUCCESS != vkCreateDescriptorSetLayout( gfx.device, &layout_info, NULL, &gfx.descriptor_set_layout ) ) {
    fprintf( stderr, "ERROR: Failed to create descriptor set layout!\n" );
    return false;
  }
  return true;
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

  /// Vertex binding layout stuff here
  VkVertexInputBindingDescription vert_bind_descrip = (VkVertexInputBindingDescription){
    .binding   = 0,                           //
    .stride    = ( 2 + 3 ) * sizeof( float ), //
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX  //
  };
  VkVertexInputAttributeDescription vertex_attrib_descrips[2] = { { .binding = 0, .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 },
    { .binding = 0, .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 2 * sizeof( float ) } };
  ///
  VkPipelineVertexInputStateCreateInfo vertex_input_info = (VkPipelineVertexInputStateCreateInfo){
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, //
    .vertexBindingDescriptionCount   = 1,                                                         // with bufferless was: 0,
    .pVertexBindingDescriptions      = &vert_bind_descrip,                                        // with bufferless was: NULL,
    .vertexAttributeDescriptionCount = 2,                                                         // with bufferless was: 0,
    .pVertexAttributeDescriptions    = vertex_attrib_descrips                                     // with bufferless was: NULL,
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

  // TODO - above
//  rasteriser.cullMode = VK_CULL_MODE_BACK_BIT;
//rasteriser.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
    .setLayoutCount         = 1,                         // 1=reference the layout for UBO and texture sampler. Default 0.
    .pSetLayouts            = &gfx.descriptor_set_layout, // Default NULL.
    .pushConstantRangeCount = 0,                         // Optional
    .pPushConstantRanges    = NULL                       // Optional
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

uint32_t find_memory_type( uint32_t type_filter, VkMemoryPropertyFlags props ) {
  // Query info about available types.
  VkPhysicalDeviceMemoryProperties mem_props;
  vkGetPhysicalDeviceMemoryProperties( gfx.physical_device, &mem_props );
  for ( uint32_t i = 0; i < mem_props.memoryTypeCount; i++ ) {
    // Find first avail type that supports our properties like coherency and host visibility.
    // NOTE(Anton) The == bit after the bitwise logic here doesn't look right to me.
    // I think this is working by accident. e.g. works if i remove ""== props".
    // but will not work as intended if props has > 1 desired item?
    if ( type_filter & ( 1 << i ) && //
         ( ( mem_props.memoryTypes[i].propertyFlags & props ) == props ) ) {
      return i;
    }
  }
  assert( false && "Failed to find mem type" );
  return 0;
}

// x,y,r,g,b
float vertices[] = {
  0.0f, -0.5f, 0.0f, 0.0f, 1.0f, //
  0.5f, 0.5f, 0.0f, 1.0f, 0.0f,  //
  -0.5f, 0.5f, 1.0f, 0.0f, 0.0f  //
};

// Generic/abstracted buffer for all types.
//
// NOTE(Anton) HOST here implies CPU-local memory.
// set VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT flag for GPU memory,
// but use a "staging" buffer on the host first:
// 1. memcpy( user data -> host buffer )
// 2. memcpy( host buffer -> device buffer ) - requires a transfer queue.
bool create_buffer( VkDeviceSize sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props, VkBuffer* buffer_ptr, VkDeviceMemory* buffer_mem_ptr ) {
  VkBufferCreateInfo vb_info = (VkBufferCreateInfo){
    .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, //
    .size        = sz,                                   //
    .usage       = usage,                                // original: VK_BUFFER_USAGE_VERTEX_BUFFER_BIT. Can be >1 flag.
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE             // Only used by gfx queue.
  };

  // Create, but no memory assigned yet.
  if ( VK_SUCCESS != vkCreateBuffer( gfx.device, &vb_info, NULL, buffer_ptr ) ) {
    fprintf( stderr, "ERROR: Failed to create buffer!\n" );
    return false;
  }

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements( gfx.device, *buffer_ptr, &mem_reqs );

  VkMemoryAllocateInfo alloc_info = (VkMemoryAllocateInfo){
    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, //
    .allocationSize  = mem_reqs.size,                          //
    .memoryTypeIndex = find_memory_type(                       //
      mem_reqs.memoryTypeBits,                                 //
      mem_props // original was CPU/HOST mem: VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      )         //
  };

  if ( VK_SUCCESS != vkAllocateMemory( gfx.device, &alloc_info, NULL, buffer_mem_ptr ) ) {
    fprintf( stderr, "ERROR: Failed to allocated buffer memory!\n" );
    return false;
  }

  vkBindBufferMemory( gfx.device, *buffer_ptr, *buffer_mem_ptr, 0 );

  return true;
}

VkCommandBuffer begin_single_time_commands() {
  VkCommandBufferAllocateInfo allocInfo = (VkCommandBufferAllocateInfo){
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandPool = gfx.command_pool, .commandBufferCount = 1
  };
  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers( gfx.device, &allocInfo, &command_buffer );
  VkCommandBufferBeginInfo begin_info =
    (VkCommandBufferBeginInfo){ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
  vkBeginCommandBuffer( command_buffer, &begin_info );

  return command_buffer;
}

void end_single_time_commands( VkCommandBuffer command_buffer ) {
  vkEndCommandBuffer( command_buffer );
  VkSubmitInfo submit_info = (VkSubmitInfo){ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &command_buffer };
  vkQueueSubmit( gfx.graphics_queue, 1, &submit_info, VK_NULL_HANDLE );
  vkQueueWaitIdle( gfx.graphics_queue );
  vkFreeCommandBuffers( gfx.device, gfx.command_pool, 1, &command_buffer );
}

// NB(Anton) It feels ridiculous that I need to write my own memcpy()
// The API should have this - CUDA and HIP do.
void buffer_copy( VkBuffer src, VkBuffer dst, VkDeviceSize sz ) {
  VkCommandBuffer command_buffer = begin_single_time_commands();
  VkBufferCopy copy_region       = (VkBufferCopy){ .size = sz };
  vkCmdCopyBuffer( command_buffer, src, dst, 1, &copy_region );
  end_single_time_commands( command_buffer );
}

bool create_vertex_buffer( size_t sz ) {
  // Create generic buffer on host.
  // VK_BUFFER_USAGE_TRANSFER_SRC_BIT allows host->device copy.
  // HOST flags imply this is a temporary 'staging' buffer on CPU mem.
  VkBuffer host_buffer;
  VkDeviceMemory host_buffer_memory;
  if ( !create_buffer( sz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &host_buffer, &host_buffer_memory ) ) {
    fprintf( stderr, "ERROR: Failed to create main memory vertex buffer!\n" );
    return false;
  }

  void* data;
  vkMapMemory( gfx.device, host_buffer_memory, 0, sz, 0, &data );
  memcpy( data, vertices, sz );
  vkUnmapMemory( gfx.device, host_buffer_memory );

  // Create buffer on device.
  // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT specifies use. Didn't do this on host.
  // LOCAL implies on-device.
  // NOTE(Anton) using vertex_buffer now, not host_buffer.
  if ( !create_buffer( sz, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffer, &vertex_buffer_memory ) ) {
    fprintf( stderr, "ERROR: Failed to create video memory vertex buffer!\n" );
    return false;
  }

  buffer_copy( host_buffer, vertex_buffer, sz );

  vkDestroyBuffer( gfx.device, host_buffer, NULL );
  vkFreeMemory( gfx.device, host_buffer_memory, NULL );

  return true;
}

bool create_uniform_buffers( void ) {
  VkDeviceSize buffer_sz = sizeof( ubo_t );

  for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
    if ( !create_buffer( buffer_sz, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
           &uniform_buffers[i], &uniform_buffers_memory[i] ) ) {
      fprintf( stderr, "ERROR: failed to create UBO!\n" );
      return false;
    }
    vkMapMemory( gfx.device, uniform_buffers_memory[i], 0, buffer_sz, 0, &uniform_buffers_mapped[i] );
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

  // vertex buffers
  VkBuffer vbuffs[]      = { vertex_buffer };
  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers( command_buffer, 0, 1, vbuffs, offsets );

  vkCmdBindDescriptorSets( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx.pipeline_layout, 0, 1, &gfx.descriptor_sets[image_idx], 0, NULL );

  // Draw triangle! (cmd, n_verts, n_instances, first_vert, first_instance )
  uint32_t n_verts = 3;
  vkCmdDraw( command_buffer, n_verts, 1, 0, 0 );

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
  if ( !create_descriptor_set_layout() ) {
    fprintf( stderr, "ERROR: Failed to create_descriptor_set_layout.\n" );
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
  if ( !create_vertex_buffer( VERTICES_SZ ) ) {
    fprintf( stderr, "ERROR: Failed to create_vertex_buffer.\n" );
    return false;
  }
  if ( !create_uniform_buffers() ) {
    fprintf( stderr, "ERROR: Failed to create_uniform_buffers.\n" );
    return false;
  }
  if ( !create_descriptor_pool() ) {
    fprintf( stderr, "ERROR: Failed to create_descriptor_pool.\n" );
    return false;
  }
  if ( !create_descriptor_sets() ) {
    fprintf( stderr, "ERROR: Failed to create_descriptor_sets.\n" );
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

void update_uniform_buffer( uint32_t current_image ) {
  ubo_t u;
  u.model = identity_mat4();
  u.view  = identity_mat4();
  u.proj  = identity_mat4();
  memcpy( uniform_buffers_mapped[current_image], &u, sizeof( ubo_t ) );
  // NOTE(anton) A more efficient way to pass a small buffer of data to shaders are "push constants".
}

bool draw_frame( void ) {
  vkWaitForFences( gfx.device, 1, &gfx.in_flight_fence, VK_TRUE, UINT64_MAX );
  vkResetFences( gfx.device, 1, &gfx.in_flight_fence );

  // Aquire next image from swapchain.
  uint32_t image_idx = 0;
  vkAcquireNextImageKHR( gfx.device, gfx.swapchain, UINT64_MAX, gfx.image_available_semaphore, VK_NULL_HANDLE, &image_idx );

  update_uniform_buffer( image_idx );

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
      double fps       = 1.0 / elapsed_s;
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

  vkDestroyImage( gfx.device, texture_image, NULL );
  vkFreeMemory( gfx.device, texture_image_memory, NULL );
  vkDestroyImageView( gfx.device, texture_image_view, NULL );
  vkDestroySampler( gfx.device, texture_sampler, NULL );

  vkDestroyBuffer( gfx.device, vertex_buffer, NULL );
  vkFreeMemory( gfx.device, vertex_buffer_memory, NULL );

  for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
    vkDestroyBuffer( gfx.device, uniform_buffers[i], NULL );
    vkFreeMemory( gfx.device, uniform_buffers_memory[i], NULL );
  }

  vkDestroyDescriptorPool( gfx.device, gfx.descriptor_pool, NULL );
  vkDestroyDescriptorSetLayout( gfx.device, gfx.descriptor_set_layout, NULL );

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

bool transition_image_layout( VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout ) {
  VkCommandBuffer command_buffer = begin_single_time_commands();

  VkImageMemoryBarrier barrier = (VkImageMemoryBarrier){
    .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, //
    .oldLayout                       = old_layout,                             //
    .newLayout                       = new_layout,                             //
    .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,                //
    .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,                //
    .image                           = image,                                  //
    .srcAccessMask                   = 0,                                      // Modified below.
    .dstAccessMask                   = 0,                                      // Modified below.
    .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,              //
    .subresourceRange.baseMipLevel   = 0,                                      //
    .subresourceRange.levelCount     = 1,                                      //
    .subresourceRange.baseArrayLayer = 0,                                      //
    .subresourceRange.layerCount     = 1                                       //
  };

  VkPipelineStageFlags src_stage_flags;
  VkPipelineStageFlags dst_stage_flags;

  // Undefined → transfer destination: transfer writes that don't need to wait on anything.
  // Transfer destination → shader reading: shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's
  // where we're going to use the texture.
  if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    src_stage_flags       = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst_stage_flags       = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if ( old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    src_stage_flags       = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage_flags       = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    fprintf( stderr, "ERROR: Unsupported layout transition!\n" );
    return false;
  }

  vkCmdPipelineBarrier( command_buffer, src_stage_flags, dst_stage_flags, 0, 0, NULL, 0, NULL, 1, &barrier );

  end_single_time_commands( command_buffer );

  return true;
}

void copy_buffer_to_image( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height ) {
  VkCommandBuffer command_buffer = begin_single_time_commands();

  VkBufferImageCopy region = (VkBufferImageCopy){
    .bufferOffset                    = 0,                         //
    .bufferRowLength                 = 0,                         //
    .bufferImageHeight               = 0,                         //
    .imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, //
    .imageSubresource.mipLevel       = 0,                         //
    .imageSubresource.baseArrayLayer = 0,                         //
    .imageSubresource.layerCount     = 1,                         //
    .imageOffset                     = { 0, 0, 0 },               //
    .imageExtent                     = { width, height, 1 }       //
  };
  vkCmdCopyBufferToImage( command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

  end_single_time_commands( command_buffer );
}

bool image_create( uint32_t w, uint32_t h, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkImage* image_ptr,
  VkDeviceMemory* image_memory_ptr ) {
  /*
Although we could set up the shader to access the pixel values in the buffer, it's better to use image objects in Vulkan for this purpose. Image objects will
make it easier and faster to retrieve colors by allowing us to use 2D coordinates, for one. Pixels within an image object are known as texels and we'll use
that name from this point on. Add the following new class members:
*/

  VkImageCreateInfo image_info = (VkImageCreateInfo){
    .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, //
    .imageType     = VK_IMAGE_TYPE_2D,                    //
    .extent.width  = w,                                   //
    .extent.height = h,                                   //
    .extent.depth  = 1,                                   //
    .mipLevels     = 1,                                   //
    .arrayLayers   = 1,                                   //
    .format        = format,                              //
    .tiling        = tiling,                              // Use VK_IMAGE_TILING_LINEAR to acces pixel memory directly as it will be in same row order.
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,           //
    .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, //
    .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,                                    // Use by only 1 queue family for buffer copy.
    .samples       = VK_SAMPLE_COUNT_1_BIT,                                        // Used by multisampling with attachements.
    .flags         = 0                                                             // Flags available for sparse images/voxels etc.
  };
  if ( VK_SUCCESS != vkCreateImage( gfx.device, &image_info, NULL, image_ptr ) ) {
    fprintf( stderr, "ERROR: Failed to create image!\n" );
    return false;
  }

  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements( gfx.device, *image_ptr, &mem_reqs );

  VkMemoryAllocateInfo alloc_info = (VkMemoryAllocateInfo){
    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,                                          //
    .allocationSize  = mem_reqs.size,                                                                   //
    .memoryTypeIndex = find_memory_type( mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) //
  };
  if ( vkAllocateMemory( gfx.device, &alloc_info, NULL, image_memory_ptr ) != VK_SUCCESS ) {
    fprintf( stderr, "ERROR: Failed to allocate image memory!\n" );
    return false;
  }

  vkBindImageMemory( gfx.device, *image_ptr, *image_memory_ptr, 0 );

  return true;
}

bool texture_image_create( img_t img ) {
  // create a "staging" buffer on the host side of memory to copy in pixels.
  VkBuffer host_buffer;
  VkDeviceMemory host_buffer_memory;
  size_t img_sz = img.w * img.h * img.n;
  if ( !create_buffer( img_sz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &host_buffer, &host_buffer_memory ) ) {
    fprintf( stderr, "ERROR: Failed to create image buffer!\n" );
    return false;
  }
  // NB(Anton) why do I need to create ANOTHER local memory for this?
  // img data->mem then mem->host mem then host mem->device mem??!
  void* data;
  vkMapMemory( gfx.device, host_buffer_memory, 0, img_sz, 0, &data );
  memcpy( data, img.ptr, img_sz );
  vkUnmapMemory( gfx.device, host_buffer_memory );

  if ( !image_create( img.w, img.h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture_image, &texture_image_memory ) ) {
    fprintf( stderr, "ERROR: Failed to create image\n" );
    return false;
  }

  if ( !transition_image_layout( texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) ) { return false; }
  copy_buffer_to_image( host_buffer, texture_image, (uint32_t)img.w, (uint32_t)img.h );
  if ( !transition_image_layout( texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) ) {
    return false;
  }

  vkDestroyBuffer( gfx.device, host_buffer, NULL );
  vkFreeMemory( gfx.device, host_buffer_memory, NULL );

  return true;
}

bool texture_sampler_create( void ) {
  // Figure out max anisotropy with:
  VkPhysicalDeviceProperties properties = (VkPhysicalDeviceProperties){ .apiVersion = 0 };
  vkGetPhysicalDeviceProperties( gfx.physical_device, &properties );

  VkSamplerCreateInfo sampler_info = (VkSamplerCreateInfo){
    .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,  //
    .magFilter               = VK_FILTER_LINEAR,                       //
    .minFilter               = VK_FILTER_LINEAR,                       //
    .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,         //
    .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,         //
    .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,         //
    .anisotropyEnable        = VK_TRUE,                                //
    .maxAnisotropy           = properties.limits.maxSamplerAnisotropy, //
    .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
    .compareEnable           = VK_FALSE,
    .compareOp               = VK_COMPARE_OP_ALWAYS,
    .mipmapMode              = VK_FALSE,
    .compareOp               = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .mipLodBias              = 0.0f,
    .minLod                  = 0.0f,
    .maxLod                  = 0.0f //
  };
  if ( VK_SUCCESS != vkCreateSampler( gfx.device, &sampler_info, NULL, &texture_sampler ) ) {
    fprintf( stderr, "ERROR: Failed to create texture sampler!\n" );
    return false;
  }

  return true;
}

int main() {
  printf( "Anton's Vulkan Hello Triangle in C.\n" );

#ifdef NDEBUG
  bool enable_validation = false;
#else
  bool enable_validation = true;
#endif

  if ( !init_gfx( enable_validation ) ) {
    fprintf( stderr, "ERROR: Failed to init gfx.\n" );
    return 1;
  }

  img_t img;
  img.ptr = apg_bmp_read( "floppy_64.bmp", &img.w, &img.h, &img.n );
  if ( !img.ptr || !img.w || !img.w || !img.n ) {
    fprintf( stderr, "FAILED to load image from file\n" );
    return 1;
  }
  if ( !texture_image_create( img ) ) { return 1; }
  if ( !image_view_create( texture_image, VK_FORMAT_R8G8B8A8_SRGB, &texture_image_view ) ) { return 1; }
  if ( !texture_sampler_create() ) { return 1; }
  apg_bmp_free( img.ptr );

  main_loop();

  free_gfx();

  printf( "Normal exit.\n" );
  return 0;
}

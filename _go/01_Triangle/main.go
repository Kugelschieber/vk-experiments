package main

import (
	"errors"
	"fmt"
	"github.com/go-gl/glfw/v3.3/glfw"
	vk "github.com/vulkan-go/vulkan"
	"log"
	"runtime"
)

var (
	vkInstance            vk.Instance
	vkPhysicalDevice      vk.PhysicalDevice
	vkDevice              vk.Device
	vkGraphicsQueue       vk.Queue
	vkPresentQueue        vk.Queue
	vkSurface             vk.Surface
	vkSwapchain           vk.Swapchain
	vkSwapchainImages     []vk.Image
	vkSwapchainImageViews []vk.ImageView
)

func init() {
	runtime.LockOSThread()
}

func createWindow(title string, width, height int) (*glfw.Window, error) {
	if err := glfw.Init(); err != nil {
		return nil, err
	}

	if !glfw.VulkanSupported() {
		return nil, errors.New("vulkan not support")
	}

	glfw.WindowHint(glfw.ClientAPI, glfw.NoAPI)
	window, err := glfw.CreateWindow(width, height, title, nil, nil)

	if err != nil {
		return nil, err
	}

	window.SetSizeLimits(width, height, width, height)
	return window, nil
}

func destroyWindow(window *glfw.Window) {
	window.Destroy()
	glfw.Terminate()
}

func initVk(window *glfw.Window, width, height uint32) error {
	log.Println("Initializing vulkan...")
	vk.SetGetInstanceProcAddr(glfw.GetVulkanGetInstanceProcAddress())

	if err := vk.Init(); err != nil {
		return err
	}

	// create vulkan instance
	extensions := window.GetRequiredInstanceExtensions()

	if result := vk.CreateInstance(&vk.InstanceCreateInfo{
		SType: vk.StructureTypeInstanceCreateInfo,
		PApplicationInfo: &vk.ApplicationInfo{
			SType:            vk.StructureTypeApplicationInfo,
			PApplicationName: "01 Triangle",
			ApiVersion:       vk.ApiVersion11,
		},
		EnabledExtensionCount:   uint32(len(extensions)),
		PpEnabledExtensionNames: extensions,
	}, nil, &vkInstance); result != vk.Success {
		return errors.New(fmt.Sprintf("vulkan: error creating instance: %d", result))
	}

	// create surface
	surface, err := window.CreateWindowSurface(vkInstance, nil)

	if err != nil {
		return errors.New(fmt.Sprintf("vulkan: error creating surface: %s", err))
	}

	vkSurface = vk.SurfaceFromPointer(surface)

	// select the first device with vulkan support
	var deviceCount uint32

	if result := vk.EnumeratePhysicalDevices(vkInstance, &deviceCount, nil); result != vk.Success {
		return errors.New(fmt.Sprintf("vulkan: error counting devices: %d", result))
	}

	if deviceCount == 0 {
		return errors.New("vulkan: no device with vulkan support found")
	}

	log.Printf("vulkan: number of physical devices: %d", deviceCount)
	devices := make([]vk.PhysicalDevice, deviceCount)

	if result := vk.EnumeratePhysicalDevices(vkInstance, &deviceCount, devices); result != vk.Success {
		return errors.New(fmt.Sprintf("vulkan: error listing devices: %d", result))
	}

	for _, d := range devices {
		vkPhysicalDevice = d
		break // TODO check suitability
	}

	if vkPhysicalDevice == nil {
		return errors.New("vulkan: no suitable device found")
	}

	// find a graphics and presentation queue
	var queueCount uint32
	vk.GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, nil)
	queueProperties := make([]vk.QueueFamilyProperties, queueCount)
	vk.GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, queueProperties)
	var presentQueueIndex, graphicsQueueIndex uint32
	foundPresentQueue, foundGraphicsQueue := false, false

	for i := range queueProperties {
		queueProperties[i].Deref()

		if !foundPresentQueue {
			var presentSupport vk.Bool32
			vk.GetPhysicalDeviceSurfaceSupport(vkPhysicalDevice, uint32(i), vkSurface, &presentSupport)

			if presentSupport == 1 {
				presentQueueIndex = uint32(i)
				foundPresentQueue = true

				if foundGraphicsQueue {
					break
				}
			}
		}

		if !foundGraphicsQueue && queueProperties[i].QueueFlags&vk.QueueFlags(vk.QueueGraphicsBit) != 0 {
			graphicsQueueIndex = uint32(i)
			foundGraphicsQueue = true

			if foundPresentQueue {
				break
			}
		}
	}

	if !foundPresentQueue {
		return errors.New("vulkan: no present queue found")
	}

	if !foundGraphicsQueue {
		return errors.New("vulkan: no graphics queue found")
	}

	// create a logical device and queues
	queues := []vk.DeviceQueueCreateInfo{
		{
			SType:            vk.StructureTypeDeviceQueueCreateInfo,
			QueueFamilyIndex: graphicsQueueIndex,
			QueueCount:       1,
			PQueuePriorities: []float32{1},
		},
	}

	if result := vk.CreateDevice(vkPhysicalDevice, &vk.DeviceCreateInfo{
		SType:                vk.StructureTypeDeviceCreateInfo,
		QueueCreateInfoCount: 1,
		PQueueCreateInfos:    queues,
	}, nil, &vkDevice); result != vk.Success {
		return errors.New(fmt.Sprintf("vulkan: error creating logical device: %d", result))
	}

	vk.GetDeviceQueue(vkDevice, graphicsQueueIndex, 0, &vkGraphicsQueue)
	vk.GetDeviceQueue(vkDevice, presentQueueIndex, 0, &vkPresentQueue)

	// create swap chain
	var surfaceCapabilities vk.SurfaceCapabilities

	if result := vk.GetPhysicalDeviceSurfaceCapabilities(vkPhysicalDevice, vkSurface, &surfaceCapabilities); result != vk.Success {
		return errors.New(fmt.Sprintf("vulkan: error reading device surface capabilities: %d", result))
	}

	surfaceCapabilities.Deref()
	var swapchainFormatCount uint32
	vk.GetPhysicalDeviceSurfaceFormats(vkPhysicalDevice, vkSurface, &swapchainFormatCount, nil)

	if swapchainFormatCount == 0 {
		return errors.New("vulkan: no surface formats present")
	}

	formats := make([]vk.SurfaceFormat, swapchainFormatCount)
	vk.GetPhysicalDeviceSurfaceFormats(vkPhysicalDevice, vkSurface, &swapchainFormatCount, formats)
	var swapchainFormat vk.SurfaceFormat

	for i := range formats {
		formats[i].Deref()
		swapchainFormat = formats[i]
		break // TODO
	}

	desiredSwapchainImages := surfaceCapabilities.MinImageCount + 1

	if surfaceCapabilities.MaxImageCount > 0 && desiredSwapchainImages > surfaceCapabilities.MaxImageCount {
		desiredSwapchainImages = surfaceCapabilities.MaxImageCount
	}

	var swapchainPreTransform vk.SurfaceTransformFlagBits
	requiredTransforms := vk.SurfaceTransformIdentityBit
	supportedTransforms := surfaceCapabilities.SupportedTransforms

	if vk.SurfaceTransformFlagBits(supportedTransforms)&requiredTransforms != 0 {
		swapchainPreTransform = requiredTransforms
	} else {
		swapchainPreTransform = surfaceCapabilities.CurrentTransform
	}

	swapchainCompositeAlpha := vk.CompositeAlphaOpaqueBit
	swapchainCompositeAlphaFlags := []vk.CompositeAlphaFlagBits{
		vk.CompositeAlphaOpaqueBit,
		vk.CompositeAlphaPreMultipliedBit,
		vk.CompositeAlphaPostMultipliedBit,
		vk.CompositeAlphaInheritBit,
	}

	for i := range swapchainCompositeAlphaFlags {
		alphaFlags := vk.CompositeAlphaFlags(swapchainCompositeAlphaFlags[i])

		if surfaceCapabilities.SupportedCompositeAlpha&alphaFlags != 0 {
			swapchainCompositeAlpha = swapchainCompositeAlphaFlags[i]
			break
		}
	}

	surfaceCapabilities.CurrentExtent.Deref()
	var swapchainSize vk.Extent2D

	if surfaceCapabilities.CurrentExtent.Width == vk.MaxUint32 {
		swapchainSize.Width = width
		swapchainSize.Height = height
	} else {
		swapchainSize = surfaceCapabilities.CurrentExtent
	}

	if result := vk.CreateSwapchain(vkDevice, &vk.SwapchainCreateInfo{
		SType:           vk.StructureTypeSwapchainCreateInfo,
		Surface:         vkSurface,
		MinImageCount:   desiredSwapchainImages,
		ImageFormat:     swapchainFormat.Format,
		ImageColorSpace: swapchainFormat.ColorSpace,
		ImageExtent: vk.Extent2D{
			Width:  swapchainSize.Width,
			Height: swapchainSize.Height,
		},
		ImageArrayLayers: 1,
		ImageUsage:       vk.ImageUsageFlags(vk.ImageUsageColorAttachmentBit),
		ImageSharingMode: vk.SharingModeExclusive,
		PreTransform:     swapchainPreTransform,
		CompositeAlpha:   swapchainCompositeAlpha,
		PresentMode:      vk.PresentModeFifo,
		Clipped:          vk.True,
		OldSwapchain:     vk.NullSwapchain,
	}, nil, &vkSwapchain); result != vk.Success {
		return errors.New(fmt.Sprintf("error creating vulkan swap chain: %d", result))
	}

	/*var imgCount uint32
	vk.GetSwapchainImages(vkDevice, vkSwapchain, &imgCount, nil)
	result = vk.GetSwapchainImages(vkDevice, vkSwapchain, &imgCount, vkSwapchainImages)

	if result != vk.Success {
		return errors.New("error getting vulkan swap chain images")
	}

	// create image views for the swap chain images
	vkSwapchainImageViews = make([]vk.ImageView, 0, imgCount)

	for i, img := range vkSwapchainImages {
		result = vk.CreateImageView(vkDevice, &vk.ImageViewCreateInfo{
			SType:    vk.StructureTypeImageViewCreateInfo,
			Image:    img,
			ViewType: vk.ImageViewType2d,
			Format:   vk.FormatB8g8r8a8Uint,
			Components: vk.ComponentMapping{
				R: vk.ComponentSwizzleR,
				G: vk.ComponentSwizzleG,
				B: vk.ComponentSwizzleB,
				A: vk.ComponentSwizzleA,
			},
			SubresourceRange: vk.ImageSubresourceRange{
				AspectMask:     vk.ImageAspectFlags(vk.ImageAspectColorBit),
				BaseMipLevel:   uint32(0),
				LevelCount:     uint32(1),
				BaseArrayLayer: uint32(0),
				LayerCount:     uint32(1),
			},
		}, nil, &vkSwapchainImageViews[i])

		if result != vk.Success {
			return errors.New("error creating swap chain image view")
		}
	}*/

	log.Println("Done initializing vulkan...")
	return nil
}

func cleanupVk() {
	/*for _, img := range vkSwapchainImageViews {
		vk.DestroyImageView(vkDevice, img, nil)
	}

	vk.DestroySwapchain(vkDevice, vkSwapchain, nil)*/
	vk.DestroyDevice(vkDevice, nil)
	vk.DestroySurface(vkInstance, vkSurface, nil)
	vk.DestroyInstance(vkInstance, nil)
}

func load() {

}

func loop(window *glfw.Window) {
	for !window.ShouldClose() {
		// ...
		glfw.PollEvents()
	}
}

func main() {
	window, err := createWindow("01 Triangle", 640, 480)

	if err != nil {
		log.Fatalf("error creating window: %s", err)
	}

	defer destroyWindow(window)

	if err := initVk(window, 640, 480); err != nil {
		log.Fatalf("error initializing vulkan: %s", err)
	}

	defer cleanupVk()
	load()
	loop(window)
}

package main

import (
	"errors"
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
		return nil, errors.New("missing required Vulkan support")
	}

	window, err := glfw.CreateWindow(width, height, title, nil, nil)

	if err != nil {
		return nil, err
	}

	window.MakeContextCurrent()
	return window, nil
}

func destroyWindow(window *glfw.Window) {
	window.Destroy()
	glfw.Terminate()
}

func initVk(window *glfw.Window, width, height uint32) error {
	log.Println("Initializing Vulkan...")
	vk.SetGetInstanceProcAddr(glfw.GetVulkanGetInstanceProcAddress())

	if err := vk.Init(); err != nil {
		return err
	}

	// create Vulkan instance
	extensions := window.GetRequiredInstanceExtensions()
	result := vk.CreateInstance(&vk.InstanceCreateInfo{
		SType: vk.StructureTypeInstanceCreateInfo,
		PApplicationInfo: &vk.ApplicationInfo{
			SType:            vk.StructureTypeApplicationInfo,
			PApplicationName: "01 Simple Sprite",
			ApiVersion:       vk.ApiVersion11,
		},
		EnabledExtensionCount:   uint32(len(extensions)),
		PpEnabledExtensionNames: extensions,
	}, nil, &vkInstance)

	if result != vk.Success {
		return errors.New("error creating Vulkan instance")
	}

	// create surface
	surface, err := window.CreateWindowSurface(vkInstance, nil)

	if err != nil {
		return errors.New("error creating Vulkan surface")
	}

	vkSurface = vk.SurfaceFromPointer(surface)

	// select the first device with Vulkan support
	var deviceCount uint32

	if result := vk.EnumeratePhysicalDevices(vkInstance, &deviceCount, nil); result != vk.Success {
		return errors.New("error counting Vulkan devices")
	}

	if deviceCount == 0 {
		return errors.New("no device with Vulkan support found")
	}

	devices := make([]vk.PhysicalDevice, 1)

	if result := vk.EnumeratePhysicalDevices(vkInstance, &deviceCount, devices); result != vk.Success {
		return errors.New("error listing Vulkan devices")
	}

	for _, d := range devices {
		// TODO check suitability
		vkPhysicalDevice = d
		break
	}

	if vkPhysicalDevice == nil {
		return errors.New("no suitable Vulkan device found")
	}

	// find a graphics queue
	var queueCount uint32
	vk.GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, nil)
	queueProperties := make([]vk.QueueFamilyProperties, queueCount)
	vk.GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, queueProperties)
	var graphicsQueueIndex, presentQueueIndex uint32
	foundGraphicsQueue, foundPresentQueue := false, false

	for i, q := range queueProperties {
		if !foundGraphicsQueue && vk.QueueFlagBits(q.QueueFlags)&vk.QueueGraphicsBit == 1 {
			graphicsQueueIndex = uint32(i)
			foundGraphicsQueue = true

			if foundPresentQueue {
				break
			}
		}

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
	}

	if !foundGraphicsQueue {
		return errors.New("no Vulkan graphics queue found")
	}

	if !foundPresentQueue {
		return errors.New("no Vulkan present queue found")
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

	result = vk.CreateDevice(vkPhysicalDevice, &vk.DeviceCreateInfo{
		SType:                 vk.StructureTypeDeviceCreateInfo,
		QueueCreateInfoCount:  1,
		PQueueCreateInfos:     queues,
		EnabledExtensionCount: 1,
		PpEnabledExtensionNames: []string{
			vk.KhrDisplaySwapchainExtensionName,
		},
	}, nil, &vkDevice)

	if result != vk.Success {
		return errors.New("error creating logical Vulkan device")
	}

	vk.GetDeviceQueue(vkDevice, graphicsQueueIndex, 0, &vkGraphicsQueue)
	vk.GetDeviceQueue(vkDevice, presentQueueIndex, 0, &vkPresentQueue)

	// create swap chain (screen buffer) and get the images
	sharingMode := vk.SharingModeExclusive
	queueFamilyIndexCount := 0
	var queueFamilyIndices []uint32

	if graphicsQueueIndex != presentQueueIndex {
		sharingMode = vk.SharingModeConcurrent
		queueFamilyIndexCount = 2
		queueFamilyIndices = []uint32{graphicsQueueIndex, presentQueueIndex}
	}

	result = vk.CreateSwapchain(vkDevice, &vk.SwapchainCreateInfo{
		SType:                 vk.StructureTypeSwapchainCreateInfo,
		Surface:               vkSurface,
		MinImageCount:         uint32(3),
		ImageFormat:           vk.FormatB8g8r8a8Uint,
		ImageColorSpace:       vk.ColorspaceSrgbNonlinear,
		ImageExtent:           vk.Extent2D{Width: width, Height: height},
		ImageArrayLayers:      1,
		ImageUsage:            vk.ImageUsageFlags(vk.ImageUsageColorAttachmentBit),
		ImageSharingMode:      sharingMode,
		QueueFamilyIndexCount: uint32(queueFamilyIndexCount),
		PQueueFamilyIndices:   queueFamilyIndices,
		//PreTransform
		CompositeAlpha: vk.CompositeAlphaOpaqueBit,
		PresentMode:    vk.PresentModeMailbox,
		Clipped:        vk.True,
		OldSwapchain:   vk.NullSwapchain,
	}, nil, &vkSwapchain)

	if result != vk.Success {
		return errors.New("error creating Vulkan swap chain")
	}

	var imgCount uint32
	vk.GetSwapchainImages(vkDevice, vkSwapchain, &imgCount, nil)
	result = vk.GetSwapchainImages(vkDevice, vkSwapchain, &imgCount, vkSwapchainImages)

	if result != vk.Success {
		return errors.New("error getting Vulkan swap chain images")
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
	}

	log.Println("Done initializing Vulkan...")
	return nil
}

func cleanupVk() {
	for _, img := range vkSwapchainImageViews {
		vk.DestroyImageView(vkDevice, img, nil)
	}

	vk.DestroySwapchain(vkDevice, vkSwapchain, nil)
	vk.DestroySurface(vkInstance, vkSurface, nil)
	vk.DestroyDevice(vkDevice, nil)
	vk.DestroyInstance(vkInstance, nil)
}

func load() {

}

func loop(window *glfw.Window) {
	for !window.ShouldClose() {
		// ...
		window.SwapBuffers()
		glfw.PollEvents()
	}
}

func main() {
	window, err := createWindow("01 Simple Sprite", 640, 480)

	if err != nil {
		log.Fatalf("error creating window: %s", err)
	}

	defer destroyWindow(window)

	if err := initVk(window, 640, 480); err != nil {
		log.Fatalf("error initializing Vulkan: %s", err)
	}

	defer cleanupVk()
	load()
	loop(window)
}

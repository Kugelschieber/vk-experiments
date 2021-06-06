package main

import (
	"errors"
	"github.com/go-gl/glfw/v3.3/glfw"
	vk "github.com/vulkan-go/vulkan"
	"log"
	"runtime"
)

var (
	vkInstance       vk.Instance
	vkPhysicalDevice vk.PhysicalDevice
	vkDevice         vk.Device
	vkQueue          vk.Queue
)

func init() {
	runtime.LockOSThread()
}

func createWindow(title string, width, height int) (*glfw.Window, error) {
	if err := glfw.Init(); err != nil {
		return nil, err
	}

	window, err := glfw.CreateWindow(width, height, title, nil, nil)

	if err != nil {
		return nil, err
	}

	window.MakeContextCurrent()

	for !window.ShouldClose() {
		window.SwapBuffers()
		glfw.PollEvents()
	}

	return window, nil
}

func destroyWindow(window *glfw.Window) {
	window.Destroy()
	glfw.Terminate()
}

func initVk(window *glfw.Window) error {
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

	// select the first device with Vulkan support
	var deviceCount uint32

	if result := vk.EnumeratePhysicalDevices(vkInstance, &deviceCount, nil); result != vk.Success {
		return errors.New("error counting Vulkan devices")
	}

	if deviceCount == 0 {
		return errors.New("no device with Vulkan support found")
	}

	var devices []vk.PhysicalDevice

	if result := vk.EnumeratePhysicalDevices(vkInstance, &deviceCount, devices); result != vk.Success {
		return errors.New("error listing Vulkan devices")
	}

	for _, d := range devices {
		//var properties []vk.DisplayProperties
		//vk.GetPhysicalDeviceDisplayProperties(d, nil, properties)
		vkPhysicalDevice = d
		break // TODO check suitability
	}

	if vkPhysicalDevice == nil {
		return errors.New("no suitable Vulkan device found")
	}

	// find a graphics queue
	var queueCount uint32
	var queueProperties []vk.QueueFamilyProperties
	vk.GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, nil)
	vk.GetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, queueProperties)
	var queueIndex uint32
	queueFound := false

	for i, q := range queueProperties {
		if vk.QueueFlagBits(q.QueueFlags)&vk.QueueGraphicsBit == 1 {
			queueIndex = uint32(i)
			queueFound = true
			break
		}
	}

	if !queueFound {
		return errors.New("no Vulkan graphics bit queue found")
	}

	// create a logical device and get queue
	queues := []vk.DeviceQueueCreateInfo{
		{
			SType:            vk.StructureTypeDeviceQueueCreateInfo,
			QueueFamilyIndex: queueIndex,
			QueueCount:       1,
			PQueuePriorities: []float32{1},
		},
	}

	result = vk.CreateDevice(vkPhysicalDevice, &vk.DeviceCreateInfo{
		SType:                vk.StructureTypeDeviceCreateInfo,
		QueueCreateInfoCount: 1,
		PQueueCreateInfos:    queues,
	}, nil, &vkDevice)

	if result != vk.Success {
		return errors.New("error creating logical Vulkan device")
	}

	vk.GetDeviceQueue(vkDevice, queueIndex, 0, &vkQueue)
	return nil
}

func cleanupVk() {
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

	if err := initVk(window); err != nil {
		log.Fatalf("error initializing Vulkan: %s", err)
	}

	defer cleanupVk()
	load()
	loop(window)
}

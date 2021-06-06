package main

import (
	"github.com/go-gl/glfw/v3.3/glfw"
	vk "github.com/vulkan-go/vulkan"
	"log"
	"runtime"
)

var (
	vkInstance vk.Instance
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

func initVk() error {
	vk.SetGetInstanceProcAddr(glfw.GetVulkanGetInstanceProcAddress())

	if err := vk.Init(); err != nil {
		return err
	}

	return nil
}

func load() {
	vk.CreateInstance(&vk.InstanceCreateInfo{
		EnabledLayerCount: 0,
	}, nil, &vkInstance)
}

func cleanup() {
	vk.DestroyInstance(vkInstance, nil)
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

	defer glfw.Terminate()

	if err := initVk(); err != nil {
		log.Fatalf("error initializing Vulkan: %s", err)
	}

	load()
	defer cleanup()
	loop(window)
}

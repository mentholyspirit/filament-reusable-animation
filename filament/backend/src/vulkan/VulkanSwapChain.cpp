/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VulkanSwapChain.h"
#include "VulkanTexture.h"

#include <utils/FixedCapacityVector.h>
#include <utils/Panic.h>

using namespace bluevk;
using namespace utils;

namespace filament::backend {

VulkanSwapChain::VulkanSwapChain(VulkanPlatform* platform, VulkanContext const& context,
        VmaAllocator allocator, VulkanCommands* commands, VulkanStagePool& stagePool,
        void* nativeWindow, uint64_t flags, VkExtent2D extent)
    : VulkanResource(VulkanResourceType::SWAP_CHAIN),
      mPlatform(platform),
      mCommands(commands),
      mAllocator(allocator),
      mStagePool(stagePool),
      mHeadless(extent.width != 0 && extent.height != 0 && !nativeWindow),
      mFlushAndWaitOnResize(platform->getCustomization().flushAndWaitOnWindowResize),
      mCurrentImageReadyIndex(0),
      mAcquired(false),
      mIsFirstRenderPass(true) {
    swapChain = mPlatform->createSwapChain(nativeWindow, flags, extent);
    FILAMENT_CHECK_POSTCONDITION(swapChain) << "Unable to create swapchain";

    VkSemaphoreCreateInfo const createInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    // No need to wait on this semaphore before drawing when in Headless mode.
    if (mHeadless) {
		// Set all sempahores to VK_NULL_HANDLE
		memset(mImageReady, 0, sizeof(mImageReady[0]) * IMAGE_READY_SEMAPHORE_COUNT);
	} else {
		for (uint32_t i = 0; i < IMAGE_READY_SEMAPHORE_COUNT; ++i) {
			VkResult result =
					vkCreateSemaphore(mPlatform->getDevice(), &createInfo, nullptr, mImageReady + i);
                        FILAMENT_CHECK_POSTCONDITION(result == VK_SUCCESS)
                                << "Failed to create semaphore";
                }
    }

    update();
}

VulkanSwapChain::~VulkanSwapChain() {
    // Must wait for the inflight command buffers to finish since they might contain the images
    // we're about to destroy.
    mCommands->flush();
    mCommands->wait();

    mPlatform->destroy(swapChain);
	for (uint32_t i = 0; i < IMAGE_READY_SEMAPHORE_COUNT; ++i) {
		if (mImageReady[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(mPlatform->getDevice(), mImageReady[i], VKALLOC);
		}
	}
}

void VulkanSwapChain::update() {
    mColors.clear();

    auto const bundle = mPlatform->getSwapChainBundle(swapChain);
    mColors.reserve(bundle.colors.size());
    VkDevice const device = mPlatform->getDevice();

    for (auto const color: bundle.colors) {
        mColors.push_back(std::make_unique<VulkanTexture>(device, mAllocator, mCommands, color,
                bundle.colorFormat, 1, bundle.extent.width, bundle.extent.height,
                TextureUsage::COLOR_ATTACHMENT, mStagePool, true /* heap allocated */));
    }
    mDepth = std::make_unique<VulkanTexture>(device, mAllocator, mCommands, bundle.depth,
            bundle.depthFormat, 1, bundle.extent.width, bundle.extent.height,
            TextureUsage::DEPTH_ATTACHMENT, mStagePool, true /* heap allocated */);

    mExtent = bundle.extent;
}

void VulkanSwapChain::present() {
    if (!mHeadless) {
        VkCommandBuffer const cmdbuf = mCommands->get().buffer();
        VkImageSubresourceRange const subresources{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
        };
        mColors[mCurrentSwapIndex]->transitionLayout(cmdbuf, subresources, VulkanLayout::PRESENT);
    }
    mCommands->flush();

    // We only present if it is not headless.  No-op for headless (but note that we still need the
    // flush() in the above line).
    if (!mHeadless) {
        VkSemaphore const finishedDrawing = mCommands->acquireFinishedSignal();
        VkResult const result = mPlatform->present(swapChain, mCurrentSwapIndex, finishedDrawing);
        FILAMENT_CHECK_POSTCONDITION(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR ||
                result == VK_ERROR_OUT_OF_DATE_KHR)
                << "Cannot present in swapchain.";
    }

    // We presented the last acquired buffer.
    mAcquired = false;
    mIsFirstRenderPass = true;
}

void VulkanSwapChain::acquire(bool& resized) {
    // It's ok to call acquire multiple times due to it being linked to Driver::makeCurrent().
    if (mAcquired) {
        return;
    }

    // Check if the swapchain should be resized.
    if ((resized = mPlatform->hasResized(swapChain))) {
        if (mFlushAndWaitOnResize) {
            mCommands->flush();
            mCommands->wait();
        }
        mPlatform->recreate(swapChain);
        update();
    }

	mCurrentImageReadyIndex = (mCurrentImageReadyIndex + 1) % IMAGE_READY_SEMAPHORE_COUNT;
	const VkSemaphore imageReady = mImageReady[mCurrentImageReadyIndex];
    VkResult const result = mPlatform->acquire(swapChain, imageReady, &mCurrentSwapIndex);
    FILAMENT_CHECK_POSTCONDITION(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
            << "Cannot acquire in swapchain.";
    if (imageReady != VK_NULL_HANDLE) {
        mCommands->injectDependency(imageReady);
    }
    mAcquired = true;
}

}// namespace filament::backend

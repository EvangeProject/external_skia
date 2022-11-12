/*
 * Copyright 2022 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/graphite/dawn/DawnCaps.h"

#include <algorithm>

#include "include/gpu/graphite/TextureInfo.h"
#include "src/gpu/graphite/AttachmentTypes.h"
#include "src/gpu/graphite/ComputePipelineDesc.h"
#include "src/gpu/graphite/GraphicsPipelineDesc.h"
#include "src/gpu/graphite/GraphiteResourceKey.h"
#include "src/gpu/graphite/UniformManager.h"
#include "src/gpu/graphite/dawn/DawnUtils.h"


namespace {

// These are all the valid wgpu::TextureFormat that we currently support in Skia.
// They are roughly ordered from most frequently used to least to improve look
// up times in arrays.
static constexpr wgpu::TextureFormat kFormats[] = {
    wgpu::TextureFormat::RGBA8Unorm,
    wgpu::TextureFormat::R8Unorm,
    wgpu::TextureFormat::BGRA8Unorm,
    wgpu::TextureFormat::RGBA16Float,
    wgpu::TextureFormat::Undefined,
};

}

namespace skgpu::graphite {

DawnCaps::DawnCaps(const wgpu::Device& device, const ContextOptions& options)
    : Caps() {
    this->initCaps(device);
    this->initFormatTable(device);
    this->finishInitialization(options);
}

DawnCaps::~DawnCaps() = default;

bool DawnCaps::onIsTexturable(const TextureInfo& info) const {
    if (!(info.dawnTextureSpec().fUsage & wgpu::TextureUsage::TextureBinding)) {
        return false;
    }
    return this->isTexturable(info.dawnTextureSpec().fFormat);
}

bool DawnCaps::isTexturable(wgpu::TextureFormat format) const {
    const FormatInfo& formatInfo = this->getFormatInfo(format);
    return SkToBool(FormatInfo::kTexturable_Flag & formatInfo.fFlags);
}

bool DawnCaps::isRenderable(const TextureInfo& info) const {
    return info.dawnTextureSpec().fUsage & wgpu::TextureUsage::RenderAttachment &&
    this->isRenderable(info.dawnTextureSpec().fFormat, info.numSamples());
}

uint32_t DawnCaps::maxRenderTargetSampleCount(wgpu::TextureFormat format) const {
    const FormatInfo& formatInfo = this->getFormatInfo(format);
    if (!SkToBool(formatInfo.fFlags & FormatInfo::kRenderable_Flag)) {
        return 0;
    }
    if (SkToBool(formatInfo.fFlags & FormatInfo::kMSAA_Flag)) {
        return 8;
    } else {
        return 1;
    }
}

bool DawnCaps::isRenderable(wgpu::TextureFormat format, uint32_t sampleCount) const {
    return sampleCount <= this->maxRenderTargetSampleCount(format);
}

TextureInfo DawnCaps::getDefaultSampledTextureInfo(SkColorType colorType,
                                                   Mipmapped mipmapped,
                                                   Protected,
                                                   Renderable renderable) const {
    wgpu::TextureUsage usage = wgpu::TextureUsage::TextureBinding |
                               wgpu::TextureUsage::CopyDst |
                               wgpu::TextureUsage::CopySrc;
    if (renderable == Renderable::kYes) {
        usage |= wgpu::TextureUsage::RenderAttachment;
    }

    wgpu::TextureFormat format = this->getFormatFromColorType(colorType);
    if (format == wgpu::TextureFormat::Undefined) {
        SkDebugf("colorType=%d is not supported\n", static_cast<int>(colorType));
        return {};
    }

    DawnTextureInfo info;
    info.fSampleCount = 1;
    info.fMipmapped = mipmapped;
    info.fFormat = format;
    info.fUsage = usage;

    return info;
}

TextureInfo DawnCaps::getDefaultMSAATextureInfo(const TextureInfo& singleSampledInfo,
                                                Discardable discardable) const {
    const DawnTextureSpec& singleSpec = singleSampledInfo.dawnTextureSpec();

    DawnTextureInfo info;
    info.fSampleCount = this->defaultMSAASamples();
    info.fMipmapped   = Mipmapped::kNo;
    info.fFormat      = singleSpec.fFormat;
    info.fUsage       = wgpu::TextureUsage::RenderAttachment;
    return info;
}

TextureInfo DawnCaps::getDefaultDepthStencilTextureInfo(
    SkEnumBitMask<DepthStencilFlags> depthStencilType,
    uint32_t sampleCount,
    Protected) const {
    DawnTextureInfo info;
    info.fSampleCount = sampleCount;
    info.fMipmapped   = Mipmapped::kNo;
    info.fFormat      = DawnDepthStencilFlagsToFormat(depthStencilType);
    info.fUsage       = wgpu::TextureUsage::RenderAttachment;
    return info;
}

const Caps::ColorTypeInfo* DawnCaps::getColorTypeInfo(SkColorType colorType,
                                                      const TextureInfo& textureInfo) const {
    auto dawnFormat = textureInfo.dawnTextureSpec().fFormat;
    if (dawnFormat == wgpu::TextureFormat::Undefined) {
        SkASSERT(false);
        return nullptr;
    }

    const FormatInfo& info = this->getFormatInfo(dawnFormat);
    for (int i = 0; i < info.fColorTypeInfoCount; ++i) {
        const ColorTypeInfo& ctInfo = info.fColorTypeInfos[i];
        if (ctInfo.fColorType == colorType) {
            return &ctInfo;
        }
    }

    SkASSERT(false);
    return nullptr;
}

size_t DawnCaps::getTransferBufferAlignment(size_t bytesPerPixel) const {
    return std::max(bytesPerPixel, this->getMinBufferAlignment());
}

bool DawnCaps::supportsWritePixels(const TextureInfo& textureInfo) const {
    const auto& spec = textureInfo.dawnTextureSpec();
    return spec.fUsage & wgpu::TextureUsage::CopyDst;
}

bool DawnCaps::supportsReadPixels(const TextureInfo& textureInfo) const {
    const auto& spec = textureInfo.dawnTextureSpec();
    return spec.fUsage & wgpu::TextureUsage::CopySrc;
}

SkColorType DawnCaps::supportedWritePixelsColorType(SkColorType dstColorType,
                                                    const TextureInfo& dstTextureInfo,
                                                    SkColorType srcColorType) const {
    SkASSERT(false);
    return kUnknown_SkColorType;
}

SkColorType DawnCaps::supportedReadPixelsColorType(SkColorType srcColorType,
                                                   const TextureInfo& srcTextureInfo,
                                                   SkColorType dstColorType) const {
    SkASSERT(false);
    return kUnknown_SkColorType;
}

void DawnCaps::initCaps(const wgpu::Device& device) {
    wgpu::SupportedLimits limits;
    if (!device.GetLimits(&limits)) {
        SkASSERT(false);
    }
    fMaxTextureSize = limits.limits.maxTextureDimension2D;

    fRequiredUniformBufferAlignment = 256;
    fRequiredStorageBufferAlignment = fRequiredUniformBufferAlignment;

    // TODO: support storage buffer
    fStorageBufferSupport = false;
    fStorageBufferPreferred = false;

    // TODO: support clamp to border.
    fClampToBorderSupport = false;
}

void DawnCaps::initFormatTable(const wgpu::Device& device) {
    FormatInfo* info;
    // Format: RGBA8Unorm
    {
        info = &fFormatTable[GetFormatIndex(wgpu::TextureFormat::RGBA8Unorm)];
        info->fFlags = FormatInfo::kAllFlags;
        info->fColorTypeInfoCount = 2;
        info->fColorTypeInfos.reset(new ColorTypeInfo[info->fColorTypeInfoCount]());
        int ctIdx = 0;
        // Format: RGBA8Unorm, Surface: kRGBA_8888
        {
            auto& ctInfo = info->fColorTypeInfos[ctIdx++];
            ctInfo.fColorType = kRGBA_8888_SkColorType;
            ctInfo.fFlags = ColorTypeInfo::kUploadData_Flag | ColorTypeInfo::kRenderable_Flag;
        }
        // Format: RGBA8Unorm, Surface: kRGB_888x
        {
            auto& ctInfo = info->fColorTypeInfos[ctIdx++];
            ctInfo.fColorType = kRGB_888x_SkColorType;
            ctInfo.fFlags = ColorTypeInfo::kUploadData_Flag;
            ctInfo.fReadSwizzle = skgpu::Swizzle::RGB1();
        }
    }


    // Format: R8Unorm
    {
        info = &fFormatTable[GetFormatIndex(wgpu::TextureFormat::R8Unorm)];
        info->fFlags = FormatInfo::kAllFlags;
        info->fColorTypeInfoCount = 3;
        info->fColorTypeInfos.reset(new ColorTypeInfo[info->fColorTypeInfoCount]());
        int ctIdx = 0;
        // Format: R8Unorm, Surface: kR8_unorm
        {
            auto& ctInfo = info->fColorTypeInfos[ctIdx++];
            ctInfo.fColorType = kR8_unorm_SkColorType;
            ctInfo.fFlags = ColorTypeInfo::kUploadData_Flag | ColorTypeInfo::kRenderable_Flag;
        }
        // Format: R8Unorm, Surface: kAlpha_8
        {
            auto& ctInfo = info->fColorTypeInfos[ctIdx++];
            ctInfo.fColorType = kAlpha_8_SkColorType;
            ctInfo.fFlags = ColorTypeInfo::kUploadData_Flag | ColorTypeInfo::kRenderable_Flag;
            ctInfo.fReadSwizzle = skgpu::Swizzle("000r");
            ctInfo.fWriteSwizzle = skgpu::Swizzle("a000");
        }
        // Format: R8Unorm, Surface: kGray_8
        {
            auto& ctInfo = info->fColorTypeInfos[ctIdx++];
            ctInfo.fColorType = kGray_8_SkColorType;
            ctInfo.fFlags = ColorTypeInfo::kUploadData_Flag;
            ctInfo.fReadSwizzle = skgpu::Swizzle("rrr1");
        }
    }

    // Format: BGRA8Unorm
    {
        info = &fFormatTable[GetFormatIndex(wgpu::TextureFormat::BGRA8Unorm)];
        info->fFlags = FormatInfo::kAllFlags;
        info->fColorTypeInfoCount = 1;
        info->fColorTypeInfos.reset(new ColorTypeInfo[info->fColorTypeInfoCount]());
        int ctIdx = 0;
        // Format: BGRA8Unorm, Surface: kBGRA_8888
        {
            auto& ctInfo = info->fColorTypeInfos[ctIdx++];
            ctInfo.fColorType = kBGRA_8888_SkColorType;
            ctInfo.fFlags = ColorTypeInfo::kUploadData_Flag | ColorTypeInfo::kRenderable_Flag;
        }
    }

    // Format: RGBA16Float
    {
        info = &fFormatTable[GetFormatIndex(wgpu::TextureFormat::RGBA16Float)];
        info->fFlags = FormatInfo::kAllFlags;
        info->fColorTypeInfoCount = 1;
        info->fColorTypeInfos.reset(new ColorTypeInfo[info->fColorTypeInfoCount]());
        int ctIdx = 0;
        // Format: RGBA16Float, Surface: RGBA_F16
        {
            auto& ctInfo = info->fColorTypeInfos[ctIdx++];
            ctInfo.fColorType = kRGBA_F16_SkColorType;
            ctInfo.fFlags = ColorTypeInfo::kUploadData_Flag | ColorTypeInfo::kRenderable_Flag;
        }
    }

    // Format: Undefined
    {
        info = &fFormatTable[GetFormatIndex(wgpu::TextureFormat::Undefined)];
        info->fFlags = 0;
        info->fColorTypeInfoCount = 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Map SkColorTypes (used for creating SkSurfaces) to wgpu::TextureFormat.
    // The order in which the formats are passed into the setColorType function
    // indicates the priority in selecting which format we use for a given
    // SkColorType.

    std::fill_n(fColorTypeToFormatTable, kSkColorTypeCnt, wgpu::TextureFormat::Undefined);

    this->setColorType(kAlpha_8_SkColorType,          { wgpu::TextureFormat::R8Unorm });
    this->setColorType(kRGBA_8888_SkColorType,        { wgpu::TextureFormat::RGBA8Unorm });
    this->setColorType(kRGB_888x_SkColorType,         { wgpu::TextureFormat::RGBA8Unorm });
    this->setColorType(kBGRA_8888_SkColorType,        { wgpu::TextureFormat::BGRA8Unorm });
    this->setColorType(kGray_8_SkColorType,           { wgpu::TextureFormat::R8Unorm });
    this->setColorType(kR8_unorm_SkColorType,         { wgpu::TextureFormat::R8Unorm });
    this->setColorType(kRGBA_F16_SkColorType,         { wgpu::TextureFormat::RGBA16Float });
}

// static
size_t DawnCaps::GetFormatIndex(wgpu::TextureFormat format) {
    for (size_t i = 0; i < std::size(kFormats); ++i) {
        if (format == kFormats[i]) {
            return i;
        }
        if (kFormats[i] == wgpu::TextureFormat::Undefined) {
            SkDEBUGFAILF("Not supported wgpu::TextureFormat: %d\n", format);
            return i;
        }
    }
    SkUNREACHABLE;
    return 0;
}

void DawnCaps::setColorType(SkColorType colorType,
                            std::initializer_list<wgpu::TextureFormat> formats) {
    static_assert(std::tuple_size<decltype(fFormatTable)>::value == std::size(kFormats),
                  "Size is not same for DawnCaps::fFormatTable and kFormats");
#ifdef SK_DEBUG
    for (size_t i = 0; i < std::size(fFormatTable); ++i) {
        const auto& formatInfo = fFormatTable[i];
        for (int j = 0; j < formatInfo.fColorTypeInfoCount; ++j) {
            const auto& ctInfo = formatInfo.fColorTypeInfos[j];
            if (ctInfo.fColorType == colorType) {
                bool found = false;
                for (auto it = formats.begin(); it != formats.end(); ++it) {
                    if (kFormats[i] == *it) {
                        found = true;
                    }
                }
                SkASSERT(found);
            }
        }
    }
#endif
    int idx = static_cast<int>(colorType);
    for (auto it = formats.begin(); it != formats.end(); ++it) {
        const auto& info = this->getFormatInfo(*it);
        for (int i = 0; i < info.fColorTypeInfoCount; ++i) {
            if (info.fColorTypeInfos[i].fColorType == colorType) {
                fColorTypeToFormatTable[idx] = *it;
                return;
            }
        }
    }
}

UniqueKey DawnCaps::makeGraphicsPipelineKey(const GraphicsPipelineDesc& pipelineDesc,
                                            const RenderPassDesc& renderPassDesc) const {
    return {};
}

UniqueKey DawnCaps::makeComputePipelineKey(const ComputePipelineDesc& pipelineDesc) const {
    return {};
}

void DawnCaps::buildKeyForTexture(SkISize dimensions,
                                  const TextureInfo& info,
                                  ResourceType type,
                                  Shareable shareable,
                                  GraphiteResourceKey* key) const {}

} // namespace skgpu::graphite


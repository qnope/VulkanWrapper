# Image Module

`vw` namespace · `Image/` directory · Tier 2

GPU image management with RAII. Image has no VMA dependency — cleanup is handled via `shared_ptr` custom deleter from Allocator.

## Image

Created via Allocator, returns `shared_ptr<const Image>` with VMA cleanup deleter:

```cpp
auto image = allocator->create_image_2D(Width{w}, Height{h}, false, format, usage);
```

## ImageView / ImageViewBuilder

View into an Image with configurable aspect, type, and format:

```cpp
auto view = ImageViewBuilder(device, image)
    .set_aspect(eColor)
    .setImageType(vk::ImageViewType::e2D)
    .build();  // returns shared_ptr<const ImageView>
```

## Sampler / SamplerBuilder

Texture sampling configuration:

```cpp
auto sampler = SamplerBuilder(device)
    .set_filter(eLinear)
    .build();  // returns shared_ptr<const Sampler>
```

## CombinedImage

Pairs `ImageView` + `Sampler` for descriptor binding. Used in descriptor set writes for combined image samplers.

## Mipmap

Mipmap generation utilities — generates mip chain from base level using blit operations.

## ImageLoader

Loads images from disk via SDL_image. Returns image data ready for GPU upload.

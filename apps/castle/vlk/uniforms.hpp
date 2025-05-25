#pragma once

#include <libs/memtricks/arity.hpp>
#include <libs/memtricks/monotonic_arena.hpp>
#include <libs/memtricks/object_bytes.hpp>

#include "buf.hpp"

namespace vlk {

namespace ubo {

template <typename T>
using unique_ptr = monotonic_arena<mapped_memory>::unique_ptr<T>;

}

struct combined_image_sampler {
  vk::Sampler sampler;
  vk::ImageView image;
};

constexpr bool is_image_descriptor_type(vk::DescriptorType type) noexcept {
  using enum vk::DescriptorType;
  switch (type) {
  case eCombinedImageSampler:
  case eSampler:
  case eSampledImage:
  case eStorageImage:
  case eSampleWeightImageQCOM:
  case eBlockMatchImageQCOM:
    return true;

  case eUniformBuffer:
  case eUniformTexelBuffer:
  case eStorageTexelBuffer:
  case eUniformBufferDynamic:
  case eStorageBufferDynamic:
  case eInlineUniformBlock:
  case eInputAttachment:
  case eStorageBuffer:
  case eAccelerationStructureKHR:
  case eAccelerationStructureNV:
  case eMutableEXT:
    break;
  }
  return false;
}

template <vk::ShaderStageFlagBits S, typename T>
struct uniform {
  using value_type = T;
  using pointer = ubo::unique_ptr<T>;
  static constexpr uint32_t count = arity_v<T>;
  static constexpr vk::ShaderStageFlags stages = S;
  static constexpr vk::DescriptorType descriptor_type = vk::DescriptorType::eUniformBuffer;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(uint32_t binding_idx) {
    return vk::DescriptorSetLayoutBinding{}
        .setBinding(binding_idx)
        .setDescriptorType(descriptor_type)
        .setDescriptorCount(count)
        .setStageFlags(stages);
  }
};

template <vk::ShaderStageFlagBits S>
struct uniform<S, combined_image_sampler> {
  using value_type = combined_image_sampler;
  static constexpr uint32_t count = 1;
  static constexpr vk::ShaderStageFlags stages = S;
  static constexpr vk::DescriptorType descriptor_type = vk::DescriptorType::eCombinedImageSampler;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(uint32_t binding_idx) {
    return vk::DescriptorSetLayoutBinding{}
        .setBinding(binding_idx)
        .setDescriptorType(descriptor_type)
        .setDescriptorCount(count)
        .setStageFlags(stages);
  }

  static constexpr vk::DescriptorImageInfo make_descriptor_info(const value_type& val) {
    return vk::DescriptorImageInfo{}
        .setSampler(val.sampler)
        .setImageView(val.image)
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
  }
};

template <vk::ShaderStageFlagBits S>
struct uniform<S, vk::Sampler> {
  using value_type = vk::Sampler;
  static constexpr uint32_t count = 1;
  static constexpr vk::ShaderStageFlags stages = S;
  static constexpr vk::DescriptorType descriptor_type = vk::DescriptorType::eSampler;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(uint32_t binding_idx) {
    return vk::DescriptorSetLayoutBinding{}
        .setBinding(binding_idx)
        .setDescriptorType(descriptor_type)
        .setDescriptorCount(count)
        .setStageFlags(stages);
  }

  static constexpr vk::DescriptorImageInfo make_descriptor_info(const value_type& val) {
    return vk::DescriptorImageInfo{}.setSampler(val).setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
  }
};

template <vk::ShaderStageFlagBits S>
struct uniform<S, vk::ImageView> {
  using value_type = vk::ImageView;
  static constexpr uint32_t count = 1;
  static constexpr vk::ShaderStageFlags stages = S;
  static constexpr vk::DescriptorType descriptor_type = vk::DescriptorType::eSampledImage;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(uint32_t binding_idx) {
    return vk::DescriptorSetLayoutBinding{}
        .setBinding(binding_idx)
        .setDescriptorType(descriptor_type)
        .setDescriptorCount(count)
        .setStageFlags(stages);
  }

  static constexpr vk::DescriptorImageInfo make_descriptor_info(const value_type& val) {
    return vk::DescriptorImageInfo{}.setImageView(val).setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal
    );
  }
};

template <vk::ShaderStageFlagBits S, size_t N>
struct uniform<S, vk::ImageView[N]> {
  using value_type = std::span<const vk::ImageView>;
  static constexpr uint32_t count = N;
  static constexpr vk::ShaderStageFlags stages = S;
  static constexpr vk::DescriptorType descriptor_type = vk::DescriptorType::eSampledImage;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(uint32_t binding_idx) {
    return vk::DescriptorSetLayoutBinding{}
        .setBinding(binding_idx)
        .setDescriptorType(descriptor_type)
        .setDescriptorCount(count)
        .setStageFlags(stages);
  }

  template <std::output_iterator<vk::DescriptorImageInfo> OIt>
  static constexpr void make_descriptor_info(const value_type& val, OIt oit) {
    assert(val.size() == count);
    std::ranges::transform(val, oit, uniform<S, vk::ImageView>::make_descriptor_info);
  }
};

template <typename T>
using vertex_uniform = uniform<vk::ShaderStageFlagBits::eVertex, T>;
template <typename T>
using fragment_uniform = uniform<vk::ShaderStageFlagBits::eFragment, T>;
template <typename T>
using graphics_uniform = uniform<vk::ShaderStageFlagBits::eAllGraphics, T>;

template <typename T>
struct is_uniform : std::false_type {};
template <vk::ShaderStageFlagBits S, typename T>
struct is_uniform<uniform<S, T>> : std::true_type {};

template <typename T>
concept uniform_type = is_uniform<T>::value;

class descriptor_pool_builder {
private:
  using enum vk::DescriptorType;

public:
  constexpr descriptor_pool_builder() noexcept = default;

  template <uniform_type... U>
  constexpr descriptor_pool_builder& add_binding(uint32_t times = 1) noexcept {
    sizes_[0].descriptorCount += times * descriptors_count<U...>(eUniformBuffer);
    sizes_[0].type = vk::DescriptorType::eUniformBuffer;

    sizes_[1].descriptorCount += times * descriptors_count<U...>(eCombinedImageSampler);
    sizes_[1].type = vk::DescriptorType::eCombinedImageSampler;

    sizes_[2].descriptorCount += times * descriptors_count<U...>(eSampler);
    sizes_[2].type = vk::DescriptorType::eSampler;

    sizes_[3].descriptorCount += times * descriptors_count<U...>(eSampledImage);
    sizes_[3].type = vk::DescriptorType::eSampledImage;
    return *this;
  }

  vk::raii::DescriptorPool build(const vk::raii::Device& dev, uint32_t max_sets_count) {
    std::span<vk::DescriptorPoolSize> nonempty{
        sizes_.begin(),
        std::ranges::remove_if(
            sizes_, [](uint32_t cnt) noexcept { return cnt == 0; }, &vk::DescriptorPoolSize::descriptorCount
        )
            .begin()
    };
    return {dev, vk::DescriptorPoolCreateInfo{}.setMaxSets(max_sets_count).setPoolSizes(nonempty)};
  }

private:
  template <uniform_type... U>
  static consteval uint32_t descriptors_count(vk::DescriptorType type) noexcept {
    return ((U::descriptor_type == type ? U::count : 0) + ...);
  }

private:
  std::array<vk::DescriptorPoolSize, 4> sizes_{
      vk::DescriptorPoolSize{}.setType(eUniformBuffer),
      vk::DescriptorPoolSize{}.setType(eCombinedImageSampler)
  };
};

template <uniform_type... U>
vk::raii::DescriptorSetLayout binding_layout(const vk::raii::Device& dev) {
  auto bindings = []<size_t... Is>(std::index_sequence<Is...>) noexcept {
    return std::array<vk::DescriptorSetLayoutBinding, sizeof...(Is)>{U::make_binding(Is)...};
  }(std::index_sequence_for<U...>{});

  return {dev, vk::DescriptorSetLayoutCreateInfo{}.setBindings(bindings)};
}

class ubo_builder {
public:
  ubo_builder(monotonic_arena<mapped_memory>& arena, vk::PhysicalDeviceLimits limits) noexcept
      : arena_{&arena}, min_align_{limits.minUniformBufferOffsetAlignment} {}

  ubo_builder(std::span<const std::byte> allocated_ubo) noexcept
      : arena_{}, min_align_{0}, ubo_mem_{allocated_ubo} {}

  template <uniform_type U, typename... A>
    requires(!is_image_descriptor_type(U::descriptor_type)) &&
            std::constructible_from<typename U::value_type, A...>
  typename U::pointer create(uint32_t binding, A&&... a) {
    auto res = arena_->aligned_allocate_unique<typename U::value_type>(
        std::align_val_t{std::max(min_align_, alignof(typename U::value_type))}, std::forward<A>(a)...
    );
    bind<U>(binding, *res);
    return res;
  }

  template <uniform_type U>
    requires(!is_image_descriptor_type(U::descriptor_type))
  void bind(uint32_t binding, const typename U::value_type& val) {
    const auto obj_mem = object_bytes(val);
    ubo_mem_ = std::span(ubo_mem_.data() ? ubo_mem_.data() : obj_mem.data(), obj_mem.data() + obj_mem.size());
    const auto region = subspan_region(ubo_mem_, obj_mem);

    write_desc_set_.push_back(
        vk::WriteDescriptorSet{}
            .setDstBinding(binding)
            .setDescriptorCount(U::count)
            .setDescriptorType(U::descriptor_type)
            // Attention voodo magick here. Size is passed instead of pointer to read it in the
            // place responsible to set proper pointer
            .setPBufferInfo(reinterpret_cast<vk::DescriptorBufferInfo*>(desc_buf_info_.size()))
    );
    desc_buf_info_.push_back(vk::DescriptorBufferInfo{}.setOffset(region.offset).setRange(region.len));
  }

  template <uniform_type U>
    requires(is_image_descriptor_type(U::descriptor_type))
  void bind(uint32_t binding, const typename U::value_type& val) {
    write_desc_set_.push_back(
        vk::WriteDescriptorSet{}
            .setDstBinding(binding)
            .setDescriptorCount(U::count)
            .setDescriptorType(U::descriptor_type)
            // Attention voodo magick here. Size is passed instead of pointer to read it in the
            // place responsible to set proper pointer
            .setPImageInfo(reinterpret_cast<vk::DescriptorImageInfo*>(desc_img_info_.size()))
    );
    if constexpr (U::count == 1)
      desc_img_info_.push_back(U::make_descriptor_info(val));
    else
      U::make_descriptor_info(val, std::back_inserter(desc_img_info_));
  }

  void update(vk::Device dev, vk::Buffer buf, vk::DescriptorSet set) {
    for (auto& inf : desc_buf_info_)
      inf.buffer = buf;

    for (auto& wrt : write_desc_set_) {
      wrt.dstSet = set;
      if (is_image_descriptor_type(wrt.descriptorType)) {
        wrt.pImageInfo = &desc_img_info_[reinterpret_cast<uintptr_t>(wrt.pImageInfo)];
      } else {
        wrt.pBufferInfo = &desc_buf_info_[reinterpret_cast<uintptr_t>(wrt.pBufferInfo)];
      }
    }
    dev.updateDescriptorSets(write_desc_set_, {});

    desc_buf_info_.clear();
    desc_img_info_.clear();
    write_desc_set_.clear();
  }

  std::tuple<vk::raii::Buffer, std::span<const std::byte>>
  build(const vk::raii::Device& dev, vk::DescriptorSet set) {
    vk::raii::Buffer res = nullptr;
    if (!ubo_mem_.empty()) {
      res = arena_->mem_source().bind_buffer(dev, vk::BufferUsageFlagBits::eUniformBuffer, ubo_mem_);
    }
    update(*dev, *res, set);

    return std::tuple{std::move(res), std::exchange(ubo_mem_, {})};
  }

private:
  monotonic_arena<mapped_memory>* arena_;
  size_t min_align_;
  std::span<const std::byte> ubo_mem_{};
  std::vector<vk::DescriptorBufferInfo> desc_buf_info_;
  std::vector<vk::DescriptorImageInfo> desc_img_info_;
  std::vector<vk::WriteDescriptorSet> write_desc_set_;
};

template <size_t N, uniform_type... U>
class pipeline_bindings {
public:
  template <typename C>
  pipeline_bindings(
      const vk::raii::Device& dev, vk::DescriptorPool desc_pool, monotonic_arena<mapped_memory>& ubo_arena,
      const vk::PhysicalDeviceLimits& limits, std::span<C, N> val
  )
      : bindings_layout_{binding_layout<U...>(dev)} {
    std::array<vk::DescriptorSetLayout, N> layouts;
    layouts.fill(layout());
    auto alloc_inf = vk::DescriptorSetAllocateInfo{}.setDescriptorPool(desc_pool).setSetLayouts(layouts);
    if (const auto ec = make_error_code((*dev).allocateDescriptorSets(&alloc_inf, desc_set_.data())))
      throw std::system_error{ec, "vkAllocateDescriptorSets"};

    ubo_builder bldr{ubo_arena, limits};
    for (auto [i, v] : val | std::views::enumerate) {
      v.bind(bldr);
      std::tie(buf_[i], flush_region_[i]) = bldr.build(dev, desc_set_[i]);
    }
  }

  const vk::DescriptorSetLayout& layout() const { return *bindings_layout_; }

  std::span<const std::byte> flush_region(size_t idx) const noexcept { return flush_region_[idx]; }

  void use(size_t idx, const vk::CommandBuffer& cmd, vk::PipelineLayout pipeline_layout) const {
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1, &desc_set_[idx], 0, nullptr
    );
  }

  template <typename C>
  void update(size_t idx, vk::Device dev, C& obj) {
    ubo_builder bldr{flush_region_[idx]};
    obj.rebind(bldr);
    bldr.update(dev, buf_[idx], desc_set_[idx]);
  }

private:
  template <size_t... Is>
  constexpr static std::array<vk::raii::Buffer, sizeof...(Is)>
  default_construct_bufs_array(std::index_sequence<Is...>) noexcept {
    return {vk::raii::Buffer{(static_cast<void>(Is), nullptr)}...};
  }

private:
  vk::raii::DescriptorSetLayout bindings_layout_;
  std::array<vk::DescriptorSet, N> desc_set_;
  std::array<vk::raii::Buffer, N> buf_ = default_construct_bufs_array(std::make_index_sequence<N>{});
  std::array<std::span<const std::byte>, N> flush_region_;
};

class uniform_pools {
public:
  uniform_pools(
      const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
      const vk::PhysicalDeviceLimits& limits, vk::raii::DescriptorPool desc_pool, size_t ubo_capacity
  )
      : desc_pool_{std::move(desc_pool)},
        arena_{
            mapped_memory::allocate(dev, props, limits, vk::BufferUsageFlagBits::eUniformBuffer, ubo_capacity)
        } {}

  void flush(std::span<const std::byte> flush_region) const { arena_.mem_source().flush(flush_region); }

  template <typename C, size_t N, uniform_type... U>
  pipeline_bindings<N, U...> make_pipeline_bindings(
      const vk::raii::Device& dev, const vk::PhysicalDeviceLimits& limits, std::span<C, N> val
  ) {
    return {dev, desc_pool_, arena_, limits, val};
  }

  void clear() noexcept {
    arena_.clear();
    desc_pool_.reset({});
  }

private:
  vk::raii::DescriptorPool desc_pool_;
  monotonic_arena<mapped_memory> arena_;
};

} // namespace vlk

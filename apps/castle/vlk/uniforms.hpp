#pragma once

#include <libs/memtricks/array.hpp>
#include <libs/memtricks/monotonic_arena.hpp>
#include <libs/memtricks/object_bytes.hpp>

#include "buf.hpp"

namespace vlk {

namespace ubo {

template <typename T>
using unique_ptr = monotonic_arena<mapped_memory>::unique_ptr<T>;

}

template <vk::ShaderStageFlagBits S, typename T>
struct uniform {
  using value_type = T;
  using pointer = ubo::unique_ptr<T>;
  static constexpr uint32_t count = arity_v<T>;
  static constexpr vk::ShaderStageFlags stages = S;
  static constexpr vk::DescriptorType descriptor_type =
      vk::DescriptorType::eUniformBuffer;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(
      uint32_t binding_idx) {
    return {
        .binding = binding_idx,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = count,
        .stageFlags = stages,
        .pImmutableSamplers = nullptr,
    };
  }
};

template <vk::ShaderStageFlagBits S>
struct uniform<S, vk::Sampler> {
  using value_type = vk::Sampler;
  static constexpr uint32_t count = 1;
  static constexpr vk::ShaderStageFlags stages = S;
  static constexpr vk::DescriptorType descriptor_type =
      vk::DescriptorType::eCombinedImageSampler;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(
      uint32_t binding_idx) {
    return {
        .binding = binding_idx,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = count,
        .stageFlags = stages,
        .pImmutableSamplers = nullptr,
    };
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
  constexpr descriptor_pool_builder& add_binding() noexcept {
    sizes_[0].descriptorCount = std::max(
        sizes_[0].descriptorCount, descriptors_count<U...>(eUniformBuffer));
    sizes_[1].descriptorCount = std::max(sizes_[1].descriptorCount,
        descriptors_count<U...>(eCombinedImageSampler));
    return *this;
  }

  vk::raii::DescriptorPool build(
      const vk::raii::Device& dev, uint32_t max_sets_count) {
    std::span<vk::DescriptorPoolSize> nonempty{sizes_.begin(),
        std::ranges::remove_if(
            sizes_, [](uint32_t cnt) noexcept { return cnt == 0; },
            &vk::DescriptorPoolSize::descriptorCount)
            .begin()};
    return {dev, vk::DescriptorPoolCreateInfo{.maxSets = max_sets_count,
                     .poolSizeCount = static_cast<uint32_t>(nonempty.size()),
                     .pPoolSizes = nonempty.data()}};
  }

private:
  template <uniform_type... U>
  static consteval uint32_t descriptors_count(
      vk::DescriptorType type) noexcept {
    return ((U::descriptor_type == type ? 1 : 0) + ...);
  }

private:
  std::array<vk::DescriptorPoolSize, 2> sizes_{
      vk::DescriptorPoolSize{.type = eUniformBuffer, .descriptorCount = 0},
      vk::DescriptorPoolSize{
          .type = eCombinedImageSampler, .descriptorCount = 0}};
};

template <uniform_type... U>
vk::raii::DescriptorSetLayout binding_layout(const vk::raii::Device& dev) {
  auto bindings = []<size_t... Is>(std::index_sequence<Is...>) noexcept {
    return std::array<vk::DescriptorSetLayoutBinding, sizeof...(Is)>{
        U::make_binding(Is)...};
  }(std::index_sequence_for<U...>{});

  return {
      dev, vk::DescriptorSetLayoutCreateInfo{
               .bindingCount = bindings.size(), .pBindings = bindings.data()}};
}

class ubo_builder {
public:
  ubo_builder(monotonic_arena<mapped_memory>& arena,
      vk::PhysicalDeviceLimits limits) noexcept
      : arena_{&arena}, min_align_{limits.minUniformBufferOffsetAlignment} {}

  template <uniform_type U, typename... A>
    requires(U::descriptor_type == vk::DescriptorType::eUniformBuffer) &&
            std::constructible_from<typename U::value_type, A...>
  typename U::pointer create(uint32_t binding, A&&... a) {
    auto res = arena_->aligned_allocate_unique<typename U::value_type>(
        std::align_val_t{std::max(min_align_, alignof(typename U::value_type))},
        std::forward<A>(a)...);
    const auto obj_mem = object_bytes(*res);
    ubo_mem_ = std::span(ubo_mem_.data() ? ubo_mem_.data() : obj_mem.data(),
        obj_mem.data() + obj_mem.size());
    const auto region = subspan_region(ubo_mem_, obj_mem);

    write_desc_set_.push_back({.dstSet = nullptr,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pImageInfo = nullptr,
        .pBufferInfo =
            reinterpret_cast<vk::DescriptorBufferInfo*>(desc_buf_info_.size()),
        .pTexelBufferView = nullptr});
    desc_buf_info_.push_back(
        {.buffer = nullptr, .offset = region.offset, .range = region.len});
    return res;
  }

  template <uniform_type U>
    requires(U::descriptor_type == vk::DescriptorType::eCombinedImageSampler)
  void bind_sampler(uint32_t binding, vk::Sampler sampler, vk::ImageView img) {
    write_desc_set_.push_back({.dstSet = nullptr,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo =
            reinterpret_cast<vk::DescriptorImageInfo*>(desc_img_info_.size()),
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr});
    desc_img_info_.push_back({.sampler = sampler,
        .imageView = img,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal});
  }

  std::tuple<vk::raii::Buffer, memory_region> build(
      const vk::raii::Device& dev, vk::DescriptorSet set) {
    vk::raii::Buffer res = nullptr;
    if (!ubo_mem_.empty()) {
      res = arena_->mem_source().bind_buffer(
          dev, vk::BufferUsageFlagBits::eUniformBuffer, ubo_mem_);
    }
    for (auto& inf : desc_buf_info_)
      inf.buffer = *res;

    for (auto& wrt : write_desc_set_) {
      wrt.dstSet = set;
      if (wrt.descriptorType == vk::DescriptorType::eCombinedImageSampler) {
        wrt.pImageInfo =
            &desc_img_info_[reinterpret_cast<uintptr_t>(wrt.pImageInfo)];
      }
      if (wrt.descriptorType == vk::DescriptorType::eUniformBuffer) {
        wrt.pBufferInfo =
            &desc_buf_info_[reinterpret_cast<uintptr_t>(wrt.pBufferInfo)];
      }
    }
    dev.updateDescriptorSets(write_desc_set_, {});

    return std::tuple{
        std::move(res), subspan_region(arena_->mem_source(), ubo_mem_)};
  }

private:
  monotonic_arena<mapped_memory>* arena_;
  size_t min_align_;
  std::span<const std::byte> ubo_mem_{};
  std::vector<vk::DescriptorBufferInfo> desc_buf_info_;
  std::vector<vk::DescriptorImageInfo> desc_img_info_;
  std::vector<vk::WriteDescriptorSet> write_desc_set_;
};

template <uniform_type... U>
class pipeline_bindings {
public:
  pipeline_bindings(const vk::raii::Device& dev, vk::DescriptorPool desc_pool,
      monotonic_arena<mapped_memory>& ubo_arena,
      const vk::PhysicalDeviceLimits& limits)
      : bindings_layout_{binding_layout<U...>(dev)} {
    vk::DescriptorSetAllocateInfo alloc_inf{.descriptorPool = desc_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout()};
    if (const auto ec = make_error_code(
            (*dev).allocateDescriptorSets(&alloc_inf, &desc_set_)))
      throw std::system_error{ec, "vkAllocateDescriptorSets"};

    ubo_builder bldr{ubo_arena, limits};
    value_ = [&bldr]<size_t... Is>(std::index_sequence<Is...>) {
      return std::tuple{bldr.create<U>(Is)...};
    }(std::index_sequence_for<U...>{});
    std::tie(buf_, flush_region_) = bldr.build(dev, desc_set_);
  }

  const vk::DescriptorSetLayout& layout() const { return *bindings_layout_; }

  auto& value() const noexcept { return value_; }
  memory_region flush_region() const noexcept { return flush_region_; }

  void use(
      const vk::CommandBuffer& cmd, vk::PipelineLayout pipeline_layout) const {
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0,
        1, &desc_set_, 0, nullptr);
  }

private:
  vk::raii::DescriptorSetLayout bindings_layout_;
  std::tuple<typename U::pointer...> value_;
  vk::DescriptorSet desc_set_;
  vk::raii::Buffer buf_ = nullptr;
  memory_region flush_region_;
};

class uniform_pools {
public:
  uniform_pools(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props,
      const vk::PhysicalDeviceLimits& limits,
      vk::raii::DescriptorPool desc_pool, size_t ubo_capacity)
      : desc_pool_{std::move(desc_pool)},
        arena_{mapped_memory::allocate(dev, props, limits,
            vk::BufferUsageFlagBits::eUniformBuffer, ubo_capacity)} {}

  void flush(memory_region flush_region) const {
    arena_.mem_source().flush(flush_region);
  }

  template <uniform_type... U>
  pipeline_bindings<U...> make_pipeline_bindings(
      const vk::raii::Device& dev, const vk::PhysicalDeviceLimits& limits) {
    return {dev, desc_pool_, arena_, limits};
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

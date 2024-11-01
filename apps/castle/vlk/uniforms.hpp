#pragma once

#include <libs/memtricks/array.hpp>
#include <libs/memtricks/object_bytes.hpp>

#include "buf.hpp"
#include "pipelines.hpp"

namespace vlk {

namespace detail {

template <typename... Ts, size_t... Is>
void write_desc_set(vk::Device dev, vk::Buffer buf, vk::DescriptorSet set,
    const std::tuple<Ts...>& val, std::index_sequence<Is...>) {
  constexpr size_t cnt = sizeof...(Ts);
  std::array<vk::DescriptorBufferInfo, cnt> binfs{
      vk::DescriptorBufferInfo{.buffer = buf,
          .offset = static_cast<size_t>(
              reinterpret_cast<const std::byte*>(&std::get<Is>(val)) -
              reinterpret_cast<const std::byte*>(&val)),
          .range = sizeof(Ts)}...};
  std::array<vk::WriteDescriptorSet, cnt> winfs{
      vk::WriteDescriptorSet{.dstSet = set,
          .dstBinding = Is,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eUniformBuffer,
          .pImageInfo = nullptr,
          .pBufferInfo = &binfs[Is],
          .pTexelBufferView = nullptr}...};
  dev.updateDescriptorSets(winfs, {});
}

template <typename... Ts>
void write_desc_set(vk::Device dev, vk::Buffer buf, vk::DescriptorSet set,
    const std::tuple<Ts...>& val) {
  return write_desc_set(dev, buf, set, val, std::index_sequence_for<Ts...>{});
}

} // namespace detail

template <typename T>
class uniform_memory : public mapped_memory {
public:
  using value_type = T;

  uniform_memory() noexcept = default;
  template <typename... A>
  uniform_memory(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props,
      const vk::PhysicalDeviceLimits& limits, A&&... a)
      : mapped_memory{mapped_memory::allocate(dev, props, limits,
            vk::BufferUsageFlagBits::eUniformBuffer, sizeof(T))} {
    new (mapping().data()) T{std::forward<A>(a)...};
  }

  uniform_memory(const uniform_memory&) = delete;
  uniform_memory& operator=(const uniform_memory&) = delete;

  uniform_memory(uniform_memory&&) noexcept = default;
  uniform_memory& operator=(uniform_memory&&) noexcept = default;

  ~uniform_memory() noexcept {
    if (*this)
      reinterpret_cast<T*>(mapping().data())->~T();
  }

  operator bool() const noexcept { return !mapping().empty(); }

  T& operator*() const noexcept {
    return *reinterpret_cast<T*>(mapping().data());
  }
  T* operator->() const noexcept {
    return reinterpret_cast<T*>(mapping().data());
  }
};

template <vk::ShaderStageFlagBits S, typename T>
struct uniform {
  using value_type = T;
  static constexpr uint32_t count = arity_v<T>;
  static constexpr vk::ShaderStageFlags stages = S;

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

template <typename T>
using vertex_uniform = uniform<vk::ShaderStageFlagBits::eVertex, T>;
template <typename T>
using fragment_uniform = uniform<vk::ShaderStageFlagBits::eFragment, T>;
template <typename T>
using graphics_uniform = uniform<vk::ShaderStageFlagBits::eAllGraphics, T>;

template <typename... Ts>
class pipeline_bindings;

template <typename... Ts, vk::ShaderStageFlagBits... Ss>
class pipeline_bindings<uniform<Ss, Ts>...> : public pipeline_bindings_base {
public:
  pipeline_bindings(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props,
      const vk::PhysicalDeviceLimits& limits)
      : pipeline_bindings_base{make_layout(
            dev, std::index_sequence_for<Ts...>{})},
        desc_pool_{make_pool(dev)}, value_{dev, props, limits},
        buf_{value_.bind_buffer(dev, vk::BufferUsageFlagBits::eUniformBuffer,
            object_bytes(value()))} {
    fill_sets(*dev);
    detail::write_desc_set(dev, *buf_, desc_set_, *value_);
  }

  auto& value() const noexcept { return (*value_); }
  void flush() const { value_.flush(object_bytes(value())); }

  void use(
      const vk::CommandBuffer& cmd, vk::PipelineLayout pipeline_layout) const {
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0,
        1, &desc_set_, 0, nullptr);
  }

private:
  static vk::raii::DescriptorPool make_pool(const vk::raii::Device& dev) {
    vk::DescriptorPoolSize size{.type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = sizeof...(Ts)};
    return {dev, vk::DescriptorPoolCreateInfo{
                     .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &size}};
  }

  template <size_t... Is>
  static vk::raii::DescriptorSetLayout make_layout(
      const vk::raii::Device& dev, std::index_sequence<Is...>) {
    std::array<vk::DescriptorSetLayoutBinding, sizeof...(Ts)> bindings{
        {uniform<Ss, Ts>::make_binding(Is)...}};
    vk::DescriptorSetLayoutCreateInfo info{
        .bindingCount = bindings.size(), .pBindings = bindings.data()};
    return {dev, info};
  }

  void fill_sets(vk::Device dev) {
    vk::DescriptorSetAllocateInfo alloc_inf{.descriptorPool = *desc_pool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout()};
    if (const auto ec =
            make_error_code(dev.allocateDescriptorSets(&alloc_inf, &desc_set_)))
      throw std::system_error{ec, "vkAllocateDescriptorSets"};
  }

private:
  vk::raii::DescriptorPool desc_pool_ = nullptr;
  vk::DescriptorSet desc_set_;
  uniform_memory<std::tuple<Ts...>> value_;
  vk::raii::Buffer buf_;
};

} // namespace vlk

#pragma once

#include <libs/memtricks/array.hpp>

#include "buf.hpp"
#include "pipelines.hpp"

namespace vlk {

namespace detail {

template <typename... Ts, size_t... Is>
void write_desc_set(vk::Device dev, vk::Buffer buf, vk::DescriptorSet set,
    const std::tuple<Ts...>& val, std::index_sequence<Is...>) {
  constexpr size_t cnt = sizeof...(Ts);
  std::array<vk::DescriptorBufferInfo, cnt> binfs{{.buffer = buf,
      .offset = (static_cast<const std::byte*>(&std::get<Is>(val)) -
                 static_cast<const std::byte*>(&val)),
      .range = sizeof(Ts)}...};
  std::array<vk::WriteDescriptorSet, cnt> winfs{{.dstSet = set,
      .dstBinding = 0,
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
  return write_desc_set(buf, set, std::index_sequence_for<Ts...>{});
}

} // namespace detail

template <typename T>
class uniform_memory : public mapped_memory {
public:
  using value_type = T;

  uniform_memory() noexcept = default;
  template <typename... A>
  uniform_memory(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props, A&&... a)
      : mapped_memory{mapped_memory::allocate(dev, props, sizeof(T))} {
    new (data().data()) T{std::forward<A>(a)...};
  }

  uniform_memory(const uniform_memory&) = delete;
  uniform_memory& operator=(const uniform_memory&) = delete;

  uniform_memory(uniform_memory&&) noexcept = default;
  uniform_memory& operator=(uniform_memory&&) noexcept = default;

  ~uniform_memory() noexcept {
    if (*this)
      reinterpret_cast<T*>(data().data())->~T();
  }

  operator bool() const noexcept { return !data().empty(); }

  T& operator*() const noexcept { return *reinterpret_cast<T*>(data().data()); }
  T* operator->() const noexcept { return reinterpret_cast<T*>(data().data()); }
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

template <uint32_t N, typename... Ts>
class pipeline_bindings;

template <uint32_t N, typename... Ts, vk::ShaderStageFlagBits... Ss>
class pipeline_bindings<N, uniform<Ss, Ts>...> : public pipeline_bindings_base {
public:
  pipeline_bindings(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props)
      : pipeline_bindings_base{make_layout(
            dev, std::index_sequence_for<Ts...>{})},
        desc_pool_{make_pool(dev, N)}, value_{dev, props},
        bufs_{make_bufs(dev, std::make_index_sequence<N>{})} {
    fill_sets(*dev);
    configure_sets(*dev);
  }

  auto& value(uint32_t idx) const noexcept { return (*value_)[idx]; }
  void flush(uint32_t idx) const {
    value_.flush(elem_offset(idx), sizeof(std::tuple<Ts...>));
  }

  void use(uint32_t idx, const vk::CommandBuffer& cmd,
      vk::PipelineLayout pipeline_layout) const {
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0,
        1, &desc_sets_[idx], 0, nullptr);
  }

private:
  static vk::raii::DescriptorPool make_pool(
      const vk::raii::Device& dev, uint32_t count) {
    vk::DescriptorPoolSize size{.type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = sizeof...(Ts)};
    return {
        dev, vk::DescriptorPoolCreateInfo{
                 .maxSets = count, .poolSizeCount = 1, .pPoolSizes = &size}};
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
    std::array<vk::DescriptorSetLayout, N> layouts;
    layouts.fill(layout());
    vk::DescriptorSetAllocateInfo alloc_inf{.descriptorPool = *desc_pool_,
        .descriptorSetCount = N,
        .pSetLayouts = layouts.data()};
    if (const auto ec = make_error_code(
            dev.allocateDescriptorSets(&alloc_inf, desc_sets_.data())))
      throw std::system_error{ec, "vkAllocateDescriptorSets"};
  }

  template <size_t... Is>
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

  void configure_sets(vk::Device dev) {
    for (uint32_t idx = 0; idx < N; ++idx) {
      write_desc_set(dev, *bufs_[idx], desc_sets_[idx], (*value_)[idx],
          std::index_sequence_for<Ts...>{});
    };
  }

  template <size_t... Is>
  std::array<vk::raii::Buffer, N> make_bufs(
      const vk::raii::Device& dev, std::index_sequence<Is...>) {
    return std::array<vk::raii::Buffer, N>{
        value_.bind_buffer(dev, vk::BufferUsageFlagBits::eUniformBuffer,
            elem_offset(Is), sizeof(std::tuple<Ts...>))...};
  }

  size_t elem_offset(size_t idx) const noexcept {
    return reinterpret_cast<const std::byte*>(&(*value_)[idx]) -
           reinterpret_cast<const std::byte*>(value_->data());
  }

private:
  vk::raii::DescriptorPool desc_pool_ = nullptr;
  std::array<vk::DescriptorSet, N> desc_sets_;
  uniform_memory<std::array<std::tuple<Ts...>, N>> value_;
  std::array<vk::raii::Buffer, N> bufs_;
};

} // namespace vlk

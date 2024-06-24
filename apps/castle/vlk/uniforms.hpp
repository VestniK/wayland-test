#pragma once

#include <libs/memtricks/array.hpp>

#include "buf.hpp"
#include "pipelines.hpp"

namespace vlk {

template <typename T>
class uniform_memory : public mapped_memory {
public:
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

template <typename... Ts>
class pipeline_bindings;

template <typename... Ts, vk::ShaderStageFlagBits... Ss>
class pipeline_bindings<uniform<Ss, Ts>...> : public pipeline_bindings_base {
public:
  pipeline_bindings(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props)
      : pipeline_bindings_base{make_layout(
            dev, std::index_sequence_for<Ts...>{})},
        value_{dev, props} {}

  auto& value() const noexcept { return *value_; }
  void flush() const { value_.flush(); }

private:
  template <size_t... Is>
  static vk::raii::DescriptorSetLayout make_layout(
      const vk::raii::Device& dev, std::index_sequence<Is...>) {
    std::array<vk::DescriptorSetLayoutBinding, sizeof...(Ts)> bindings{
        {uniform<Ss, Ts>::make_binding(Is)...}};
    vk::DescriptorSetLayoutCreateInfo info{
        .bindingCount = bindings.size(), .pBindings = bindings.data()};
    return {dev, info};
  }

private:
  uniform_memory<std::tuple<Ts...>> value_;
};

} // namespace vlk

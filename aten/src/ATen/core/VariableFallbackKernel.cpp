#include <ATen/core/LegacyTypeDispatch.h>
#include <ATen/core/dispatch/Dispatcher.h>
#include <ATen/core/VariableHooksInterface.h>
#include <torch/library.h>

/*
 * This file implements a variable fallback kernel for custom operators.
 * Since tensors always have the Autograd set, but custom operators
 * usually don't have a kernel registered for Autograd, the dispatcher
 * will call into this fallback kernel instead.
 * Note that this is not a correct autograd implementation. It will just
 * fallthrough to the custom operator implementation.
 * If you want a custom operator to work with autograd, you need to use
 * autograd::Function so that the custom operator implementation knows how to
 * do autograd.
 * Note also that ops from native_functions.yaml register their own variable
 * kernels, so this is never called for them.
 */

// TODO This whole file should be deleted and replaced with the mechanism
//      described in https://github.com/pytorch/pytorch/issues/29548

using c10::Stack;

namespace {

// Register fallthrough for Autograd backends dispatch keys
// NB: But not the private use ones; maybe the extension wants
// to override it themselves!

void autograd_fallback(
    const c10::OperatorHandle& op,
    c10::DispatchKeySet dispatch_keys,
    torch::jit::Stack* stack);

#define AUTOGRAD_FALLBACK torch::CppFunction::makeFromBoxedFunction<&autograd_fallback>()

TORCH_LIBRARY_IMPL(_, AutogradOther, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

TORCH_LIBRARY_IMPL(_, AutogradCPU, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

TORCH_LIBRARY_IMPL(_, AutogradXPU, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

TORCH_LIBRARY_IMPL(_, AutogradCUDA, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

TORCH_LIBRARY_IMPL(_, AutogradXLA, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

TORCH_LIBRARY_IMPL(_, AutogradLazy, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

TORCH_LIBRARY_IMPL(_, AutogradMPS, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

TORCH_LIBRARY_IMPL(_, AutogradMeta, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

// see Note [ADInplaceOrView key]
TORCH_LIBRARY_IMPL(_, ADInplaceOrView, m) {
  m.fallback(torch::CppFunction::makeFallthrough());
}

TORCH_LIBRARY_IMPL(_, AutogradHPU, m) {
  m.fallback(AUTOGRAD_FALLBACK);
}

#undef AUTOGRAD_FALLBACK

void autograd_fallback(
    const c10::OperatorHandle& op,
    c10::DispatchKeySet dispatch_keys,
    torch::jit::Stack* stack) {
  // PyTorch has separate builds, some of which don't include autograd.
  // So we define some behavior for when autograd isn't included and
  // go through a layer of indirection (VariableHooksInterface) when it is.
  // See aten/src/ATen/core/VariableHooksInterface.h for more details.
  if (!at::impl::HasVariableHooks()) {
    op.redispatchBoxed(dispatch_keys & c10::after_autograd_keyset, stack);
    return;
  }
  at::impl::GetVariableHooks()->basic_autograd_not_implemented_fallback(op, dispatch_keys, stack);
}

} // namespace

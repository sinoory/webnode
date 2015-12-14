// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "config.h"
#include "third_party/node/deps/uv/include/uv.h"
#include "third_party/node/src/env.h"
#include "third_party/node/src/env-inl.h"
#include "node_bindings.h"

#include <string>
#include <vector>
#include <wtf/RunLoop.h>

// Force all builtin modules to be referenced so they can actually run their
// DSO constructors, see http://git.io/DRIqCg.
#define REFERENCE_MODULE(name) \
  extern "C" void _register_ ## name(void); \
  void (*fp_register_ ## name)(void) = _register_ ## name
// Electron's builtin modules.
//REFERENCE_MODULE(atom_browser_app);
#undef REFERENCE_MODULE

// The "v8::Function::kLineOffsetNotFound" is exported in node.dll, but the
// linker can not find it, could be a bug of VS.
#if defined(OS_WIN) && !defined(DEBUG)
namespace v8 {
const int Function::kLineOffsetNotFound = -1;
}
#endif

namespace atom {

namespace {

// Empty callback for async handle.
void UvNoOp(uv_async_t* handle,int status) {
}

// Convert the given vector to an array of C-strings. The strings in the
// returned vector are only guaranteed valid so long as the vector of strings
// is not modified.
/*
scoped_ptr<const char*[]> StringVectorToArgArray(
    const std::vector<std::string>& vector) {
  scoped_ptr<const char*[]> array(new const char*[vector.size()]);
  for (size_t i = 0; i < vector.size(); ++i) {
    array[i] = vector[i].c_str();
  }
  return array.Pass();
}
*/

}  // namespace

node::Environment* global_env = nullptr;

NodeBindings::NodeBindings(bool is_browser)
    : is_browser_(is_browser),
      uv_loop_(uv_default_loop()),
      embed_closed_(false),
      uv_env_(nullptr)
{
}

NodeBindings::~NodeBindings() {
  // Quit the embed thread.
  embed_closed_ = true;
  uv_sem_post(&embed_sem_);
  WakeupEmbedThread();

  // Wait for everything to be done.
  uv_thread_join(&embed_thread_);

  // Clear uv.
  uv_sem_destroy(&embed_sem_);
}

void NodeBindings::Initialize() {
  // Open node's error reporting system for browser process.
  //
#if defined(OS_LINUX)
  // Get real command line in renderer process forked by zygote.
#endif

  // Init node.
  // (we assume node::Init would not modify the parameters under embedded mode).
}

node::Environment* NodeBindings::CreateEnvironment(
    v8::Handle<v8::Context> context) {
/*
  scoped_ptr<const char*[]> c_argv = StringVectorToArgArray(args);
  node::Environment* env = node::CreateEnvironment(
      context->GetIsolate(), uv_default_loop(), context,
      args.size(), c_argv.get(), 0, nullptr);

  return env;
  */
    return 0;
}

void NodeBindings::LoadEnvironment(node::Environment* env) {
}

void NodeBindings::PrepareMessageLoop() {

  // Add dummy handle for libuv, otherwise libuv would quit when there is
  // nothing to do.
  uv_async_init(uv_loop_, &dummy_uv_handle_, UvNoOp);

  // Start worker that will interrupt main loop when having uv events.
  uv_sem_init(&embed_sem_, 0);
  uv_thread_create(&embed_thread_, EmbedThreadRunner, this);
}

void NodeBindings::RunMessageLoop() {

  // The MessageLoop should have been created, remember the one in main thread.
  //message_loop_ = base::MessageLoop::current();

  // Run uv loop for once to give the uv__io_poll a chance to add all events.
  UvRunOnce();
}

void NodeBindings::UvRunOnce() {

  // By default the global env would be used unless user specified another one
  // (this happens for renderer process, which wraps the uv loop with web page
  // context).
  //node::Environment* env = uv_env() ? uv_env() : global_env;

  // Use Locker in browser process.
  //mate::Locker locker(env->isolate());
  //v8::HandleScope handle_scope(env->isolate());

  // Enter node context while dealing with uv events.
  //v8::Context::Scope context_scope(env->context());

  // Perform microtask checkpoint after running JavaScript.
  //scoped_ptr<blink::WebScopedRunV8Script> script_scope(
  //    is_browser_ ? nullptr : new blink::WebScopedRunV8Script(env->isolate()));

  // Deal with uv events.
  int r = uv_run(uv_loop_, UV_RUN_NOWAIT);
//  if (r == 0 || uv_loop_->stop_flag != 0)
//    message_loop_->QuitWhenIdle();  // Quit from uv.

  // Tell the worker thread to continue polling.
  uv_sem_post(&embed_sem_);
}

void NodeBindings::WakeupMainThread() {
  //message_loop_->PostTask(FROM_HERE, base::Bind(&NodeBindings::UvRunOnce,
  //                                              weak_factory_.GetWeakPtr()));
  //RefPtr<NodeBindings> protector(this);
  RunLoop::main().dispatch([this] {
    this->UvRunOnce();
  });
}

void NodeBindings::WakeupEmbedThread() {
  uv_async_send(&dummy_uv_handle_);
}

// static
void NodeBindings::EmbedThreadRunner(void *arg) {
  NodeBindings* self = static_cast<NodeBindings*>(arg);

  while (true) {
    // Wait for the main loop to deal with events.

    printf("NodeBindings::EmbedThreadRunner in,sem waiting mainthread post...\n");
    uv_sem_wait(&self->embed_sem_);
    if (self->embed_closed_)
      break;
    // Wait for something to happen in uv loop.
    // Note that the PollEvents() is implemented by derived classes, so when
    // this class is being destructed the PollEvents() would not be available
    // anymore. Because of it we must make sure we only invoke PollEvents()
    // when this class is alive.
    printf("NodeBindings::EmbedThreadRunner event polling...\n");
    self->PollEvents();
    if (self->embed_closed_)
      break;

    // Deal with event in main thread.
    self->WakeupMainThread();
  }
}

}  // namespace atom

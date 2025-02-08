
#include "epi.h"
#include "sokol_local.h"
#include "thread.h"

constexpr uint32_t kMaxRenderQueue = 256;

struct RenderContext
{
};

struct RenderThread
{
    thread_ptr_t    thread_;
    thread_signal_t signal_start_;

    thread_queue_t      queue_;
    sgl_context        *render_queue_[kMaxRenderQueue];
    thread_atomic_int_t exit_flag_;
};

static struct RenderThread render_thread;

static int32_t RenderProc(void *thread_data)
{
    EPI_UNUSED(thread_data);

    while (thread_atomic_int_load(&render_thread.exit_flag_) == 0)
    {        
        /*
        if (thread_signal_wait(&render_thread.signal_start_, THREAD_SIGNAL_WAIT_INFINITE))
        {
            if (thread_atomic_int_load(&render_thread.exit_flag_))
            {
                break;
            }
        }
            */

        sgl_context *context = (sgl_context *)thread_queue_consume(&render_thread.queue_, 0);
        if (context)
        {
            EDGE_ZoneNamedN(ZoneRenderContext, "RenderContext", true);
            sgl_context_draw(*context);
            delete context;
        }
    }

    return 0;
}

void RenderQueueDrawContext(sgl_context context)
{
    sgl_context* c = new sgl_context;
    c->id = context.id;

    thread_queue_produce(&render_thread.queue_, c, 100);
}

void RenderStartThread()
{
    thread_atomic_int_store(&render_thread.exit_flag_, 0);
    thread_signal_init(&render_thread.signal_start_);
    thread_queue_init(&render_thread.queue_, kMaxRenderQueue, (void **)render_thread.render_queue_, 0);
    render_thread.thread_ = thread_create(RenderProc, nullptr, THREAD_STACK_SIZE_DEFAULT);
}
void RenderStopThread()
{
    thread_atomic_int_store(&render_thread.exit_flag_, 1);
    thread_signal_raise(&render_thread.signal_start_);
    thread_join(render_thread.thread_);
    thread_signal_term(&render_thread.signal_start_);
}

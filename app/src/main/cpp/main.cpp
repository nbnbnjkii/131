#include <jni.h>
#include "AndroidOut.h"
#include "Renderer.h"
#include <game-activity/GameActivity.h>

extern "C" {

static bool active = false;
void handle_cmd(android_app *app, int32_t command) {
    auto *renderer = static_cast<Renderer *>(app->userData);
    switch (command) {
        case APP_CMD_INIT_WINDOW:
            delete renderer;
            app->userData = app->window ? new Renderer(app) : nullptr;
            break;
        case APP_CMD_TERM_WINDOW:
            delete renderer;
            app->userData = nullptr;
            break;
        case APP_CMD_GAINED_FOCUS:
            active = true;
            if (renderer) renderer->resetInput();
            break;
        case APP_CMD_LOST_FOCUS:
        case APP_CMD_PAUSE:
            active = false;
            if (renderer) renderer->resetInput();
            break;
        default: break;
    }
}

bool motion_event_filter_func(const GameActivityMotionEvent *event) {
    return (event->source & AINPUT_SOURCE_CLASS_MASK) == AINPUT_SOURCE_CLASS_POINTER;
}

void android_main(android_app *app) {
    app->onAppCmd = handle_cmd;
    android_app_set_motion_event_filter(app, motion_event_filter_func);
    while (!app->destroyRequested) {
        int events;
        android_poll_source *source = nullptr;
        const int result = ALooper_pollOnce(active && app->userData ? 0 : -1, nullptr,
                                            &events, reinterpret_cast<void **>(&source));
        if (result == ALOOPER_POLL_ERROR) break;
        if (source) source->process(app, source);
        if (app->destroyRequested) break;
        if (active && app->userData) {
            auto *renderer = static_cast<Renderer *>(app->userData);
            renderer->handleInput();
            renderer->render();
        }
    }
    delete static_cast<Renderer *>(app->userData);
    app->userData = nullptr;
}
}

#include "hotkey.h"
#include <QSet>
#include <QSocketNotifier>
#include <xcb/xcb.h>
#include <cstdlib>

struct Hotkey::Binding {
    xcb_connection_t *connection;
    std::unique_ptr<QSocketNotifier> notifier;
    ~Binding() {
        notifier.reset();
        xcb_disconnect(connection);
    }
};

Hotkey::~Hotkey() { delete binding; }

bool Hotkey::bind(const QString &choice)
{
    if (choice == "off") {
        delete binding;
        binding = nullptr;
        return true;
    }
    const uint32_t symbol =
        choice == "Ctrl+Alt+A" ? 'a' : choice == "Ctrl+Alt+S" ? 's' :
        choice == "Ctrl+Alt+F8" ? 0xffc5 : choice == "F1" ? 0xffbe : 0;
    if (!symbol || qEnvironmentVariable("XDG_SESSION_TYPE") != "x11") return false;
    const bool functionKey = choice == "F1";
    const uint16_t modifiers = functionKey ? 0 : (XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_1);
    int screen = 0;
    std::unique_ptr<Binding> next(new Binding{xcb_connect(nullptr, &screen), {}});
    auto *c = next->connection;
    if (xcb_connection_has_error(c)) return false;
    const auto *setup = xcb_get_setup(c);
    auto roots = xcb_setup_roots_iterator(setup);
    for (int i=0; i<screen && roots.rem; ++i) xcb_screen_next(&roots);
    if (!roots.rem) return false;
    const auto first = setup->min_keycode;
    auto *map = xcb_get_keyboard_mapping_reply(c, xcb_get_keyboard_mapping(c, first, setup->max_keycode-first+1), nullptr);
    auto *mods = xcb_get_modifier_mapping_reply(c, xcb_get_modifier_mapping(c), nullptr);
    if (!map || !mods) { free(map); free(mods); return false; }
    auto *keys = xcb_get_keyboard_mapping_keysyms(map);
    auto *modifierKeys = xcb_get_modifier_mapping_keycodes(mods);
    int target = 0, numLock = 0;
    for (int code=first; code<=setup->max_keycode; ++code) {
        const auto key = keys[(code-first)*map->keysyms_per_keycode];
        if (key == symbol && !target) target = code;
        if (key == 0xff7f)
            for (int i=0; i<8*mods->keycodes_per_modifier; ++i)
                if (modifierKeys[i] == code) numLock |= 1 << (i/mods->keycodes_per_modifier);
    }
    free(map); free(mods);
    if (!target) return false;
    const QSet<int> locks{0, XCB_MOD_MASK_LOCK, numLock, numLock | XCB_MOD_MASK_LOCK};
    for (int lock : locks) {
        auto cookie = xcb_grab_key_checked(c, false, roots.data->root, modifiers | lock,
                                           target, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        if (auto *error = xcb_request_check(c, cookie)) { free(error); return false; }
    }
    next->notifier = std::make_unique<QSocketNotifier>(xcb_get_file_descriptor(c), QSocketNotifier::Read);
    connect(next->notifier.get(), &QSocketNotifier::activated, this, [this,c,target]{
        bool pressed = false;
        while (auto *event = xcb_poll_for_event(c)) {
            if ((event->response_type & 127) == XCB_KEY_PRESS &&
                reinterpret_cast<xcb_key_press_event_t *>(event)->detail == target) pressed = true;
            free(event);
        }
        if (pressed) emit triggered();
    });
    delete binding;
    binding = next.release();
    return true;
}

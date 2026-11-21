#include "windowframes.h"
#include <QGuiApplication>
#include <QSet>
#include <QScreen>
#include <QPixmap>
#include <xcb/xcb.h>
#include <cstdlib>
#include <memory>

namespace {
template<typename T> auto owned(T *p)
{
    return std::unique_ptr<T, decltype(&std::free)>(p, &std::free);
}
}

static QList<WindowFrame> windowFrames(xcb_connection_t *c)
{
    auto root = xcb_setup_roots_iterator(xcb_get_setup(c)).data->root;
    auto atom = owned(xcb_intern_atom_reply(c, xcb_intern_atom(c, 1, 16, "_NET_CLIENT_LIST"), nullptr));
    if (!atom || !atom->atom) return {};
    auto clients = owned(xcb_get_property_reply(c, xcb_get_property(c, 0, root, atom->atom,
                              XCB_ATOM_WINDOW, 0, 65536), nullptr));
    if (!clients || clients->format != 32) return {};
    const auto ids = static_cast<const xcb_window_t *>(xcb_get_property_value(clients.get()));
    QSet<xcb_window_t> frames;
    for (unsigned i = 0; i < clients->value_len; ++i) {
        auto id = ids[i];
        while (id && id != root) {
            auto tree = owned(xcb_query_tree_reply(c, xcb_query_tree(c, id), nullptr));
            if (!tree) break;
            if (tree->parent == root) { frames.insert(id); break; }
            id = tree->parent;
        }
    }
    auto tree = owned(xcb_query_tree_reply(c, xcb_query_tree(c, root), nullptr));
    if (!tree) return {};
    auto children = xcb_query_tree_children(tree.get());
    QList<WindowFrame> result;
    for (int i = tree->children_len - 1; i >= 0; --i) {
        auto id = children[i];
        auto attr = owned(xcb_get_window_attributes_reply(c, xcb_get_window_attributes(c, id), nullptr));
        if (!attr || attr->map_state != XCB_MAP_STATE_VIEWABLE || attr->_class != XCB_WINDOW_CLASS_INPUT_OUTPUT)
            continue;
        auto g = owned(xcb_get_geometry_reply(c, xcb_get_geometry(c, id), nullptr));
        if (g)
            result.append({QRect(g->x, g->y, g->width + 2*g->border_width, g->height + 2*g->border_width),
                           frames.contains(id)});
    }
    return result;
}

QList<WindowFrame> captureWindowFrames(QScreen *screen, QImage &image)
{
    auto *native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!native) return {};
    auto *connection = native->connection();
    // Capture geometry and pixels on Qt's X connection before other clients can move.
    xcb_grab_server(connection);
    const auto frames = windowFrames(connection);
    const QPixmap snapshot = screen->grabWindow(0);
    xcb_ungrab_server(connection);
    xcb_flush(connection);
    image = snapshot.toImage();
    return frames;
}

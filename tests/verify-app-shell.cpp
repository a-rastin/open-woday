#include <QColor>
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QRect>
#include <QSize>
#include <QTimer>

#include <cstdio>

namespace {

int failures = 0;

void check(bool condition, const char *message)
{
    if (condition) {
        std::printf("PASS: %s\n", message);
        return;
    }

    std::fprintf(stderr, "FAIL: %s\n", message);
    ++failures;
}

bool isColor(const QImage &image, int x, int y, const QColor &expected)
{
    return image.pixelColor(x, y) == expected;
}

// Find the full-colour extent of the centre surface on a scanline. Rounded-edge
// antialiasing is intentionally ignored; the first and last solid pixels are the
// surface's visible bounds at that position.
bool findPanelExtent(const QImage &image, int scanline, bool horizontal,
                     int &start, int &end)
{
    const QColor panel(QStringLiteral("#FFFFFF"));
    const int length = horizontal ? image.width() : image.height();
    start = -1;
    end = -1;

    for (int position = 0; position < length; ++position) {
        const int x = horizontal ? position : scanline;
        const int y = horizontal ? scanline : position;
        if (!isColor(image, x, y, panel))
            continue;

        if (start < 0)
            start = position;
        end = position;
    }

    return start >= 0;
}

bool hasUniformMargin(const QImage &image, int start, int end, int scanline,
                      bool horizontal, const QColor &expected)
{
    for (int position = start; position <= end; ++position) {
        const int x = horizontal ? position : scanline;
        const int y = horizontal ? scanline : position;
        if (!isColor(image, x, y, expected))
            return false;
    }
    return true;
}

bool isCleanPanel(const QImage &image, const QRect &bounds)
{
    const QColor panel(QStringLiteral("#FFFFFF"));
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            if (!isColor(image, x, y, panel))
                return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    // Render deterministically without touching the user's desktop or database.
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_SCALE_FACTOR", "1");
    qputenv("QT_QUICK_BACKEND", "software");

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/MainWindow.qml")));

    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr,
                     "FAIL: the root QML component did not create a window\n");
        return 1;
    }

    check(engine.rootObjects().size() == 1,
          "launching creates exactly one window");

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (window == nullptr) {
        std::fprintf(stderr, "FAIL: the root object is not a window\n");
        return 1;
    }

    QCoreApplication::processEvents();

    check(window->title() == QStringLiteral("open-woday"),
          "the native window keeps the open-woday title");
    check(window->isVisible(), "the window is visible");
    check(window->size() == QSize(800, 600),
          "the normal launch size remains 800 by 600");

    QEventLoop frameLoop;
    bool frameSwapped = false;
    const auto frameConnection = QObject::connect(
        window, &QQuickWindow::frameSwapped, &frameLoop,
        [&] {
            frameSwapped = true;
            frameLoop.quit();
        });
    QTimer::singleShot(1000, &frameLoop, &QEventLoop::quit);
    window->update();
    frameLoop.exec();
    QObject::disconnect(frameConnection);
    QCoreApplication::processEvents();

    check(frameSwapped, "the launched window completes a rendered frame");

    const QImage image = window->grabWindow();
    check(!image.isNull(), "the launched window can be rendered");
    if (image.isNull())
        return 1;

    check(image.size() == QSize(800, 600),
          "the rendered window matches the normal launch size");

    const QColor canvas(QStringLiteral("#1C1C1E"));
    const QColor sidebar(QStringLiteral("#17181C"));
    constexpr int gap = 12;
    constexpr int radius = 18;

    int left = -1;
    int right = -1;
    int top = -1;
    int bottom = -1;

    check(findPanelExtent(image, image.height() / 2, true, left, right),
          "a centre panel is visible beside the Sidebar");
    check(findPanelExtent(image, image.width() / 2, false, top, bottom),
          "a centre panel is visible beneath the TopBar");

    if (left < 0 || right < 0 || top < 0 || bottom < 0)
        return 1;

    check(isColor(image, 10, image.height() / 2, sidebar),
          "the dark Sidebar remains on the left");
    check(isColor(image, image.width() / 2, 10, sidebar),
          "the TopBar continues to span the frame");

    check(left >= gap && right + gap < image.width()
              && top >= gap && bottom + gap < image.height(),
          "the centre panel is inset from the window edges");
    if (left < gap || right + gap >= image.width()
        || top < gap || bottom + gap >= image.height()) {
        return 1;
    }

    check(hasUniformMargin(image, left - gap, left - 1,
                           image.height() / 2, true, canvas),
          "the shared gap separates the Sidebar from the centre panel");
    check(hasUniformMargin(image, right + 1, right + gap,
                           image.height() / 2, true, canvas),
          "the shared gap separates the centre panel from the right edge");
    check(hasUniformMargin(image, top - gap, top - 1,
                           image.width() / 2, false, canvas),
          "the shared gap separates the TopBar from the centre panel");
    check(hasUniformMargin(image, bottom + 1, bottom + gap,
                           image.width() / 2, false, canvas),
          "the shared gap separates the centre panel from the bottom edge");

    check(right + gap == image.width() - 1,
          "the centre panel reaches the available right edge");
    check(bottom + gap == image.height() - 1,
          "the centre panel reaches the available bottom edge");

    check(!isColor(image, left, top, QColor(Qt::white)),
          "the centre panel has a rounded top-left corner");
    check(isColor(image, left + radius, top + radius, QColor(Qt::white)),
          "the rounded corner opens onto the white panel");

    const QRect cleanBounds(left + radius + 1, top + radius + 1,
                            right - left - 2 * radius - 1,
                            bottom - top - 2 * radius - 1);
    check(isCleanPanel(image, cleanBounds),
          "the centre panel contains no page content or placeholder");

    if (failures != 0) {
        std::fprintf(stderr, "%d app-shell appearance check(s) failed\n",
                     failures);
        return 1;
    }

    std::printf("All app-shell appearance checks passed\n");
    return 0;
}

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
#include <vector>

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

struct PanelRun {
    int start;
    int end;
};

// Find the full-colour panel runs on a scanline. Rounded-edge antialiasing is
// intentionally ignored; the first and last solid pixels are each surface's
// visible bounds at that position.
std::vector<PanelRun> findPanelRuns(const QImage &image, int scanline,
                                   bool horizontal)
{
    const QColor panel(QStringLiteral("#FFFFFF"));
    const int length = horizontal ? image.width() : image.height();
    std::vector<PanelRun> runs;
    bool inPanel = false;

    for (int position = 0; position < length; ++position) {
        const int x = horizontal ? position : scanline;
        const int y = horizontal ? scanline : position;
        if (isColor(image, x, y, panel)) {
            if (!inPanel) {
                runs.push_back({position, position});
                inPanel = true;
            } else {
                runs.back().end = position;
            }
        } else {
            inPanel = false;
        }
    }

    return runs;
}

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

bool hasRoundedCorners(const QImage &image, const QRect &bounds, int radius)
{
    const QColor panel(Qt::white);
    const int left = bounds.left();
    const int top = bounds.top();
    const int right = bounds.right();
    const int bottom = bounds.bottom();

    return !isColor(image, left, top, panel)
        && !isColor(image, right, top, panel)
        && !isColor(image, left, bottom, panel)
        && !isColor(image, right, bottom, panel)
        && isColor(image, left + radius, top + radius, panel)
        && isColor(image, right - radius, top + radius, panel)
        && isColor(image, left + radius, bottom - radius, panel)
        && isColor(image, right - radius, bottom - radius, panel);
}

QImage grabWindowAtSize(QQuickWindow *window, const QSize &size,
                        bool &frameSwapped)
{
    window->resize(size);
    QCoreApplication::processEvents();

    QEventLoop frameLoop;
    frameSwapped = false;
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

    return window->grabWindow();
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

    const std::vector<PanelRun> panelRuns =
        findPanelRuns(image, image.height() / 2, true);
    check(panelRuns.size() == 2,
          "a white RightRail is visible to the right of the centre panel");
    if (panelRuns.size() != 2)
        return 1;

    const PanelRun centre = panelRuns.at(0);
    const PanelRun rightRail = panelRuns.at(1);
    const int left = centre.start;
    const int right = centre.end;
    int top = -1;
    int bottom = -1;

    check(findPanelExtent(image, image.width() / 2, false, top, bottom),
          "a centre panel is visible beneath the TopBar");
    if (top < 0 || bottom < 0)
        return 1;

    check(isColor(image, 10, image.height() / 2, sidebar),
          "the dark Sidebar remains on the left");
    check(isColor(image, image.width() / 2, 10, sidebar),
          "the TopBar continues to span the frame");

    check(left >= gap && rightRail.end + gap < image.width()
              && top >= gap && bottom + gap < image.height(),
          "both panels are inset from the window edges");
    if (left < gap || rightRail.end + gap >= image.width()
        || top < gap || bottom + gap >= image.height()) {
        return 1;
    }

    check(hasUniformMargin(image, left - gap, left - 1,
                           image.height() / 2, true, canvas),
          "the shared gap separates the Sidebar from the centre panel");
    check(hasUniformMargin(image, right + 1, rightRail.start - 1,
                           image.height() / 2, true, canvas),
          "the shared gap separates the centre panel from the RightRail");
    check(hasUniformMargin(image, rightRail.end + 1, rightRail.end + gap,
                           image.height() / 2, true, canvas),
          "the shared gap separates the RightRail from the right edge");
    check(hasUniformMargin(image, top - gap, top - 1,
                           image.width() / 2, false, canvas),
          "the shared gap separates the TopBar from the centre panel");
    check(hasUniformMargin(image, bottom + 1, bottom + gap,
                           image.width() / 2, false, canvas),
          "the shared gap separates the centre panel from the bottom edge");

    check(rightRail.start - right - 1 == gap,
          "the centre panel and RightRail use the shared panel gap");
    check(rightRail.end + gap == image.width() - 1,
          "the RightRail reaches the available right edge");
    check(right - left + 1 > rightRail.end - rightRail.start + 1,
          "the centre panel remains wider than the RightRail");
    check(bottom + gap == image.height() - 1,
          "the centre panel reaches the available bottom edge");

    const int railCenter = rightRail.start
        + (rightRail.end - rightRail.start) / 2;
    int railTop = -1;
    int railBottom = -1;
    check(findPanelExtent(image, railCenter, false, railTop, railBottom),
          "the RightRail is visible beneath the TopBar");
    if (railTop < 0 || railBottom < 0)
        return 1;

    check(hasUniformMargin(image, railTop - gap, railTop - 1,
                           railCenter, false, canvas),
          "the shared gap separates the TopBar from the RightRail");
    check(hasUniformMargin(image, railBottom + 1, railBottom + gap,
                           railCenter, false, canvas),
          "the shared gap separates the RightRail from the bottom edge");
    check(railBottom + gap == image.height() - 1,
          "the RightRail reaches the available bottom edge");

    check(hasRoundedCorners(image,
                            QRect(left, top, right - left + 1,
                                  bottom - top + 1),
                            radius),
          "the centre panel has rounded corners");
    check(hasRoundedCorners(image,
                            QRect(rightRail.start, railTop,
                                  rightRail.end - rightRail.start + 1,
                                  railBottom - railTop + 1),
                            radius),
          "the RightRail has rounded corners");

    const QRect cleanBounds(left + radius + 1, top + radius + 1,
                            right - left - 2 * radius - 1,
                            bottom - top - 2 * radius - 1);
    check(isCleanPanel(image, cleanBounds),
          "the centre panel contains no page content or placeholder");

    const QRect cleanRailBounds(rightRail.start + radius + 1,
                                railTop + radius + 1,
                                rightRail.end - rightRail.start - 2 * radius - 1,
                                railBottom - railTop - 2 * radius - 1);
    check(isCleanPanel(image, cleanRailBounds),
          "the RightRail contains no Item content or placeholder");

    bool sidebarFoldFrameSwapped = false;
    const QImage sidebarFoldImage = grabWindowAtSize(
        window, QSize(700, 600), sidebarFoldFrameSwapped);
    check(sidebarFoldFrameSwapped,
           "narrowing the window completes another rendered frame");
    check(sidebarFoldImage.size() == QSize(700, 600),
           "the window renders at the Sidebar-fold size");

    const std::vector<PanelRun> sidebarFoldRuns =
        findPanelRuns(sidebarFoldImage, sidebarFoldImage.height() / 2, true);
    check(sidebarFoldRuns.size() == 2,
           "the RightRail stays visible when the Sidebar folds");
    check(isColor(sidebarFoldImage, 10, sidebarFoldImage.height() / 2,
                  canvas),
           "the Sidebar folds before the RightRail");
    check(isColor(sidebarFoldImage, 10, 10, sidebar)
              && isColor(sidebarFoldImage, sidebarFoldImage.width() - 11,
                         10, sidebar),
           "the TopBar spans the frame after the Sidebar folds");
    if (sidebarFoldRuns.size() == 2) {
        const PanelRun foldedCentre = sidebarFoldRuns.at(0);
        const PanelRun foldedRightRail = sidebarFoldRuns.at(1);
        check(foldedCentre.start == gap,
               "the centre panel reclaims the folded Sidebar space");
        check(foldedRightRail.end + gap == sidebarFoldImage.width() - 1,
               "the RightRail keeps its shared inset after the Sidebar folds");
        check(foldedCentre.end - foldedCentre.start + 1
                  > foldedRightRail.end - foldedRightRail.start + 1,
               "the centre panel remains wider than the RightRail");
    }

    bool widenedFrameSwapped = false;
    const QImage widenedImage = grabWindowAtSize(
        window, QSize(800, 600), widenedFrameSwapped);
    check(widenedFrameSwapped,
           "widening the window completes another rendered frame");
    check(widenedImage.size() == QSize(800, 600),
           "the window renders again at the normal launch size");
    const std::vector<PanelRun> widenedRuns =
        findPanelRuns(widenedImage, widenedImage.height() / 2, true);
    check(widenedRuns.size() == 2,
           "widening restores the RightRail alongside the centre panel");
    check(isColor(widenedImage, 10, widenedImage.height() / 2, sidebar),
           "widening restores the Sidebar after it folded first");
    check(isColor(widenedImage, 10, 10, sidebar)
              && isColor(widenedImage, widenedImage.width() - 11, 10,
                         sidebar),
           "the TopBar still spans the restored frame");
    if (widenedRuns.size() == 2) {
        check(widenedRuns.at(0).start > gap
                  && widenedRuns.at(1).end + gap == widenedImage.width() - 1,
               "the restored panels return to their original inset");
    }

    bool rightRailFoldFrameSwapped = false;
    const QImage rightRailFoldImage = grabWindowAtSize(
        window, QSize(520, 600), rightRailFoldFrameSwapped);
    check(rightRailFoldFrameSwapped,
           "narrowing past the RightRail threshold renders another frame");
    check(rightRailFoldImage.size() == QSize(520, 600),
           "the window renders at the RightRail-fold size");

    const std::vector<PanelRun> rightRailFoldRuns =
        findPanelRuns(rightRailFoldImage, rightRailFoldImage.height() / 2,
                      true);
    check(rightRailFoldRuns.size() == 1,
           "the RightRail folds after the Sidebar");
    check(isColor(rightRailFoldImage, 10, rightRailFoldImage.height() / 2,
                  canvas)
              && isColor(rightRailFoldImage,
                         rightRailFoldImage.width() - 11,
                         rightRailFoldImage.height() / 2, canvas),
           "both side panels are absent at the narrowest stage");
    check(isColor(rightRailFoldImage, 10, 10, sidebar)
              && isColor(rightRailFoldImage,
                         rightRailFoldImage.width() - 11, 10, sidebar),
           "the TopBar spans the frame after the RightRail folds");
    if (rightRailFoldRuns.size() == 1) {
        const PanelRun narrowCentre = rightRailFoldRuns.at(0);
        check(narrowCentre.start == gap
                  && narrowCentre.end + gap == rightRailFoldImage.width() - 1,
               "the centre panel reclaims both folded-panel spaces");
        check(narrowCentre.end - narrowCentre.start + 1
                  >= rightRailFoldImage.width() - 2 * gap,
               "the centre panel keeps the shared inset without overlap");
    }

    bool rightRailRestoredFrameSwapped = false;
    const QImage rightRailRestoredImage = grabWindowAtSize(
        window, QSize(700, 600), rightRailRestoredFrameSwapped);
    check(rightRailRestoredFrameSwapped,
           "widening to the Sidebar-fold stage renders another frame");
    const std::vector<PanelRun> rightRailRestoredRuns =
        findPanelRuns(rightRailRestoredImage,
                      rightRailRestoredImage.height() / 2, true);
    check(rightRailRestoredRuns.size() == 2,
           "widening restores the RightRail before the Sidebar");
    check(isColor(rightRailRestoredImage, 10,
                  rightRailRestoredImage.height() / 2, canvas),
           "the Sidebar remains folded while the RightRail returns");
    check(isColor(rightRailRestoredImage, 10, 10, sidebar)
              && isColor(rightRailRestoredImage,
                         rightRailRestoredImage.width() - 11, 10, sidebar),
           "the TopBar spans the frame as the RightRail returns");

    // Issue 06 — the Phase 3 merge gate at the single build-and-launch seam.
    // After the whole resize cycle the window is widened back to the normal
    // launch size: the complete three-panel frame must return in the shared
    // visual language, and closing the launched window must exit cleanly. No
    // new harness, framework or tooling layer is introduced for this check.
    bool mergeGateFrameSwapped = false;
    const QImage mergeGateImage = grabWindowAtSize(
        window, QSize(800, 600), mergeGateFrameSwapped);
    check(mergeGateFrameSwapped,
          "the merge gate completes a rendered frame at the normal launch size");
    check(mergeGateImage.size() == QSize(800, 600),
          "the merge gate ends at the normal launch size");

    const std::vector<PanelRun> mergeGateRuns =
        findPanelRuns(mergeGateImage, mergeGateImage.height() / 2, true);
    check(mergeGateRuns.size() == 2,
          "the merge gate shows the centre panel and the RightRail");
    check(isColor(mergeGateImage, 10, mergeGateImage.height() / 2, sidebar),
          "the merge gate shows the dark Sidebar");
    check(isColor(mergeGateImage, 10, 10, sidebar)
              && isColor(mergeGateImage, mergeGateImage.width() - 11, 10,
                         sidebar),
          "the TopBar spans the frame at the merge gate");

    if (mergeGateRuns.size() == 2) {
        const PanelRun gateCentre = mergeGateRuns.at(0);
        const PanelRun gateRail = mergeGateRuns.at(1);

        check(gateCentre.start >= gap
                  && gateRail.end + gap == mergeGateImage.width() - 1,
              "both panels keep the shared inset at the merge gate");
        check(gateRail.start - gateCentre.end - 1 == gap,
              "the panels keep the shared panel gap at the merge gate");
        check(hasUniformMargin(mergeGateImage, gateCentre.start - gap,
                               gateCentre.start - 1,
                               mergeGateImage.height() / 2, true, canvas),
              "the dark canvas separates the Sidebar from the centre panel at "
              "the merge gate");

        int gateTop = -1;
        int gateBottom = -1;
        const bool gateExtent = findPanelExtent(
            mergeGateImage, mergeGateImage.width() / 2, false, gateTop,
            gateBottom);
        check(gateExtent,
              "the centre panel is visible beneath the TopBar at the merge gate");
        if (gateExtent) {
            check(hasRoundedCorners(
                      mergeGateImage,
                      QRect(gateCentre.start, gateTop,
                            gateCentre.end - gateCentre.start + 1,
                            gateBottom - gateTop + 1),
                      radius),
                  "the centre panel keeps the shared rounded corners at the "
                  "merge gate");
            check(isCleanPanel(
                      mergeGateImage,
                      QRect(gateCentre.start + radius + 1, gateTop + radius + 1,
                            gateCentre.end - gateCentre.start - 2 * radius - 1,
                            gateBottom - gateTop - 2 * radius - 1)),
                  "the merge gate renders no live Item data or placeholder");
        }
    }

    window->close();
    QCoreApplication::processEvents();
    check(!window->isVisible(), "closing the launched window exits cleanly");

    if (failures != 0) {
        std::fprintf(stderr, "%d app-shell appearance check(s) failed\n",
                     failures);
        return 1;
    }

    std::printf("verify-app-shell: PASS - the Phase 3 merge gate holds: one "
                "natively decorated window shows the dark Sidebar, spanning "
                "TopBar, white rounded centre panel and white rounded "
                "RightRail on the dark canvas; resizing folds the Sidebar "
                "first and the RightRail second, widening restores the frame, "
                "and closing the window exits cleanly\n");
    return 0;
}

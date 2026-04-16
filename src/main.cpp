#include <QApplication>
#include <QCursor>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>
#include "MainWindow.h"

namespace {

class SoftwareCursorOverlay : public QWidget {
public:
    explicit SoftwareCursorOverlay(QWidget* parent = nullptr)
        : QWidget(parent) {
        setFixedSize(20, 20);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        hide();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.setPen(QPen(QColor(20, 20, 20), 3));
        painter.setBrush(QColor(255, 235, 59));
        painter.drawEllipse(QRectF(2, 2, 16, 16));

        painter.setPen(QPen(QColor(20, 20, 20), 2));
        painter.drawLine(10, 0, 10, 20);
        painter.drawLine(0, 10, 20, 10);
    }
};

QCursor createVisibleArrowCursor() {
    QPixmap pixmap(28, 28);
    // Keep the cursor fully opaque to avoid transparency issues on some X11/WSL setups.
    pixmap.fill(QColor(255, 235, 59));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(QColor(20, 20, 20), 3));
    painter.drawLine(2, 2, 25, 25);
    painter.drawLine(25, 2, 2, 25);
    painter.drawRect(1, 1, 25, 25);

    return QCursor(pixmap, 1, 1);
}

} // namespace

int main(int argc, char *argv[]) {
    // WSL/mesa environments can fail EGL/ZINK probing and crash on window creation.
    // Force a stable software stack before QApplication is constructed.
    qputenv("QT_QPA_PLATFORM", "xcb");
    QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    qputenv("QT_OPENGL", "software");
    qputenv("LIBGL_ALWAYS_SOFTWARE", "1");
    qputenv("MESA_LOADER_DRIVER_OVERRIDE", "llvmpipe");
    qputenv("GALLIUM_DRIVER", "llvmpipe");
    qputenv("QT_XCB_FORCE_SOFTWARE_OPENGL", "1");

    QApplication app(argc, argv);
    const QCursor visibleCursor = createVisibleArrowCursor();
    QApplication::setOverrideCursor(visibleCursor);

    MainWindow window;
    window.show();

    auto* softwareCursor = new SoftwareCursorOverlay(&window);

    QTimer cursorTimer;
    QObject::connect(&cursorTimer, &QTimer::timeout, [&window, softwareCursor]() {
        const QPoint globalPos = QCursor::pos();
        const QPoint localPos = window.mapFromGlobal(globalPos);

        if (window.rect().contains(localPos)) {
            softwareCursor->move(localPos - QPoint(2, 2));
            softwareCursor->show();
            softwareCursor->raise();
        } else {
            softwareCursor->hide();
        }
    });
    cursorTimer.start(16);

    return app.exec();
}

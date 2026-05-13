#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QDebug>
#include <QScreen>
#include <QStyleFactory>
#include <QStringList>
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

QString resolveAssetPath(const QString& relativePath) {
    if (relativePath.isEmpty()) {
        return QString();
    }

    const QFileInfo directInfo(relativePath);
    if (directInfo.isAbsolute() && directInfo.exists()) {
        return directInfo.absoluteFilePath();
    }

    const QStringList candidates = {
        QDir::current().filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../") + relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../") + relativePath)
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }

    return QString();
}

} // namespace

int main(int argc, char *argv[]) {
    // Windows often reports a fractional logical desktop at 125%/150% scaling.
    // The UI was tuned in WSL at 1:1 logical pixels, so normalize only native
    // Windows builds to avoid text and fixed panels crowding each other.
#if defined(Q_OS_WIN)
    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "RoundPreferFloor");
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#endif

    // WSL/mesa environments can fail EGL/ZINK probing and crash on window creation.
    // Force a stable software stack before QApplication is constructed.
#if defined(Q_OS_LINUX)
    qputenv("QT_QPA_PLATFORM", "xcb");
    QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    qputenv("QT_OPENGL", "software");
    qputenv("LIBGL_ALWAYS_SOFTWARE", "1");
    qputenv("MESA_LOADER_DRIVER_OVERRIDE", "llvmpipe");
    qputenv("GALLIUM_DRIVER", "llvmpipe");
    qputenv("QT_XCB_FORCE_SOFTWARE_OPENGL", "1");
#endif

    QApplication app(argc, argv);

#if defined(Q_OS_WIN)
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QFont appFont(QStringLiteral("Segoe UI"));
    appFont.setPointSize(10);
    app.setFont(appFont);
    if (const QScreen* screen = app.primaryScreen()) {
        qDebug() << "Gladiators Windows UI compatibility:"
                 << "screen" << screen->geometry()
                 << "logicalDpi" << screen->logicalDotsPerInch()
                 << "devicePixelRatio" << screen->devicePixelRatio();
    }
#endif

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
    const QString appIconPath = resolveAssetPath(QStringLiteral("assets/icons/shield.png"));
    if (!appIconPath.isEmpty()) {
        const QIcon appIcon(appIconPath);
        if (!appIcon.isNull()) {
            app.setWindowIcon(appIcon);
        }
    }

    MainWindow window;
    if (!appIconPath.isEmpty()) {
        const QIcon appIcon(appIconPath);
        if (!appIcon.isNull()) {
            window.setWindowIcon(appIcon);
        }
    }
    window.showFullScreen();

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

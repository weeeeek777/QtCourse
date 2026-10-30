#include "calculator.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("DGUT"));
    QApplication::setApplicationName(QStringLiteral("Calculator"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    // 样式表统一放在资源文件中，界面外观与逻辑分离
    QFile styleFile(QStringLiteral(":/style.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    Calculator window;
    window.show();
    return app.exec();
}

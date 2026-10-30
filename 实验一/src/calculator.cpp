#include "calculator.h"
#include "ui_calculator.h"

#include <QFontMetricsF>
#include <QKeyEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QTimer>

#include <algorithm>

namespace {
// 语义按键在键盘上的别名（供 keyFromEvent 与 dispatch 共用）
inline QString backspaceKey()
{
    return QStringLiteral("⌫");
}
inline QString equalsKey()
{
    return QStringLiteral("=");
}
constexpr double kDisplayPointSize = 32.0;
constexpr double kErrorPointSize = 18.0;
} // namespace

Calculator::Calculator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Calculator)
{
    ui->setupUi(this);

    m_displayFont.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei UI")});
    m_displayFont.setPointSizeF(kDisplayPointSize);
    m_displayFont.setWeight(QFont::DemiBold);
    ui->display->setFont(m_displayFont);

    setupConnections();

    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    setToolTip(QStringLiteral("键盘：数字 / . + − × ÷ / Enter 等号 / Backspace 退格\n"
                              "Delete=CE  Esc=C  % 百分号  F9 取反  R 倒数  Q 平方  S 开方"));
    updateDisplay();
}

Calculator::~Calculator()
{
    delete ui;
}

void Calculator::setupConnections()
{
    // 让 6 行按钮等高：忽略按钮自身的 sizeHint（只保留最小尺寸），
    // 剩余空间由网格平均分配，避免不同字号造成行高参差不齐。
    for (int row = 0; row < ui->buttonGrid->rowCount(); ++row) {
        ui->buttonGrid->setRowStretch(row, 1);
    }
    for (int column = 0; column < ui->buttonGrid->columnCount(); ++column) {
        ui->buttonGrid->setColumnStretch(column, 1);
    }

    const auto buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        button->setFocusPolicy(Qt::NoFocus); // 焦点留在窗口上，保证键盘事件稳定送达
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        const QString key = button->property("key").toString();
        if (key.isEmpty()) {
            continue;
        }
        // 鼠标点击与键盘最终都走 dispatch()
        connect(button, &QPushButton::clicked, this, [this, key] { dispatch(key); });
    }
}

void Calculator::keyPressEvent(QKeyEvent *event)
{
    const QString key = keyFromEvent(event);
    if (key.isEmpty()) {
        QWidget::keyPressEvent(event);
        return;
    }
    dispatch(key);
    event->accept();
}

QString Calculator::keyFromEvent(const QKeyEvent *event)
{
    // 带 Ctrl / Alt 的组合键交给系统处理，避免误触发计算
    if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        return QString();
    }

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:
        return equalsKey();
    case Qt::Key_Backspace:
        return backspaceKey();
    case Qt::Key_Delete:
        return QStringLiteral("CE");
    case Qt::Key_Escape:
        return QStringLiteral("C");
    case Qt::Key_Percent:
        return QStringLiteral("%");
    case Qt::Key_F9:
        return QStringLiteral("±");
    default:
        break;
    }

    const QString text = event->text();
    if (text.size() == 1) {
        const QChar ch = text.at(0);
        if (ch.isDigit()) {
            return QString(ch);
        }
        if (ch == QLatin1Char('.') || ch == QLatin1Char(',')) {
            return QStringLiteral(".");
        }
        if (ch == QLatin1Char('+')) {
            return QStringLiteral("+");
        }
        if (ch == QLatin1Char('-')) {
            return QStringLiteral("-");
        }
        if (ch == QLatin1Char('*')) {
            return QStringLiteral("×");
        }
        if (ch == QLatin1Char('/')) {
            return QStringLiteral("÷");
        }
        if (ch == QLatin1Char('r') || ch == QLatin1Char('R')) {
            return QStringLiteral("1/x");
        }
        if (ch == QLatin1Char('q') || ch == QLatin1Char('Q')) {
            return QStringLiteral("x²");
        }
        if (ch == QLatin1Char('s') || ch == QLatin1Char('S')) {
            return QStringLiteral("√x");
        }
    }
    return QString();
}

void Calculator::dispatch(const QString &key)
{
    if (key.isEmpty()) {
        return;
    }

    if (key.size() == 1 && key.at(0).isDigit()) {
        m_engine.inputDigit(key.at(0));
    } else if (key == QStringLiteral(".")) {
        m_engine.inputDot();
    } else if (key == QStringLiteral("+") || key == QStringLiteral("-")
               || key == QStringLiteral("×") || key == QStringLiteral("÷")) {
        m_engine.setOperator(key);
    } else if (key == QStringLiteral("=")) {
        m_engine.equals();
    } else if (key == QStringLiteral("C")) {
        m_engine.clearAll();
    } else if (key == QStringLiteral("CE")) {
        m_engine.clearEntry();
    } else if (key == backspaceKey()) {
        m_engine.backspace();
    } else if (key == QStringLiteral("±")) {
        m_engine.toggleSign();
    } else if (key == QStringLiteral("%")) {
        m_engine.percent();
    } else if (key == QStringLiteral("1/x")) {
        m_engine.reciprocal();
    } else if (key == QStringLiteral("x²")) {
        m_engine.square();
    } else if (key == QStringLiteral("√x")) {
        m_engine.squareRoot();
    } else {
        return;
    }

    updateDisplay();
    flashButton(key);
    setFocus(); // 点击按钮后把焦点收回窗口，键盘输入不中断
}

void Calculator::updateDisplay()
{
    ui->expressionDisplay->setText(m_engine.expressionText());
    ui->display->setText(m_engine.displayText());

    const bool isError = m_engine.isError();
    if (ui->display->property("error").toBool() != isError) {
        ui->display->setProperty("error", isError);
        ui->display->style()->unpolish(ui->display);
        ui->display->style()->polish(ui->display);
    }

    fitDisplayFont();
    ui->display->setCursorPosition(ui->display->text().size());
}

void Calculator::fitDisplayFont()
{
    // 数字较长时按可用宽度等比缩小字号，保证结果始终完整显示
    const QString text = ui->display->text();
    if (text.isEmpty()) {
        return;
    }

    const double available = ui->display->contentsRect().width() - 20.0;
    if (available <= 20.0) {
        return;
    }

    double pointSize = m_engine.isError() ? kErrorPointSize : kDisplayPointSize;
    QFont font = m_displayFont;
    font.setPointSizeF(pointSize);
    QFontMetricsF metrics(font);

    for (int i = 0; i < 4; ++i) {
        const double width = metrics.horizontalAdvance(text);
        if (width <= available || pointSize <= 12.0) {
            break;
        }
        pointSize = std::max(12.0, pointSize * available / width);
        font.setPointSizeF(pointSize);
        metrics = QFontMetricsF(font);
    }
    ui->display->setFont(font);
}

void Calculator::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    fitDisplayFont();
}

void Calculator::flashButton(const QString &key)
{
    const auto buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->property("key").toString() != key) {
            continue;
        }
        button->setDown(true);
        QTimer::singleShot(90, button, [button] { button->setDown(false); });
        break;
    }
}

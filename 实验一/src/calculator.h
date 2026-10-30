#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QFont>
#include <QWidget>

#include "calculatorengine.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Calculator;
}
QT_END_NAMESPACE

class QKeyEvent;
class QResizeEvent;

/*!
 * \brief 计算器主窗口。
 *
 * 窗口只负责显示与输入分发：鼠标点击和键盘按键都被转换成同一个
 * “语义按键”字符串（如 "7"、"+"、"⌫"），再交给 dispatch() 统一处理，
 * 因此两种输入方式复用同一套计算逻辑。
 */
class Calculator : public QWidget
{
    Q_OBJECT

public:
    explicit Calculator(QWidget *parent = nullptr);
    ~Calculator() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupConnections();
    void dispatch(const QString &key); // 所有输入的唯一入口
    void updateDisplay();
    void fitDisplayFont();                // 长数字自动缩小字号
    void flashButton(const QString &key); // 键盘输入时给出视觉反馈
    static QString keyFromEvent(const QKeyEvent *event);

    Ui::Calculator *ui;
    CalculatorEngine m_engine;
    QFont m_displayFont;
};

#endif // CALCULATOR_H

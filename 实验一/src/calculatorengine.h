#ifndef CALCULATORENGINE_H
#define CALCULATORENGINE_H

#include <QString>

/*!
 * \brief 计算器核心逻辑（与界面无关）。
 *
 * 采用显式状态机管理输入过程，所有输入（鼠标按键、键盘按键）最终都
 * 汇聚到本类的同一个成员函数上，从而保证两种输入方式共用一套规则。
 *
 * 状态：
 *  - FirstOperand  正在输入/刚得到第一个操作数
 *  - SecondOperand 已经按下运算符，正在输入第二个操作数
 *  - Result        已按等号，显示计算结果（再次输入数字会开始新的计算）
 *  - Error         发生错误（如除数为 0），显示错误提示
 */
class CalculatorEngine
{
public:
    enum class State {
        FirstOperand,
        SecondOperand,
        Result,
        Error
    };

    CalculatorEngine();

    // ---- 输入接口（界面无论鼠标还是键盘都只调用这些函数）----
    void inputDigit(QChar digit);        // 0-9
    void inputDot();                     // 小数点
    void backspace();                    // 退格
    void clearAll();                     // C：清除全部
    void clearEntry();                   // CE：只清除当前输入
    void setOperator(const QString &op); // "+"、"-"、"×"、"÷"
    void equals();                       // "="
    void toggleSign();                   // ±
    void percent();                      // %
    void reciprocal();                   // 1/x
    void square();                       // x²
    void squareRoot();                   // √x

    // ---- 显示数据 ----
    QString displayText() const;    // 主显示区（当前输入或结果）
    QString expressionText() const; // 副显示区（算式）
    State state() const;
    bool isError() const;

    static constexpr int kMaxDigits = 16; // 单个操作数最多 16 位有效数字

private:
    void startNewEntry(QChar digit);      // 开始一个新的数字输入
    void reset();                         // 复位全部状态
    void setError(const QString &message); // 进入错误状态
    double currentValue() const;
    void showValue(double value);         // 将计算结果写回显示
    void finishUnary();                   // 一元运算后的状态收尾
    bool applyPendingOperation(double rhs); // 执行待定运算，失败时已进入错误状态
    static QString formatNumber(double value);
    static double evaluate(double lhs, const QString &op, double rhs);
    int digitCount() const;

    State m_state = State::FirstOperand;
    QString m_current = QStringLiteral("0"); // 正在显示/输入的数字文本
    double m_accumulator = 0.0;              // 第一个操作数或中间结果
    QString m_pendingOp;                     // 待定运算符，空表示没有
    QString m_expression;                    // 副显示区文本
    bool m_entering = false;                 // 是否正在逐位输入数字
    bool m_secondOperandReady = false;       // 第二个操作数是否已经就绪（用于连续运算）

    // 连续按 "=" 时重复上一次运算
    QString m_lastOp;
    double m_lastOperand = 0.0;
    bool m_hasLastOperation = false;
};

#endif // CALCULATORENGINE_H

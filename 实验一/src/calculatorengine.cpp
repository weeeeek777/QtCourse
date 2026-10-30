#include "calculatorengine.h"

#include <QtGlobal>

#include <cmath>
#include <limits>

namespace {
constexpr double kEpsilon = 1e-15;
}

CalculatorEngine::CalculatorEngine()
{
    reset();
}

void CalculatorEngine::reset()
{
    m_state = State::FirstOperand;
    m_current = QStringLiteral("0");
    m_accumulator = 0.0;
    m_pendingOp.clear();
    m_expression.clear();
    m_entering = false;
    m_secondOperandReady = false;
    m_lastOp.clear();
    m_lastOperand = 0.0;
    m_hasLastOperation = false;
}

void CalculatorEngine::setError(const QString &message)
{
    m_state = State::Error;
    m_current = message;
    m_entering = false;
    m_secondOperandReady = false;
    m_pendingOp.clear();
    m_hasLastOperation = false;
}

void CalculatorEngine::inputDigit(QChar digit)
{
    if (m_state == State::Error || m_state == State::Result) {
        reset(); // 出错或显示结果后，输入数字即开始一次新的计算
    }

    if (!m_entering) {
        startNewEntry(digit);
        return;
    }

    if (digitCount() >= kMaxDigits) {
        return; // 限制单个操作数的长度，避免超出显示宽度
    }

    if (m_current == QStringLiteral("0")) {
        m_current = digit;
    } else if (m_current == QStringLiteral("-0")) {
        m_current = QStringLiteral("-") + digit;
    } else {
        m_current += digit;
    }
}

void CalculatorEngine::startNewEntry(QChar digit)
{
    m_current = digit;
    m_entering = true;
    // 按下运算符后再输入数字 → 进入第二个操作数；否则是第一个操作数
    m_secondOperandReady = !m_pendingOp.isEmpty();
    m_state = m_pendingOp.isEmpty() ? State::FirstOperand : State::SecondOperand;
    if (m_pendingOp.isEmpty()) {
        m_expression.clear();
        m_hasLastOperation = false;
    }
}

void CalculatorEngine::inputDot()
{
    if (m_state == State::Error || m_state == State::Result) {
        reset();
    }

    if (!m_entering) {
        m_current = QStringLiteral("0.");
        m_entering = true;
        m_secondOperandReady = !m_pendingOp.isEmpty();
        m_state = m_pendingOp.isEmpty() ? State::FirstOperand : State::SecondOperand;
        if (m_pendingOp.isEmpty()) {
            m_expression.clear();
            m_hasLastOperation = false;
        }
        return;
    }

    if (m_current.contains(QLatin1Char('.'))) {
        return; // 同一个操作数中不允许出现第二个小数点
    }
    m_current += QLatin1Char('.');
}

void CalculatorEngine::backspace()
{
    if (m_state == State::Error) {
        reset();
        return;
    }
    if (!m_entering) {
        return; // 结果或刚按下运算符时没有可退格的输入
    }

    m_current.chop(1);
    if (m_current.isEmpty() || m_current == QStringLiteral("-")) {
        m_current = QStringLiteral("0");
        m_entering = false;
    }
    if (m_current.endsWith(QLatin1Char('.')) && m_current.size() == 1) {
        m_current = QStringLiteral("0");
    }
}

void CalculatorEngine::clearAll()
{
    reset();
}

void CalculatorEngine::clearEntry()
{
    if (m_state == State::Error) {
        reset();
        return;
    }

    m_current = QStringLiteral("0");
    m_entering = true;
    m_secondOperandReady = !m_pendingOp.isEmpty();
    m_state = m_pendingOp.isEmpty() ? State::FirstOperand : State::SecondOperand;
}

void CalculatorEngine::setOperator(const QString &op)
{
    if (m_state == State::Error) {
        return;
    }

    if (m_state == State::Result) {
        // 用上一次的结果继续参与运算
        m_accumulator = currentValue();
        m_pendingOp.clear();
        m_expression.clear();
        m_hasLastOperation = false;
        m_state = State::FirstOperand;
    }

    if (!m_pendingOp.isEmpty() && m_secondOperandReady) {
        if (!applyPendingOperation(currentValue())) {
            return; // 连续运算时可能除零，错误已由 setError 处理
        }
    } else if (m_pendingOp.isEmpty()) {
        m_accumulator = currentValue();
    }

    m_pendingOp = op;
    m_secondOperandReady = false;
    m_entering = false;
    m_hasLastOperation = false;
    m_state = State::SecondOperand;
    m_expression = formatNumber(m_accumulator) + QLatin1Char(' ') + m_pendingOp;
}

void CalculatorEngine::equals()
{
    if (m_state == State::Error) {
        return;
    }

    if (m_pendingOp.isEmpty()) {
        if (m_hasLastOperation) {
            // 连续按等号：重复上一次运算
            const double lhs = m_accumulator;
            const double rhs = m_lastOperand;
            const double result = evaluate(lhs, m_lastOp, rhs);
            if (!std::isfinite(result)) {
                setError(QStringLiteral("结果超出范围"));
                return;
            }
            m_expression = formatNumber(lhs) + QLatin1Char(' ') + m_lastOp
                           + QLatin1Char(' ') + formatNumber(rhs) + QStringLiteral(" =");
            showValue(result);
            m_accumulator = result;
            m_entering = false;
            m_state = State::Result;
            return;
        }

        m_expression = m_current + QStringLiteral(" =");
        m_accumulator = currentValue();
        m_entering = false;
        m_state = State::Result;
        return;
    }

    const double lhs = m_accumulator;
    const double rhs = currentValue();
    if (!applyPendingOperation(rhs)) {
        return;
    }

    m_lastOp = m_pendingOp;
    m_lastOperand = rhs;
    m_hasLastOperation = true;
    m_expression = formatNumber(lhs) + QLatin1Char(' ') + m_pendingOp
                   + QLatin1Char(' ') + formatNumber(rhs) + QStringLiteral(" =");
    m_pendingOp.clear();
    m_secondOperandReady = false;
    m_entering = false;
    m_state = State::Result;
}

void CalculatorEngine::toggleSign()
{
    if (m_state == State::Error || m_current == QStringLiteral("0")) {
        return;
    }

    if (m_current.startsWith(QLatin1Char('-'))) {
        m_current.remove(0, 1);
    } else {
        m_current.prepend(QLatin1Char('-'));
    }
    if (!m_entering) {
        m_accumulator = currentValue();
    }
}

void CalculatorEngine::percent()
{
    if (m_state == State::Error) {
        return;
    }

    double value = currentValue();
    if (!m_pendingOp.isEmpty() && (m_pendingOp == QStringLiteral("+")
                                   || m_pendingOp == QStringLiteral("-"))) {
        // 5 + 10% 视为 5 + 5×10%
        value = m_accumulator * value / 100.0;
    } else {
        value /= 100.0;
    }
    showValue(value);
    finishUnary();
}

void CalculatorEngine::reciprocal()
{
    if (m_state == State::Error) {
        return;
    }
    const double value = currentValue();
    if (std::abs(value) < kEpsilon) {
        setError(QStringLiteral("不能除以零"));
        return;
    }
    showValue(1.0 / value);
    finishUnary();
}

void CalculatorEngine::square()
{
    if (m_state == State::Error) {
        return;
    }
    const double value = currentValue();
    const double result = value * value;
    if (!std::isfinite(result)) {
        setError(QStringLiteral("结果超出范围"));
        return;
    }
    showValue(result);
    finishUnary();
}

void CalculatorEngine::squareRoot()
{
    if (m_state == State::Error) {
        return;
    }
    const double value = currentValue();
    if (value < 0.0) {
        setError(QStringLiteral("无效输入"));
        return;
    }
    showValue(std::sqrt(value));
    finishUnary();
}

void CalculatorEngine::finishUnary()
{
    m_secondOperandReady = !m_pendingOp.isEmpty();
    m_entering = false;
    m_hasLastOperation = false;
    m_state = m_pendingOp.isEmpty() ? State::Result : State::SecondOperand;
    if (m_pendingOp.isEmpty()) {
        m_expression.clear(); // 一元运算后不再显示上一条算式
    }
}

bool CalculatorEngine::applyPendingOperation(double rhs)
{
    const double lhs = m_accumulator;
    if (m_pendingOp == QStringLiteral("÷") && std::abs(rhs) < kEpsilon) {
        m_expression = formatNumber(lhs) + QLatin1Char(' ') + m_pendingOp
                       + QLatin1Char(' ') + formatNumber(rhs) + QStringLiteral(" =");
        setError(QStringLiteral("不能除以零"));
        return false;
    }

    const double result = evaluate(lhs, m_pendingOp, rhs);
    if (!std::isfinite(result)) {
        setError(QStringLiteral("结果超出范围"));
        return false;
    }

    showValue(result);
    m_accumulator = result;
    return true;
}

double CalculatorEngine::evaluate(double lhs, const QString &op, double rhs)
{
    if (op == QStringLiteral("+")) {
        return lhs + rhs;
    }
    if (op == QStringLiteral("-")) {
        return lhs - rhs;
    }
    if (op == QStringLiteral("×")) {
        return lhs * rhs;
    }
    if (op == QStringLiteral("÷")) {
        return rhs == 0.0 ? std::numeric_limits<double>::quiet_NaN() : lhs / rhs;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

void CalculatorEngine::showValue(double value)
{
    // 只更新显示文本；累加器由调用者按语义决定是否更新，
    // 否则 "200 + 10%" 会把第一个操作数覆盖成 20。
    m_current = formatNumber(value);
}

double CalculatorEngine::currentValue() const
{
    bool ok = false;
    const double value = m_current.toDouble(&ok);
    return ok ? value : 0.0;
}

int CalculatorEngine::digitCount() const
{
    int count = 0;
    for (const QChar ch : m_current) {
        if (ch.isDigit()) {
            ++count;
        }
    }
    return count;
}

QString CalculatorEngine::displayText() const
{
    return m_current;
}

QString CalculatorEngine::expressionText() const
{
    return m_expression;
}

CalculatorEngine::State CalculatorEngine::state() const
{
    return m_state;
}

bool CalculatorEngine::isError() const
{
    return m_state == State::Error;
}

QString CalculatorEngine::formatNumber(double value)
{
    if (!std::isfinite(value)) {
        return QStringLiteral("错误");
    }
    if (value == 0.0) {
        return QStringLiteral("0"); // 同时消除 -0
    }

    // 15 位有效数字足以消除 0.1+0.2 这类二进制浮点误差
    QString text = QString::number(value, 'g', 15);
    if (!text.contains(QLatin1Char('e')) && !text.contains(QLatin1Char('E'))) {
        if (text.contains(QLatin1Char('.'))) {
            while (text.endsWith(QLatin1Char('0'))) {
                text.chop(1);
            }
            if (text.endsWith(QLatin1Char('.'))) {
                text.chop(1);
            }
        }
    }
    return text;
}

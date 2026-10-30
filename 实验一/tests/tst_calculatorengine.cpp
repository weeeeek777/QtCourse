#include "calculator.h"
#include "calculatorengine.h"

#include <QLineEdit>
#include <QPushButton>
#include <QTest>

class TestCalculator : public QObject
{
    Q_OBJECT

private:
    static void type(CalculatorEngine &engine, const QString &keys);

private slots:
    void basicAddition();
    void floatingPointResult();
    void decimalInput();
    void repeatedDotIsIgnored();
    void leadingZeroHandling();
    void operatorCanBeReplaced();
    void chainedOperations();
    void divideByZero();
    void errorRecoveryByTyping();
    void backspaceDuringInput();
    void backspaceAfterResultIsIgnored();
    void clearEntryKeepsOperator();
    void clearAllResetsEverything();
    void continueAfterResult();
    void repeatedEquals();
    void percentOfAccumulator();
    void toggleSign();
    void squareAndSquareRoot();
    void squareRootOfNegative();
    void reciprocalOfZero();
    void digitLimit();
    void divideByZeroCanBeContinued();
    void keyboardAndMouseShareLogic();
};

void TestCalculator::type(CalculatorEngine &engine, const QString &keys)
{
    for (const QChar key : keys) {
        if (key.isDigit()) {
            engine.inputDigit(key);
        } else if (key == QLatin1Char('.')) {
            engine.inputDot();
        } else {
            engine.setOperator(QString(key));
        }
    }
}

void TestCalculator::basicAddition()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("1+2"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("3"));
    QCOMPARE(engine.expressionText(), QStringLiteral("1 + 2 ="));

    engine.clearAll();
    type(engine, QStringLiteral("12+30"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("42"));
}

void TestCalculator::floatingPointResult()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("0.1+0.2"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("0.3")); // 消除二进制浮点误差
}

void TestCalculator::decimalInput()
{
    CalculatorEngine engine;
    engine.inputDigit(QLatin1Char('3'));
    engine.inputDot();
    QCOMPARE(engine.displayText(), QStringLiteral("3."));
    engine.inputDigit(QLatin1Char('5'));
    QCOMPARE(engine.displayText(), QStringLiteral("3.5"));

    engine.clearAll();
    engine.inputDot();
    QCOMPARE(engine.displayText(), QStringLiteral("0."));
}

void TestCalculator::repeatedDotIsIgnored()
{
    CalculatorEngine engine;
    engine.inputDigit(QLatin1Char('1'));
    engine.inputDot();
    engine.inputDot();
    engine.inputDot();
    QCOMPARE(engine.displayText(), QStringLiteral("1."));
    engine.inputDigit(QLatin1Char('5'));
    QCOMPARE(engine.displayText(), QStringLiteral("1.5"));
    engine.inputDot();
    QCOMPARE(engine.displayText(), QStringLiteral("1.5"));
}

void TestCalculator::leadingZeroHandling()
{
    CalculatorEngine engine;
    engine.inputDigit(QLatin1Char('0'));
    engine.inputDigit(QLatin1Char('0'));
    engine.inputDigit(QLatin1Char('5'));
    QCOMPARE(engine.displayText(), QStringLiteral("5"));
}

void TestCalculator::operatorCanBeReplaced()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("8+"));
    engine.setOperator(QStringLiteral("×"));
    type(engine, QStringLiteral("2"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("16"));
}

void TestCalculator::chainedOperations()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("2+3+4"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("9"));

    // 连续运算：按下第二个运算符时先算出中间结果
    engine.clearAll();
    type(engine, QStringLiteral("10-4"));
    engine.setOperator(QStringLiteral("×"));
    QCOMPARE(engine.displayText(), QStringLiteral("6"));
    type(engine, QStringLiteral("3"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("18"));
}

void TestCalculator::divideByZero()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("5÷0"));
    engine.equals();
    QVERIFY(engine.isError());
    QCOMPARE(engine.displayText(), QStringLiteral("不能除以零"));
    QCOMPARE(engine.expressionText(), QStringLiteral("5 ÷ 0 ="));
}

void TestCalculator::errorRecoveryByTyping()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("8÷0"));
    engine.equals();
    QVERIFY(engine.isError());

    engine.inputDigit(QLatin1Char('7'));
    QVERIFY(!engine.isError());
    QCOMPARE(engine.displayText(), QStringLiteral("7"));

    // 错误状态下按运算符不产生新计算
    type(engine, QStringLiteral("1÷0"));
    engine.equals();
    QVERIFY(engine.isError());
    engine.setOperator(QStringLiteral("+"));
    QVERIFY(engine.isError());
}

void TestCalculator::backspaceDuringInput()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("123"));
    engine.backspace();
    QCOMPARE(engine.displayText(), QStringLiteral("12"));
    engine.backspace();
    engine.backspace();
    QCOMPARE(engine.displayText(), QStringLiteral("0"));

    engine.inputDot();
    engine.inputDigit(QLatin1Char('5'));
    engine.backspace();
    QCOMPARE(engine.displayText(), QStringLiteral("0."));
}

void TestCalculator::backspaceAfterResultIsIgnored()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("1+2"));
    engine.equals();
    engine.backspace();
    QCOMPARE(engine.displayText(), QStringLiteral("3"));
}

void TestCalculator::clearEntryKeepsOperator()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("5+3"));
    engine.clearEntry();
    QCOMPARE(engine.displayText(), QStringLiteral("0"));
    type(engine, QStringLiteral("2"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("7"));
}

void TestCalculator::clearAllResetsEverything()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("5+3"));
    engine.clearAll();
    QCOMPARE(engine.displayText(), QStringLiteral("0"));
    QCOMPARE(engine.expressionText(), QString());
    engine.inputDigit(QLatin1Char('9'));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("9"));
}

void TestCalculator::continueAfterResult()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("2×3"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("6"));

    engine.setOperator(QStringLiteral("+"));
    type(engine, QStringLiteral("4"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("10"));

    // 结果后直接输入数字 → 开始新的计算
    engine.inputDigit(QLatin1Char('9'));
    QCOMPARE(engine.displayText(), QStringLiteral("9"));
    QCOMPARE(engine.expressionText(), QString());
}

void TestCalculator::repeatedEquals()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("5+3"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("8"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("11"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("14"));
}

void TestCalculator::percentOfAccumulator()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("200+10"));
    engine.percent();
    QCOMPARE(engine.displayText(), QStringLiteral("20"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("220"));

    engine.clearAll();
    engine.inputDigit(QLatin1Char('5'));
    engine.inputDigit(QLatin1Char('0'));
    engine.percent();
    QCOMPARE(engine.displayText(), QStringLiteral("0.5"));
}

void TestCalculator::toggleSign()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("5"));
    engine.toggleSign();
    QCOMPARE(engine.displayText(), QStringLiteral("-5"));
    engine.toggleSign();
    QCOMPARE(engine.displayText(), QStringLiteral("5"));

    engine.clearAll();
    type(engine, QStringLiteral("0"));
    engine.toggleSign();
    QCOMPARE(engine.displayText(), QStringLiteral("0")); // 0 不显示负号
}

void TestCalculator::squareAndSquareRoot()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("9"));
    engine.squareRoot();
    QCOMPARE(engine.displayText(), QStringLiteral("3"));

    engine.clearAll();
    type(engine, QStringLiteral("7"));
    engine.square();
    QCOMPARE(engine.displayText(), QStringLiteral("49"));
}

void TestCalculator::squareRootOfNegative()
{
    CalculatorEngine engine;
    engine.inputDigit(QLatin1Char('9'));
    engine.toggleSign();
    engine.squareRoot();
    QVERIFY(engine.isError());
    QCOMPARE(engine.displayText(), QStringLiteral("无效输入"));
}

void TestCalculator::reciprocalOfZero()
{
    CalculatorEngine engine;
    engine.clearAll();
    engine.reciprocal();
    QVERIFY(engine.isError());
    QCOMPARE(engine.displayText(), QStringLiteral("不能除以零"));
}

void TestCalculator::digitLimit()
{
    CalculatorEngine engine;
    for (int i = 0; i < 20; ++i) {
        engine.inputDigit(QLatin1Char('7'));
    }
    QCOMPARE(engine.displayText().size(), CalculatorEngine::kMaxDigits);
}

void TestCalculator::divideByZeroCanBeContinued()
{
    CalculatorEngine engine;
    type(engine, QStringLiteral("5÷0"));
    engine.equals();
    QVERIFY(engine.isError());
    engine.clearAll();
    type(engine, QStringLiteral("6÷2"));
    engine.equals();
    QCOMPARE(engine.displayText(), QStringLiteral("3"));
}

void TestCalculator::keyboardAndMouseShareLogic()
{
    Calculator window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *display = window.findChild<QLineEdit *>(QStringLiteral("display"));
    QVERIFY(display);

    // 键盘输入 12+3，回车求值
    QTest::keyClick(&window, Qt::Key_1);
    QTest::keyClick(&window, Qt::Key_2);
    QTest::keyClick(&window, Qt::Key_Plus);
    QTest::keyClick(&window, Qt::Key_3);
    QCOMPARE(display->text(), QStringLiteral("3"));
    QTest::keyClick(&window, Qt::Key_Return);
    QCOMPARE(display->text(), QStringLiteral("15"));

    // 鼠标点击 7 ÷ 2 =，结果应与键盘一致
    QTest::mouseClick(window.findChild<QPushButton *>(QStringLiteral("btnClear")), Qt::LeftButton);
    QTest::mouseClick(window.findChild<QPushButton *>(QStringLiteral("btn7")), Qt::LeftButton);
    QTest::mouseClick(window.findChild<QPushButton *>(QStringLiteral("btnDivide")), Qt::LeftButton);
    QTest::mouseClick(window.findChild<QPushButton *>(QStringLiteral("btn2")), Qt::LeftButton);
    QTest::mouseClick(window.findChild<QPushButton *>(QStringLiteral("btnEquals")), Qt::LeftButton);
    QCOMPARE(display->text(), QStringLiteral("3.5"));
}

QTEST_MAIN(TestCalculator)
#include "tst_calculatorengine.moc"

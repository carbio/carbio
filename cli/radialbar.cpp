#include <QPainter>

#include "carbio/floating_point.h"
#include "radialbar.h"


RadialBar::RadialBar(QQuickItem *parent)
    : QQuickPaintedItem(parent),

    m_SuffixText(""),

    m_Size(200),
    m_StartAngle(40),
    m_SpanAngle(280),
    m_MinValue(0),
    m_MaxValue(100),
    m_Value(50),

    m_BackgroundColor(Qt::transparent),
    m_DialColor(QColor(80,80,80)),
    m_ProgressColor(QColor(135,26,5)),
    m_TextColor(QColor(0, 0, 0)),

    m_DialWidth(15),
    m_PenStyle(Qt::FlatCap),
    m_DialType(DialType::MinToMax),
    m_ShowText(true)
{
    setWidth(200);
    setHeight(200);
    setSmooth(true);
    setAntialiasing(true);
}

RadialBar::~RadialBar()
{
}

void RadialBar::paint(QPainter *painter)
{
    double startAngle;
    double spanAngle;
    qreal size = qMin(this->width(), this->height());
    setWidth(size);
    setHeight(size);
    QRectF rect = this->boundingRect();
    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen = painter->pen();
    pen.setCapStyle(m_PenStyle);

    startAngle = -90 - m_StartAngle;
    if(FullDial != m_DialType)
    {
        spanAngle = 0 - m_SpanAngle;
    }
    else
    {
        spanAngle = -360;
    }

    //Draw outer dial
    painter->save();
    pen.setWidth(m_DialWidth);
    pen.setColor(m_DialColor);
    painter->setPen(pen);
    qreal offset = m_DialWidth / 2;
    if(m_DialType == MinToMax)
    {
        painter->drawArc(rect.adjusted(offset, offset, -offset, -offset), static_cast<int>(startAngle * 16), static_cast<int>(spanAngle * 16));
    }
    else if(m_DialType == FullDial)
    {
        painter->drawArc(rect.adjusted(offset, offset, -offset, -offset), -90 * 16, -360 * 16);
    }
    else
    {
        //do not draw dial
    }
    painter->restore();

    //Draw background
    painter->save();
    painter->setBrush(m_BackgroundColor);
    painter->setPen(m_BackgroundColor);
    qreal inner = offset * 2;
    painter->drawEllipse(rect.adjusted(inner, inner, -inner, -inner));
    painter->restore();

    //Draw progress text with suffix
    painter->save();
    painter->setFont(m_TextFont);
    pen.setColor(m_TextColor);
    painter->setPen(pen);
    if(m_ShowText)
    {
        painter->drawText(rect.adjusted(offset, offset, -offset, -offset), Qt::AlignCenter,QString::number(m_Value) + m_SuffixText);
    }
    else
    {
        painter->drawText(rect.adjusted(offset, offset, -offset, -offset), Qt::AlignCenter, m_SuffixText);
    }
    painter->restore();

    //Draw progress bar
    painter->save();
    pen.setWidth(m_DialWidth);
    pen.setColor(m_ProgressColor);
    qreal valueAngle = ((m_Value - m_MinValue)/(m_MaxValue - m_MinValue)) * spanAngle;  //Map value to angle range
    painter->setPen(pen);
    painter->drawArc(rect.adjusted(offset, offset, -offset, -offset), static_cast<int>(startAngle * 16), static_cast<int>(valueAngle * 16));
    painter->restore();
}

void RadialBar::setSize(qreal size)
{
    if (!carbio::nearlyEqual(m_Size, size))
    {
        m_Size = size;
        emit sizeChanged();
    }
}

void RadialBar::setStartAngle(qreal angle)
{
    if (!carbio::nearlyEqual(m_StartAngle, angle))
    {
        m_StartAngle = angle;
        emit startAngleChanged();
    }
}

void RadialBar::setSpanAngle(qreal angle)
{
    if (!carbio::nearlyEqual(m_SpanAngle, angle))
    {
        m_SpanAngle = angle;
        emit spanAngleChanged();
    }
}

void RadialBar::setMinValue(qreal value)
{
    if (!carbio::nearlyEqual(m_MinValue, value))
    {
        m_MinValue = value;
        emit minValueChanged();
    }
}

void RadialBar::setMaxValue(qreal value)
{
    if (!carbio::nearlyEqual(m_MaxValue, value))
    {
        m_MaxValue = value;
        emit maxValueChanged();
    }
}

void RadialBar::setValue(qreal value)
{
    if (!carbio::nearlyEqual(m_Value, value))
    {
        m_Value = value;
        update();
        emit valueChanged();
    }
}

void RadialBar::setDialWidth(qreal width)
{
    if (!carbio::nearlyEqual(m_DialWidth, width))
    {
        m_DialWidth = static_cast<int>(width);
        emit dialWidthChanged();
    }
}

void RadialBar::setBackgroundColor(QColor color)
{
    if (m_BackgroundColor != color)
    {
        m_BackgroundColor = color;
        emit backgroundColorChanged();
    }
}

void RadialBar::setForegroundColor(QColor color)
{
    if (m_DialColor != color)
    {
        m_DialColor = color;
        emit foregroundColorChanged();
    }
}

void RadialBar::setProgressColor(QColor color)
{
    if(m_ProgressColor != color)
    {
        m_ProgressColor = color;
        emit progressColorChanged();
    }
}

void RadialBar::setTextColor(QColor color)
{
    if(m_TextColor != color)
    {
        m_TextColor = color;
        emit textColorChanged();
    }
}

void RadialBar::setSuffixText(QString text)
{
    if (m_SuffixText != text)
    {
        m_SuffixText = text;
        emit suffixTextChanged();
    }
}

void RadialBar::setShowText(bool show)
{
    if (m_ShowText != show)
    {
        m_ShowText = show;
    }
}

void RadialBar::setPenStyle(Qt::PenCapStyle style)
{
    if (m_PenStyle != style)
    {
        m_PenStyle = style;
        emit penStyleChanged();
    }
}

void RadialBar::setDialType(RadialBar::DialType type)
{
    if (m_DialType != type)
    {
        m_DialType = type;
        emit dialTypeChanged();
    }
}

void RadialBar::setTextFont(QFont font)
{
    if (m_TextFont != font)
    {
        m_TextFont = font;
        emit textFontChanged();
    }
}

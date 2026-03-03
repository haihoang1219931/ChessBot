#include "Button.h"
#include "ApplicationController.h"
Button::Button(ApplicationController* app) :
    m_app(app),
    m_buttonSignalHigh(false),
    m_buttonSignalHighStartTime(0),
    m_buttonSignalHighTime(0),
    m_buttonSignalHandling(false),
    m_buttonState(BUTTON_NOMAL)
{

}

void Button::checkState() {
    if(m_buttonSignalHigh)
    {
        if(!m_buttonSignalHandling) {
            m_buttonSignalHighStartTime = m_app->getSystemTime()*1000;
            m_buttonSignalHandling = true;
            m_buttonState = BUTTON_PRESS;
        } else {
            m_buttonSignalHighTime = m_app->getSystemTime()*1000 - m_buttonSignalHighStartTime;
            if(m_buttonSignalHighTime >5) {
                m_buttonState = BUTTON_HOLD;
            }
        }
    } else {
        if(m_buttonSignalHandling) {
            m_buttonSignalHighTime = m_app->getSystemTime()*1000 - m_buttonSignalHighStartTime;
            m_buttonSignalHandling = false;
        }
        m_buttonState = BUTTON_NOMAL;
    }
}

void Button::setPressed(bool pressed) {
    m_buttonSignalHigh = pressed;
}

BUTTON_STATE Button::buttonState() {
    return m_buttonState;
}

long Button::holdingTime() {
    return m_buttonSignalHighTime;
}

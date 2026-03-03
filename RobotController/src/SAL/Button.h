#ifndef BUTTON_H
#define BUTTON_H

typedef enum {
    BUTTON_NOMAL,
    BUTTON_PRESS,
    BUTTON_HOLD,
}BUTTON_STATE;

class ApplicationController;
class Button
{
public:
    Button(ApplicationController* app);
    void checkState();
    void setPressed(bool pressed);
    BUTTON_STATE buttonState();
    long holdingTime();

private:
    ApplicationController* m_app;
    bool m_buttonSignalHigh;
    long m_buttonSignalHighStartTime;
    long m_buttonSignalHighTime;
    bool m_buttonSignalHandling;
    BUTTON_STATE m_buttonState;
};

#endif // BUTTON_H

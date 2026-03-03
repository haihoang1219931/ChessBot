#ifndef COMMANDREADER_H
#define COMMANDREADER_H

class ApplicationController;
class CommandReader
{
public:
    CommandReader(ApplicationController* app);
    void loop();
private:
    ApplicationController* m_app;
    int m_incomingByte;
};

#endif // COMMANDREADER_H

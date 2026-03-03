#include "CommandReader.h"
#include "ApplicationController.h"
#include <string.h>
volatile char m_command[128];
CommandReader::CommandReader(ApplicationController* app) :
    m_app(app)
{
  m_incomingByte = 0;
}
void CommandReader::loop() {
  memset((void*)m_command,0,sizeof(m_command));
  m_incomingByte = m_app->readSerial((char*)m_command,sizeof(m_command));
  // m_incomingByte = 32;
  if(m_incomingByte>0) {
    m_app->printf("====== Received [%d][",m_incomingByte);
    // m_app->printf("[%d][%s]\r\n",m_incomingByte,m_command);
    for(int i=0; i < m_incomingByte; i++) {
      m_app->printf("%02X%c", m_command[i], i==m_incomingByte-1?0:' ');
    }
    m_app->printf("]\r\n");
    m_app->executeCommand((char*)m_command);
  }
}

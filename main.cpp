/***************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ARDUINO
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "RobotSim/MainProcess.h"
#include "RobotSim/VideoDisplay/VideoRender.h"

#define QML_ROBOT
#ifdef QML_ROBOT
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    qmlRegisterType<VideoRender>("diy", 1, 0, "VideoRender");
    qmlRegisterType<MainProcess>("diy", 1, 0, "MainProcess");
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
            return -1;
    return app.exec();
}
#else
int main(int argc, char *argv[]) {
    printf("Hello\r\n");
    int numMotor = argc - 1;
    int listSteps[12];
    int listTimeStep[12];
    int maxStep = 0;
    int minStep = 0;

    for(int i=0; i< numMotor; i++){
        listSteps[i] = atoi(argv[i+1]);
        if(listSteps[i] > maxStep) maxStep = listSteps[i];
        if(minStep == 0) minStep = listSteps[i];
        else if(listSteps[i] < minStep) minStep = listSteps[i];
    }
//    printf("minStep[%d] maxStep[%d]\r\n",
//           minStep,maxStep);
    int minScale = (int)pow(10,1+(int)log10((double)maxStep));
//    printf("minScale[%d]\r\n",minScale);
    for(int i=0; i< numMotor; i++){
        listTimeStep[i] = round((float)(minScale * maxStep) / listSteps[i]);
//        printf("M[%d] [%d %d = %d] ",
//               i,listSteps[i],listTimeStep[i],listSteps[i]*listTimeStep[i]);
    }
    return 0;
}
#endif
#endif

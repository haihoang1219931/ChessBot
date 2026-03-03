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

import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import diy 1.0
ApplicationWindow {
    id: wroot
    visible: true
    width: 1400
    height: 780
    title: qsTr("Chess Simulator")
    color: "gray"
    MainProcess{
        id: mainProcess
//        onListAngleChanged: {
//            command.angle1 = listAngle[0];
//            command.angle2 = listAngle[1];
//        }
    }

    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 80
        anchors.leftMargin: 80
        spacing: 20
        Rectangle {
            width: 600
            height: 600
            border.color: "red"
            color: "transparent"
            property int maxRow: 8
            property int maxCol: 12
            property int square: 50 * command.visualScale
            Canvas {
                id: cvsGridLine
                x: parent.width/2
                y: 0
                width: parent.width/2
                height: parent.height
                z: chessBoard.z + 2
                onPaint: {
                    var ctx = getContext("2d");

                    ctx.strokeStyle = "green";
                    ctx.lineWidth = 2;
                    for(var rowId = 0; rowId < parent.maxRow; rowId ++) {
                        for(var colId =0; colId < parent.maxCol; colId ++) {
                            ctx.beginPath();
                            ctx.moveTo(rowId * parent.square, 0);
                            ctx.lineTo(rowId * parent.square, colId * parent.square);
                            ctx.stroke();
                            ctx.beginPath();
                            ctx.moveTo(0, colId * parent.square);
                            ctx.lineTo(rowId * parent.square, colId * parent.square);
                            ctx.stroke();
                        }
                    }
                }
            }
            Canvas {
                id: cvsGridLineLeft
                x: 0
                y: 0
                width: parent.width/2
                height: parent.height
                z: chessBoard.z + 3
                onPaint: {
                    var ctx = getContext("2d");

                    ctx.strokeStyle = "red";
                    ctx.lineWidth = 2;
                    for(var rowId = 0; rowId < parent.maxRow; rowId ++) {
                        for(var colId =0; colId < parent.maxCol; colId ++) {
                            ctx.beginPath();
                            ctx.moveTo(rowId * parent.square, 0);
                            ctx.lineTo(rowId * parent.square, colId * parent.square);
                            ctx.stroke();
                            ctx.beginPath();
                            ctx.moveTo(0, colId * parent.square);
                            ctx.lineTo(rowId * parent.square, colId * parent.square);
                            ctx.stroke();
                        }
                    }
                }
            }
            Canvas {
                id: cvsPathPlan
                z: chessBoard.z + 1
                anchors.fill: parent
                property var currPos
                property var prevPos
                function clearCanvas() {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    requestPaint();
                }

                onPaint: {
                    var ctx = getContext("2d");

                    ctx.strokeStyle = "blue";
                    ctx.lineWidth = 1;

                    ctx.beginPath();
                    if(typeof prevPos !== "undefined") {
                        ctx.moveTo(prevPos.x, prevPos.y);
                    }
                    if(typeof currPos !== "undefined") {
                        ctx.lineTo(currPos.x, currPos.y);
                    }
                    ctx.stroke();

                }
            }

            ChessBoard {
                id: chessBoard
                x: parent.width/2+command.chessBoardPosX
                y: command.chessBoardPosY
                width: command.chessBoardWidth
                height: command.chessBoardWidth
                visible: command.showChessBoard
            }

            DropZone {
                id: dropZone
                x: parent.width/2 + command.chessBoardPosX - command.chessBoardWidth / 8 * 3
                y: command.chessBoardPosY
                width: command.chessBoardWidth / 4
                height: command.chessBoardWidth
            }

            DropZone {
                id: dropZoneGuest
                x: parent.width/2 + command.chessBoardPosX +
                   command.chessBoardWidth + command.chessBoardWidth / 8 * 1
                y: command.chessBoardPosY
                width: command.chessBoardWidth / 4
                height: command.chessBoardWidth
            }

            Robot {
                id: robot
                x: parent.width/2
                y: -5
                arm1Width: command.arm1Length
                arm2Width: command.arm2Length
                arm3Width: command.arm3Length
                arm4Width: parseInt(command.arm4Length * Math.cos((45-command.upAngle)/180.0*Math.PI))
                arm5Width: command.arm5Length
                angle1: 180-command.angle1
                angle2: 180+command.angle2
                angle3: 180+command.angle3
                angle4: 180+command.angle4
                angle5: 180+command.angle5
                grabberAngle: command.grabberAngle
            }
        }
        CommandController{
            id: command
            width: 500
            height: 300
            property real visualScale: 1
            arm1Length: mainProcess.listArmLength[1] * visualScale
            arm2Length: mainProcess.listArmLength[2] * visualScale
            arm3Length: mainProcess.listArmLength[3] * visualScale
            arm4Length: mainProcess.listArmLength[4] * visualScale
            arm5Length: mainProcess.listArmLength[5] * visualScale
            angle1: mainProcess.listAngle[1]
            angle2: mainProcess.listAngle[2]
            angle3: mainProcess.listAngle[3]
            angle4: mainProcess.listAngle[4]
            angle5: 180
            upAngle: mainProcess.listAngle[5]
            grabberAngle: mainProcess.listAngle[0]
            chessBoardPosX: mainProcess.chessBoardInfo[0] * visualScale
            chessBoardPosY: mainProcess.chessBoardInfo[1] * visualScale
            chessBoardWidth: mainProcess.chessBoardInfo[2] * visualScale
//            onAngle1Changed: {
//                var pos = robot.getPosition();
//                if(typeof cvsPathPlan.currPos == "undefined") {
//                    cvsPathPlan.currPos = Qt.point(pos.x + robot.x, pos.y + robot.y);
//                    cvsPathPlan.prevPos = Qt.point(pos.x + robot.x, pos.y + robot.y);
//                } else {
//                    cvsPathPlan.prevPos = cvsPathPlan.currPos;
//                    cvsPathPlan.currPos = Qt.point(pos.x + robot.x, pos.y + robot.y);
//                }
//                cvsPathPlan.requestPaint();
////                console.log("["+cvsPathPlan.currPos.x + "," +cvsPathPlan.currPos.y+"]");
//            }
//            onPathClearClicked: {
//                cvsPathPlan.clearCanvas();
//            }
        }
    }

    Component.onCompleted: {
        console.log("GUI started");
        console.log("mainProcess:"+mainProcess);
        console.log("mainProcess.listArmLength:"+mainProcess.listArmLength[0]);
    }
}

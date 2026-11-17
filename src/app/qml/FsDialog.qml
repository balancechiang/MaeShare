// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
import QtQuick 1.0

Item {
    id: dlg
    anchors.fill: parent
    visible: false
    z: 100

    property string titleText: ""
    property string message: ""
    property string acceptText: "OK"
    property string rejectText: "Cancel"
    property bool showReject: true

    signal accepted()
    signal rejected()

    function open() { dlg.visible = true }
    function close() { dlg.visible = false }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.78
        MouseArea {
            anchors.fill: parent
            onClicked: {}   // 挡住下层页面的点击
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: 660
        height: content.height + 44
        radius: 16
        smooth: true
        color: "#22252A"
        border.color: "#3A5450"
        border.width: 1

        Column {
            id: content
            x: 26
            y: 22
            width: parent.width - 52
            spacing: 14

            Text {
                width: parent.width
                text: dlg.titleText
                font.pixelSize: 27
                font.bold: true
                color: "#FFFFFF"
                wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                text: dlg.message
                font.pixelSize: 21
                color: "#B0B6C2"
                wrapMode: Text.WordWrap
            }
            Row {
                width: parent.width
                spacing: 12

                FsButton {
                    width: (parent.width - 12) / 2
                    fontSize: 19
                    visible: dlg.showReject
                    text: dlg.rejectText
                    onClicked: {
                        dlg.close();
                        dlg.rejected();
                    }
                }
                FsButton {
                    width: dlg.showReject ? (parent.width - 12) / 2 : parent.width
                    fontSize: 19
                    primary: true
                    text: dlg.acceptText
                    onClicked: {
                        dlg.close();
                        dlg.accepted();
                    }
                }
            }
        }
    }
}

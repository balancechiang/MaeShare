// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Dark Material-3 style action button sized for the N900 resistive screen.
import QtQuick 1.0

Rectangle {
    id: btn
    property string text: ""
    property bool primary: false
    property int fontSize: 20
    property bool bold: false
    signal clicked()

    width: 200
    height: 50
    radius: 25
    smooth: true
    color: !enabled ? "#2A2E36"
         : mouse.pressed ? "#3A5450"
         : (primary ? "#82D5C8" : "#333E3B")

    Behavior on color {
        ColorAnimation { duration: 120 }
    }

    Text {
        anchors.centerIn: parent
        width: parent.width - 20
        text: btn.text
        font.pixelSize: btn.fontSize
        font.bold: btn.bold
        color: !btn.enabled ? "#6B727F" : (btn.primary ? "#181A1E" : "#E2E4E9")
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: btn.enabled
        onClicked: btn.clicked()
    }
}

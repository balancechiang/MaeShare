// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Left navigation rail pill (one glyph per tab, 52x38 for finger targets).
import QtQuick 1.0

Rectangle {
    id: pill
    property string labelText: ""
    property bool active: false
    signal clicked()

    width: 60
    height: 50
    radius: 25
    smooth: true
    color: active ? "#2A3240" : (mouse.pressed ? "#22252A" : "transparent")

    Behavior on color {
        ColorAnimation { duration: 120 }
    }

    Text {
        anchors.centerIn: parent
        text: pill.labelText
        color: pill.active ? "#82D5C8" : "#8A919E"
        font.pixelSize: 18
        font.bold: true
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        onClicked: pill.clicked()
    }
}

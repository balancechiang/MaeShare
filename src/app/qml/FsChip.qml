// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Small pill button used in page headers (Fremantle touch size, MD3 colours).
import QtQuick 1.0

Rectangle {
    id: chip
    property string text: ""
    property bool active: false
    signal clicked()

    width: chipLabel.width + 36
    height: 50
    radius: 25
    smooth: true
    color: active ? "#2A3240" : (mouse.pressed ? "#384152" : "#282C34")

    Text {
        id: chipLabel
        anchors.centerIn: parent
        text: chip.text
        color: chip.active ? "#FFFFFF" : "#82D5C8"
        font.pixelSize: 20
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: chip.enabled
        onClicked: chip.clicked()
    }
}

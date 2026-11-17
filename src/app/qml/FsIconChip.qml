// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Round icon button (same 60×50 / radius 25 touch size as FsChip) for header actions.
import QtQuick 1.0

Rectangle {
    id: chip
    property string icon: ""
    property string fallbackText: ""
    signal clicked()

    width: 60
    height: 50
    radius: 25
    smooth: true
    color: mouse.pressed ? "#384152" : "#282C34"

    Image {
        anchors.centerIn: parent
        width: 28
        height: 28
        smooth: true
        fillMode: Image.PreserveAspectFit
        source: chip.icon
        visible: chip.icon.length > 0
        opacity: mouse.pressed ? 0.55 : 1.0
    }

    Text {
        anchors.centerIn: parent
        visible: chip.icon.length === 0
        text: chip.fallbackText
        color: "#82D5C8"
        font.pixelSize: 20
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: chip.enabled
        onClicked: chip.clicked()
    }
}
